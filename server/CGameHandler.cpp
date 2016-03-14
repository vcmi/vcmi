#include "StdInc.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileInfo.h"
#include "../lib/int3.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/StartInfo.h"
#include "../lib/CModHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/spells/ISpellMechanics.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "../lib/ScopeGuard.h"
#include "../lib/CSoundBase.h"
#include "CGameHandler.h"
#include "CVCMIServer.h"
#include "../lib/CCreatureSet.h"
#include "../lib/CThreadHelper.h"
#include "../lib/GameConstants.h"
#include "../lib/registerTypes/RegisterTypes.h"

/*
 * CGameHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


#ifndef _MSC_VER
#include <boost/thread/xtime.hpp>
#endif
extern bool end2;
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define COMPLAIN_RET_IF(cond, txt) do {if(cond){complain(txt); return;}} while(0)
#define COMPLAIN_RET_FALSE_IF(cond, txt) do {if(cond){complain(txt); return false;}} while(0)
#define COMPLAIN_RET(txt) {complain(txt); return false;}
#define COMPLAIN_RETF(txt, FORMAT) {complain(boost::str(boost::format(txt) % FORMAT)); return false;}
#define NEW_ROUND 		BattleNextRound bnr;\
		bnr.round = gs->curB->round + 1;\
		sendAndApply(&bnr);

class ServerSpellCastEnvironment: public SpellCastEnvironment
{
public:
	ServerSpellCastEnvironment(CGameHandler * gh);
	~ServerSpellCastEnvironment(){};
	void sendAndApply(CPackForClient * info) const override;
	CRandomGenerator & getRandomGenerator() const override;
	void complain(const std::string & problem) const override;
	const CMap * getMap() const override;
	const CGameInfoCallback * getCb() const override;
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, PlayerColor asker = PlayerColor::NEUTRAL) const override;
private:
	mutable CGameHandler * gh;
};

CondSh<bool> battleMadeAction;
CondSh<BattleResult *> battleResult(nullptr);
template <typename T> class CApplyOnGH;

class CBaseForGHApply
{
public:
	virtual bool applyOnGH(CGameHandler *gh, CConnection *c, void *pack, PlayerColor player) const =0;
	virtual ~CBaseForGHApply(){}
	template<typename U> static CBaseForGHApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnGH<U>;
	}
};

template <typename T> class CApplyOnGH : public CBaseForGHApply
{
public:
	bool applyOnGH(CGameHandler *gh, CConnection *c, void *pack, PlayerColor player) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->c = c;
		ptr->player = player;
		return ptr->applyGh(gh);
	}
};

template <>
class CApplyOnGH<CPack> : public CBaseForGHApply
{
public:
	bool applyOnGH(CGameHandler *gh, CConnection *c, void *pack, PlayerColor player) const
	{
		logGlobal->errorStream() << "Cannot apply on GH plain CPack!";
		assert(0);
		return false;
	}
};

static CApplier<CBaseForGHApply> *applier = nullptr;

CMP_stack cmpst ;

static inline double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}
static void giveExp(BattleResult &r)
{
	if(r.winner > 1)
	{
		// draw
		return;
	}
	r.exp[0] = 0;
	r.exp[1] = 0;
	for(auto i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
	{
		r.exp[r.winner] += VLC->creh->creatures.at(i->first)->valOfBonuses(Bonus::STACK_HEALTH) * i->second;
	}
}

PlayerStatus PlayerStatuses::operator[](PlayerColor player)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		return players.at(player);
	}
	else
	{
		throw std::runtime_error("No such player!");
	}
}
void PlayerStatuses::addPlayer(PlayerColor player)
{
	boost::unique_lock<boost::mutex> l(mx);
	players[player];
}

bool PlayerStatuses::checkFlag(PlayerColor player, bool PlayerStatus::*flag)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		return players[player].*flag;
	}
	else
	{
		throw std::runtime_error("No such player!");
	}
}

void PlayerStatuses::setFlag(PlayerColor player, bool PlayerStatus::*flag, bool val)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		players[player].*flag = val;
	}
	else
	{
		throw std::runtime_error("No such player!");
	}
	cv.notify_all();
}

template <typename T>
void callWith(std::vector<T> args, std::function<void(T)> fun, ui32 which)
{
	fun(args[which]);
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero, SecondarySkill skill)
{
	changeSecSkill(hero, skill, 1, 0);
	expGiven(hero);
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero)
{
	// required exp for at least 1 lvl-up hasn't been reached
	if(!hero->gainsLevel())
	{
		return;
	}

	// give primary skill
	logGlobal->traceStream() << hero->name << " got level "<< hero->level;
	auto primarySkill = hero->nextPrimarySkill();

	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = primarySkill;
	sps.abs = false;
	sps.val = 1;
	sendAndApply(&sps);

	HeroLevelUp hlu;
	hlu.hero = hero;
	hlu.primskill = primarySkill;
	hlu.skills = hero->getLevelUpProposedSecondarySkills();

	if(hlu.skills.size() == 0)
	{
		sendAndApply(&hlu);
		levelUpHero(hero);
	}
	else if(hlu.skills.size() == 1  ||  hero->tempOwner == PlayerColor::NEUTRAL)  //choose skill automatically
	{
		sendAndApply(&hlu);
		levelUpHero(hero, *RandomGeneratorUtil::nextItem(hlu.skills, hero->skillsInfo.rand));
	}
	else if(hlu.skills.size() > 1)
	{
		auto levelUpQuery = std::make_shared<CHeroLevelUpDialogQuery>(hlu);
		hlu.queryID = levelUpQuery->queryID;
		queries.addQuery(levelUpQuery);
		sendAndApply(&hlu);
		//level up will be called on query reply
	}
}

void CGameHandler::levelUpCommander (const CCommanderInstance * c, int skill)
{
	SetCommanderProperty scp;

	auto hero = dynamic_cast<const CGHeroInstance *>(c->armyObj);
	if (hero)
		scp.heroid = hero->id;
	else
	{
		complain ("Commander is not led by hero!");
		return;
	}

	scp.accumulatedBonus.subtype = 0;
	scp.accumulatedBonus.additionalInfo = 0;
	scp.accumulatedBonus.duration = Bonus::PERMANENT;
	scp.accumulatedBonus.turnsRemain = 0;
	scp.accumulatedBonus.source = Bonus::COMMANDER;
	scp.accumulatedBonus.valType = Bonus::BASE_NUMBER;
	if (skill <= ECommander::SPELL_POWER)
	{
		scp.which = SetCommanderProperty::BONUS;

		auto difference = [](std::vector< std::vector <ui8> > skillLevels, std::vector <ui8> secondarySkills, int skill)->int
		{
			int s = std::min (skill, (int)ECommander::SPELL_POWER); //spell power level controls also casts and resistance
			return skillLevels.at(skill).at(secondarySkills.at(s)) - (secondarySkills.at(s) ? skillLevels.at(skill).at(secondarySkills.at(s)-1) : 0);
		};

		switch (skill)
		{
			case ECommander::ATTACK:
				scp.accumulatedBonus.type = Bonus::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = PrimarySkill::ATTACK;
				break;
			case ECommander::DEFENSE:
				scp.accumulatedBonus.type = Bonus::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = PrimarySkill::DEFENSE;
				break;
			case ECommander::HEALTH:
				scp.accumulatedBonus.type = Bonus::STACK_HEALTH;
				scp.accumulatedBonus.valType = Bonus::PERCENT_TO_BASE;
				break;
			case ECommander::DAMAGE:
				scp.accumulatedBonus.type = Bonus::CREATURE_DAMAGE;
				scp.accumulatedBonus.subtype = 0;
				scp.accumulatedBonus.valType = Bonus::PERCENT_TO_BASE;
				break;
			case ECommander::SPEED:
				scp.accumulatedBonus.type = Bonus::STACKS_SPEED;
				break;
			case ECommander::SPELL_POWER:
				scp.accumulatedBonus.type = Bonus::MAGIC_RESISTANCE;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::RESISTANCE);
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = Bonus::CREATURE_SPELL_POWER;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::SPELL_POWER) * 100; //like hero with spellpower = ability level
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = Bonus::CASTS;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::CASTS);
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = Bonus::CREATURE_ENCHANT_POWER; //send normally
				break;
		}

		scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, skill);
		sendAndApply (&scp);

		scp.which = SetCommanderProperty::SECONDARY_SKILL;
		scp.additionalInfo = skill;
		scp.amount = c->secondarySkills.at(skill) + 1;
		sendAndApply (&scp);
	}
	else if (skill >= 100)
	{
		scp.which = SetCommanderProperty::SPECIAL_SKILL;
		scp.accumulatedBonus = *VLC->creh->skillRequirements.at(skill-100).first;
		scp.additionalInfo = skill; //unnormalized
		sendAndApply (&scp);
	}
	expGiven(hero);
}

void CGameHandler::levelUpCommander(const CCommanderInstance * c)
{
	if (!c->gainsLevel())
	{
		return;
	}
	CommanderLevelUp clu;

	auto hero = dynamic_cast<const CGHeroInstance *>(c->armyObj);
	if (hero)
		clu.hero = hero;
	else
	{
		complain ("Commander is not led by hero!");
		return;
	}

	//picking sec. skills for choice

	for (int i = 0; i <= ECommander::SPELL_POWER; ++i)
	{
		if (c->secondarySkills.at(i) < ECommander::MAX_SKILL_LEVEL)
			clu.skills.push_back(i);
	}
	int i = 100;
	for (auto specialSkill : VLC->creh->skillRequirements)
	{
		if (c->secondarySkills.at(specialSkill.second.first) == ECommander::MAX_SKILL_LEVEL
			&&  c->secondarySkills.at(specialSkill.second.second) == ECommander::MAX_SKILL_LEVEL
			&&  !vstd::contains (c->specialSKills, i))
			clu.skills.push_back (i);
		++i;
	}
	int skillAmount = clu.skills.size();

	if(!skillAmount)
	{
		sendAndApply(&clu);
		levelUpCommander(c);
	}
	else if(skillAmount == 1  ||  hero->tempOwner == PlayerColor::NEUTRAL) //choose skill automatically
	{
		sendAndApply(&clu);
		levelUpCommander(c, *RandomGeneratorUtil::nextItem(clu.skills, gs->getRandomGenerator()));
	}
	else if(skillAmount > 1) //apply and ask for secondary skill
	{
		auto commanderLevelUp = std::make_shared<CCommanderLevelUpDialogQuery>(clu);
		clu.queryID = commanderLevelUp->queryID;
		queries.addQuery(commanderLevelUp);
		sendAndApply(&clu);
	}
}

void CGameHandler::expGiven(const CGHeroInstance *hero)
{
	if(hero->gainsLevel())
		levelUpHero(hero);
	else if(hero->commander && hero->commander->gainsLevel())
		levelUpCommander(hero->commander);

	//if(hero->commander && hero->level > hero->commander->level && hero->commander->gainsLevel())
// 		levelUpCommander(hero->commander);
// 	else
// 		levelUpHero(hero);
}

void CGameHandler::changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs)
{
	if (which == PrimarySkill::EXPERIENCE) // Check if scenario limit reached
	{
		if (gs->map->levelLimit != 0)
		{
			TExpType expLimit = VLC->heroh->reqExp(gs->map->levelLimit);
			TExpType resultingExp = abs ? val : hero->exp + val;
			if (resultingExp > expLimit)
			{
				// set given experience to max possible, but don't decrease if hero already over top
				abs = true;
				val = std::max(expLimit, hero->exp);

				InfoWindow iw;
				iw.player = hero->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 1); //can gain no more XP
				iw.text.addReplacement(hero->name);
				sendAndApply(&iw);
			}
		}
	}

	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = which;
	sps.abs = abs;
	sps.val = val;
	sendAndApply(&sps);

	//only for exp - hero may level up
	if (which == PrimarySkill::EXPERIENCE)
	{
		if(hero->commander && hero->commander->alive)
		{
			//FIXME: trim experience according to map limit?
			SetCommanderProperty scp;
			scp.heroid = hero->id;
			scp.which = SetCommanderProperty::EXPERIENCE;
			scp.amount = val;
			sendAndApply (&scp);
			CBonusSystemNode::treeHasChanged();
		}

		expGiven(hero);
	}
}

void CGameHandler::changeSecSkill( const CGHeroInstance * hero, SecondarySkill which, int val, bool abs/*=false*/ )
{
	SetSecSkill sss;
	sss.id = hero->id;
	sss.which = which;
	sss.val = val;
	sss.abs = abs;
	sendAndApply(&sss);

	if(which == SecondarySkill::WISDOM)
	{
		if(hero && hero->visitedTown)
			giveSpells(hero->visitedTown, hero);
	}
}

void CGameHandler::endBattle(int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	LOG_TRACE(logGlobal);

	//Fill BattleResult structure with exp info
	giveExp(*battleResult.data);

	if (battleResult.get()->result == BattleResult::NORMAL) // give 500 exp for defeating hero, unless he escaped
	{
		if (hero1)
			battleResult.data->exp[1] += 500;
		if (hero2)
			battleResult.data->exp[0] += 500;
	}

	if (hero1)
		battleResult.data->exp[0] = hero1->calculateXp(battleResult.data->exp[0]);//scholar skill
	if (hero2)
		battleResult.data->exp[1] = hero2->calculateXp(battleResult.data->exp[1]);

	const CArmedInstance *bEndArmy1 = gs->curB->sides.at(0).armyObject;
	const CArmedInstance *bEndArmy2 = gs->curB->sides.at(1).armyObject;
	const BattleResult::EResult result = battleResult.get()->result;

	auto findBattleQuery = [this]() -> std::shared_ptr<CBattleQuery>
	{
		for(auto &q : queries.allQueries())
		{
			if(auto bq = std::dynamic_pointer_cast<CBattleQuery>(q))
				if(bq->bi == gs->curB)
					return bq;
		}
		return std::shared_ptr<CBattleQuery>();
	};

	auto battleQuery = findBattleQuery();
	if(!battleQuery)
	{
		logGlobal->errorStream() << "Cannot find battle query!";
		if(gs->initialOpts->mode == StartInfo::DUEL)
		{
			battleQuery = std::make_shared<CBattleQuery>(gs->curB);
		}
	}
	if(battleQuery != queries.topQuery(gs->curB->sides[0].color))
		complain("Player " + boost::lexical_cast<std::string>(gs->curB->sides[0].color) + " although in battle has no battle query at the top!");

	battleQuery->result = *battleResult.data;

	//Check how many battle queries were created (number of players blocked by battle)
	const int queriedPlayers = battleQuery ? boost::count(queries.allQueries(), battleQuery) : 0;
	finishingBattle = make_unique<FinishingBattleHelper>(battleQuery, gs->initialOpts->mode == StartInfo::DUEL, queriedPlayers);


	CasualtiesAfterBattle cab1(bEndArmy1, gs->curB), cab2(bEndArmy2, gs->curB); //calculate casualties before deleting battle

	if(finishingBattle->duel)
	{
		duelFinished();
		sendAndApply(battleResult.data); //after this point casualties objects are destroyed
		return;
	}


	ChangeSpells cs; //for Eagle Eye

	if(finishingBattle->winnerHero)
	{
		if(int eagleEyeLevel = finishingBattle->winnerHero->getSecSkillLevel(SecondarySkill::EAGLE_EYE))
		{
			int maxLevel = eagleEyeLevel + 1;
			double eagleEyeChance = finishingBattle->winnerHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::EAGLE_EYE);
			for(const CSpell *sp : gs->curB->sides.at(!battleResult.data->winner).usedSpellsHistory)
				if(sp->level <= maxLevel && !vstd::contains(finishingBattle->winnerHero->spells, sp->id) && gs->getRandomGenerator().nextInt(99) < eagleEyeChance)
					cs.spells.insert(sp->id);
		}
	}


	std::vector<const CArtifactInstance *> arts; //display them in window

	if(result == BattleResult::NORMAL && finishingBattle->winnerHero)
	{
		auto sendMoveArtifact = [&](const CArtifactInstance *art, MoveArtifact *ma)
		{
			arts.push_back(art);
			ma->dst = ArtifactLocation(finishingBattle->winnerHero, art->firstAvailableSlot(finishingBattle->winnerHero));
			sendAndApply(ma);
		};
		if(finishingBattle->loserHero)
		{
			//TODO: wrap it into a function, somehow (boost::variant -_-)
			auto artifactsWorn = finishingBattle->loserHero->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if(art && !art->artType->isBig() &&
				    art->artType->id != ArtifactID::SPELLBOOK)
						// don't move war machines or locked arts (spellbook)
				{
					sendMoveArtifact(art, &ma);
				}
			}
			while(!finishingBattle->loserHero->artifactsInBackpack.empty())
			{
				//we assume that no big artifacts can be found
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero,
					ArtifactPosition(GameConstants::BACKPACK_START)); //backpack automatically shifts arts to beginning
				const CArtifactInstance * art =  ma.src.getArt();
				if(art->artType->id != ArtifactID::GRAIL) //grail may not be won
				{
					sendMoveArtifact(art, &ma);
				}
			}
			if(finishingBattle->loserHero->commander) //TODO: what if commanders belong to no hero?
			{
				artifactsWorn = finishingBattle->loserHero->commander->artifactsWorn;
				for(auto artSlot : artifactsWorn)
				{
					MoveArtifact ma;
					ma.src = ArtifactLocation(finishingBattle->loserHero->commander.get(), artSlot.first);
					const CArtifactInstance * art =  ma.src.getArt();
					if (art && !art->artType->isBig())
					{
						sendMoveArtifact(art, &ma);
					}
				}
			}
		}
		for(auto armySlot : gs->curB->battleGetArmyObject(!battleResult.data->winner)->stacks)
		{
			auto artifactsWorn = armySlot.second->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(armySlot.second, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if (art && !art->artType->isBig())
				{
					sendMoveArtifact(art, &ma);
				}
			}
		}
	}

	sendAndApply(battleResult.data); //after this point casualties objects are destroyed

	if(arts.size()) //display loot
	{
		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;

		iw.text.addTxt (MetaString::GENERAL_TXT, 30); //You have captured enemy artifact

		for(auto art : arts) //TODO; separate function to display loot for various ojects?
		{
			iw.components.push_back(Component(
				Component::ARTIFACT, art->artType->id,
				art->artType->id == ArtifactID::SPELL_SCROLL? art->getGivenSpellID() : 0, 0));
			if(iw.components.size() >= 14)
			{
				sendAndApply(&iw);
				iw.components.clear();
			}
		}
		if(iw.components.size())
		{
			sendAndApply(&iw);
		}
	}
	//Eagle Eye secondary skill handling
	if(!cs.spells.empty())
	{
		cs.learn = 1;
		cs.hid = finishingBattle->winnerHero->id;

		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 221); //Through eagle-eyed observation, %s is able to learn %s
		iw.text.addReplacement(finishingBattle->winnerHero->name);

		std::ostringstream names;
		for(int i = 0; i < cs.spells.size(); i++)
		{
			names << "%s";
			if(i < cs.spells.size() - 2)
				names << ", ";
			else if(i < cs.spells.size() - 1)
				names << "%s";
		}
		names << ".";

		iw.text.addReplacement(names.str());

		auto it = cs.spells.begin();
		for(int i = 0; i < cs.spells.size(); i++, it++)
		{
			iw.text.addReplacement(MetaString::SPELL_NAME, it->toEnum());
			if(i == cs.spells.size() - 2) //we just added pre-last name
				iw.text.addReplacement(MetaString::GENERAL_TXT, 141); // " and "
			iw.components.push_back(Component(Component::SPELL, *it, 0, 0));
		}

		sendAndApply(&iw);
		sendAndApply(&cs);
	}

	cab1.updateArmy(this);
	cab2.updateArmy(this); //take casualties after battle is deleted

	//if one hero has lost we will erase him
	if(battleResult.data->winner!=0 && hero1)
	{
		RemoveObject ro(hero1->id);
		sendAndApply(&ro);
	}
	if(battleResult.data->winner!=1 && hero2)
	{
		auto town = hero2->visitedTown;
		RemoveObject ro(hero2->id);
		sendAndApply(&ro);

		if(town && !town->garrisonHero) // TODO: that must be called from CGHeroInstance or CGTownInstance
			town->battleFinished(hero1, *battleResult.get());
	}

	//give exp
	if (battleResult.data->exp[0] && hero1 && battleResult.get()->winner == 0)
		changePrimSkill(hero1, PrimarySkill::EXPERIENCE, battleResult.data->exp[0]);
	else if (battleResult.data->exp[1] && hero2 && battleResult.get()->winner == 1)
		changePrimSkill(hero2, PrimarySkill::EXPERIENCE, battleResult.data->exp[1]);

	queries.popIfTop(battleQuery);

	//--> continuation (battleAfterLevelUp) occurs after level-up queries are handled or on removing query (above)
}

void CGameHandler::battleAfterLevelUp( const BattleResult &result )
{
	LOG_TRACE(logGlobal);


	finishingBattle->remainingBattleQueriesCount--;
	logGlobal->traceStream() << "Decremented queries count to " << finishingBattle->remainingBattleQueriesCount;

	if(finishingBattle->remainingBattleQueriesCount > 0)
		//Battle results will be handled when all battle queries are closed
		return;

	//TODO consider if we really want it to work like above. ATM each player as unblocked as soon as possible
	// but the battle consequences are applied after final player is unblocked. Hard to abuse...
	// Still, it looks like a hole.

	// Necromancy if applicable.
	const CStackBasicDescriptor raisedStack = finishingBattle->winnerHero ? finishingBattle->winnerHero->calculateNecromancy(*battleResult.data) : CStackBasicDescriptor();
	// Give raised units to winner and show dialog, if any were raised,
	// units will be given after casualties are taken
	const SlotID necroSlot = raisedStack.type ? finishingBattle->winnerHero->getSlotFor(raisedStack.type) : SlotID();

	if (necroSlot != SlotID())
	{
		finishingBattle->winnerHero->showNecromancyDialog(raisedStack);
		addToSlot(StackLocation(finishingBattle->winnerHero, necroSlot), raisedStack.type, raisedStack.count);
	}

	BattleResultsApplied resultsApplied;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	sendAndApply(&resultsApplied);

	setBattle(nullptr);

	if(visitObjectAfterVictory && result.winner==0 && !finishingBattle->winnerHero->stacks.empty())
	{
		logGlobal->traceStream() << "post-victory visit";
		visitObjectOnTile(*getTile(finishingBattle->winnerHero->getPosition()), finishingBattle->winnerHero);
	}
	visitObjectAfterVictory = false;

	//handle victory/loss of engaged players
	std::set<PlayerColor> playerColors = {finishingBattle->loser, finishingBattle->victor};
	checkVictoryLossConditions(playerColors);

	if(result.result == BattleResult::SURRENDER || result.result == BattleResult::ESCAPE) //loser has escaped or surrendered
	{
		SetAvailableHeroes sah;
		sah.player = finishingBattle->loser;
		sah.hid[0] = finishingBattle->loserHero->subID;
		if(result.result == BattleResult::ESCAPE) //retreat
		{
			sah.army[0].clear();
			sah.army[0].setCreature(SlotID(0), finishingBattle->loserHero->type->initialArmy.at(0).creature, 1);
		}

		if(const CGHeroInstance *another =  getPlayer(finishingBattle->loser)->availableHeroes.at(0))
			sah.hid[1] = another->subID;
		else
			sah.hid[1] = -1;

		sendAndApply(&sah);
	}
	if(result.winner != 2 && finishingBattle->winnerHero && finishingBattle->winnerHero->stacks.empty())
	{
		RemoveObject ro(finishingBattle->winnerHero->id);
		sendAndApply(&ro);

		if (VLC->modh->settings.WINNING_HERO_WITH_NO_TROOPS_RETREATS)
		{
			SetAvailableHeroes sah;
			sah.player = finishingBattle->victor;
			sah.hid[0] = finishingBattle->winnerHero->subID;
			sah.army[0].clear();
			sah.army[0].setCreature(SlotID(0), finishingBattle->winnerHero->type->initialArmy.at(0).creature, 1);

			if(const CGHeroInstance *another =  getPlayer(finishingBattle->victor)->availableHeroes.at(0))
				sah.hid[1] = another->subID;
			else
				sah.hid[1] = -1;

			sendAndApply(&sah);
		}
	}
}

void CGameHandler::prepareAttack(BattleAttack &bat, const CStack *att, const CStack *def, int distance, int targetHex)
{
	bat.bsa.clear();
	bat.stackAttacking = att->ID;
	const int attackerLuck = att->LuckVal();

	auto sideHeroBlocksLuck = [](const SideInBattle &side){ return NBonus::hasOfType(side.hero, Bonus::BLOCK_LUCK); };

	if(!vstd::contains_if(gs->curB->sides, sideHeroBlocksLuck))
	{
		if(attackerLuck > 0  && gs->getRandomGenerator().nextInt(23) < attackerLuck)
		{
			bat.flags |= BattleAttack::LUCKY;
		}
		if (VLC->modh->settings.data["hardcodedFeatures"]["NEGATIVE_LUCK"].Bool()) // negative luck enabled
		{
			if (attackerLuck < 0 && gs->getRandomGenerator().nextInt(23) < abs(attackerLuck))
			{
				bat.flags |= BattleAttack::UNLUCKY;
			}
		}
	}

	if(gs->getRandomGenerator().nextInt(99) < att->valOfBonuses(Bonus::DOUBLE_DAMAGE_CHANCE))
	{
		bat.flags |= BattleAttack::DEATH_BLOW;
	}

	if(att->getCreature()->idNumber == CreatureID::BALLISTA)
	{
		static const int artilleryLvlToChance[] = {0, 50, 75, 100};
		const CGHeroInstance * owner = gs->curB->getHero(att->owner);
		int chance = artilleryLvlToChance[owner->getSecSkillLevel(SecondarySkill::ARTILLERY)];
		if(chance > gs->getRandomGenerator().nextInt(99))
		{
			bat.flags |= BattleAttack::BALLISTA_DOUBLE_DMG;
		}
	}
	// only primary target
	applyBattleEffects(bat, att, def, distance, false);

	if (!bat.shot()) //multiple-hex attack - only in meele
	{
		std::set<const CStack*> attackedCreatures = gs->curB->getAttackedCreatures(att, targetHex); //creatures other than primary target

		for(const CStack * stack : attackedCreatures)
		{
			if (stack != def) //do not hit same stack twice
			{
				applyBattleEffects(bat, att, stack, distance, true);
			}
		}
	}

	const Bonus * bonus = att->getBonusLocalFirst(Selector::type(Bonus::SPELL_LIKE_ATTACK));
	if (bonus && (bat.shot())) //TODO: make it work in melee?
	{
		//this is need for displaying hit animation
		bat.flags |= BattleAttack::SPELL_LIKE;
		bat.spellID = SpellID(bonus->subtype);

		//TODO: should spell override creature`s projectile?

		std::set<const CStack*> attackedCreatures = SpellID(bonus->subtype).toSpell()->getAffectedStacks(gs->curB, ECastingMode::SPELL_LIKE_ATTACK, att->owner, bonus->val, targetHex, att);

		//TODO: get exact attacked hex for defender

		for(const CStack * stack : attackedCreatures)
		{
			if (stack != def) //do not hit same stack twice
			{
				applyBattleEffects(bat, att, stack, distance, true);
			}
		}

		//now add effect info for all attacked stacks
		for(BattleStackAttacked & bsa : bat.bsa)
		{
			if(bsa.attackerID == att->ID) //this is our attack and not f.e. fire shield
			{
				//this is need for displaying affect animation
				bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
				bsa.spellID = SpellID(bonus->subtype);
			}
		}

	}
}
void CGameHandler::applyBattleEffects(BattleAttack &bat, const CStack *att, const CStack *def, int distance, bool secondary) //helper function for prepareAttack
{
	BattleStackAttacked bsa;
	if (secondary)
		bsa.flags |= BattleStackAttacked::SECONDARY; //all other targets do not suffer from spells & spell-like abilities
	bsa.attackerID = att->ID;
	bsa.stackAttacked = def->ID;
	bsa.damageAmount = gs->curB->calculateDmg(att, def, gs->curB->battleGetOwner(att), gs->curB->battleGetOwner(def),
		bat.shot(), distance, bat.lucky(), bat.unlucky(), bat.deathBlow(), bat.ballistaDoubleDmg(), gs->getRandomGenerator());
	def->prepareAttacked(bsa, gs->getRandomGenerator()); //calculate casualties

	//life drain handling
	if (att->hasBonusOfType(Bonus::LIFE_DRAIN) && def->isLiving())
	{
		StacksHealedOrResurrected shi;
		shi.lifeDrain = true;
		shi.tentHealing = false;
		shi.drainedFrom = def->ID;

		StacksHealedOrResurrected::HealInfo hi;
		hi.stackID = att->ID;
		hi.healedHP = att->calculateHealedHealthPoints(bsa.damageAmount * att->valOfBonuses (Bonus::LIFE_DRAIN) / 100, true);
		hi.lowLevelResurrection = false;
		shi.healedStacks.push_back(hi);

		if (hi.healedHP > 0)
		{
			bsa.healedStacks.push_back(shi);
		}
	}
	bat.bsa.push_back(bsa); //add this stack to the list of victims after drain life has been calculated

	//fire shield handling
	if(!bat.shot() && !vstd::contains(def->state, EBattleStackState::CLONED) &&
		def->hasBonusOfType(Bonus::FIRE_SHIELD) && !att->hasBonusOfType(Bonus::FIRE_IMMUNITY))
	{
		// TODO: Fire sheild damage should be calculated separately after BattleAttack applied.
		// Currently it's looks like attacking stack damage itself with defenders fire shield.
		// So no separate message on spell damge in log and expirience calculation is likely wrong too.
		BattleStackAttacked bsa2;
		bsa2.stackAttacked = att->ID; //invert
		bsa2.attackerID = def->ID;
		bsa2.flags |= BattleStackAttacked::EFFECT; //FIXME: play animation upon efreet and not attacker
		bsa2.effect = 11;

		bsa2.damageAmount = (std::min(def->totalHelth(), bsa.damageAmount) * def->valOfBonuses(Bonus::FIRE_SHIELD)) / 100; //TODO: scale with attack/defense
		att->prepareAttacked(bsa2, gameState()->getRandomGenerator());
		bat.bsa.push_back(bsa2);
	}
}
void CGameHandler::handleConnection(std::set<PlayerColor> players, CConnection &c)
{
	setThreadName("CGameHandler::handleConnection");

	try
	{
		while(1)//server should never shut connection first //was: while(!end2)
		{
			CPack *pack = nullptr;
			PlayerColor player = PlayerColor::NEUTRAL;
			si32 requestID = -999;
			int packType = 0;

			{
				boost::unique_lock<boost::mutex> lock(*c.rmx);
				c >> player >> requestID >> pack; //get the package

				if(!pack)
				{
					logGlobal ->errorStream() << boost::format("Received a null package marked as request %d from player %d") % requestID % player;
				}

				packType = typeList.getTypeID(pack); //get the id of type

				logGlobal->traceStream() << boost::format("Received client message (request %d by player %d) of type with ID=%d (%s).\n")
					% requestID % player.getNum() % packType % typeid(*pack).name();
			}

			//prepare struct informing that action was applied
			auto sendPackageResponse = [&](bool succesfullyApplied)
			{
				PackageApplied applied;
				applied.player = player;
				applied.result = succesfullyApplied;
				applied.packType = packType;
				applied.requestID = requestID;
				boost::unique_lock<boost::mutex> lock(*c.wmx);
				c << &applied;
			};

			CBaseForGHApply *apply = applier->apps[packType]; //and appropriate applier object
			if(isBlockedByQueries(pack, player))
			{
				sendPackageResponse(false);
			}
			else if(apply)
			{
				const bool result = apply->applyOnGH(this,&c,pack, player);
				if(!result)
				{
					complain((boost::format("Got false in applying %s... that request must have been fishy!")
						% typeid(*pack).name()).str());
				}
				logGlobal->traceStream() << "Message successfully applied (result=" << result << ")!";
				sendPackageResponse(true);
			}
			else
			{
				logGlobal->errorStream() << "Message cannot be applied, cannot find applier (unregistered type)!";
				sendPackageResponse(false);
			}

			vstd::clear_pointer(pack);
		}
	}
	catch(boost::system::system_error &e) //for boost errors just log, not crash - probably client shut down connection
	{
		assert(!c.connected); //make sure that connection has been marked as broken
		logGlobal->errorStream() << e.what();
		end2 = true;
	}
	catch(...)
	{
		end2 = true;
		handleException();
		throw;
	}

	logGlobal->errorStream() << "Ended handling connection";
}

int CGameHandler::moveStack(int stack, BattleHex dest)
{
	int ret = 0;

	const CStack *curStack = gs->curB->battleGetStackByID(stack),
		*stackAtEnd = gs->curB->battleGetStackByPos(dest);

	assert(curStack);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (gs->curB->tacticDistance)
	{
		assert(gs->curB->isInTacticRange(dest));
	}

	auto start = curStack->position;
	if(start == dest)
		return 0;

	//initing necessary tables
	auto accessibility = getAccesibility(curStack);

	//shifting destination (if we have double wide stack and we can occupy dest but not be exactly there)
	if(!stackAtEnd && curStack->doubleWide() && !accessibility.accessible(dest, curStack))
	{
		if(curStack->attackerOwned)
		{
			if(accessibility.accessible(dest+1, curStack))
				dest += BattleHex::RIGHT;
		}
		else
		{
			if(accessibility.accessible(dest-1, curStack))
				dest += BattleHex::LEFT;
		}
	}

	if((stackAtEnd && stackAtEnd!=curStack && stackAtEnd->alive()) || !accessibility.accessible(dest, curStack))
	{
		complain("Given destination is not accessible!");
		return 0;
	}

	bool canUseGate = false;
	auto dbState = gs->curB->si.gateState;
	if(battleGetSiegeLevel() > 0 && !curStack->attackerOwned &&
		dbState != EGateState::DESTROYED &&
		dbState != EGateState::BLOCKED)
	{
		canUseGate = true;
	}

	std::pair< std::vector<BattleHex>, int > path = gs->curB->getPath(start, dest, curStack);

	ret = path.second;

	int creSpeed = gs->curB->tacticDistance ? GameConstants::BFIELD_SIZE : curStack->Speed();

	auto isGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if(gs->curB->town->subID == ETownType::FORTRESS && hex == ESiegeHex::GATE_BRIDGE)
			return true;
		if(hex == ESiegeHex::GATE_OUTER)
			return true;
		if(hex == ESiegeHex::GATE_INNER)
			return true;

		return false;
	};

	auto occupyGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if(isGateDrawbridgeHex(hex))
			return true;

		if(curStack->doubleWide())
		{
			BattleHex otherHex = curStack->occupiedHex(hex);
			if(otherHex.isValid() && isGateDrawbridgeHex(otherHex))
				return true;
		}

		return false;
	};

	if(curStack->hasBonusOfType(Bonus::FLYING))
	{
		if(path.second <= creSpeed && path.first.size() > 0)
		{
			if(canUseGate && dbState != EGateState::OPENED &&
				occupyGateDrawbridgeHex(dest))
			{
				BattleUpdateGateState db;
				db.state = EGateState::OPENED;
				sendAndApply(&db);
			}

			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			std::vector<BattleHex> tiles;
			tiles.push_back(path.first[0]);
			sm.tilesToMove = tiles;
			sm.distance = path.second;
			sm.teleporting = false;
			sendAndApply(&sm);
		}
	}
	else //for non-flying creatures
	{
		std::shared_ptr<const CObstacleInstance> obstacle, obstacle2; //obstacle that interrupted movement
		std::vector<BattleHex> tiles;
		const int tilesToMove = std::max((int)(path.first.size() - creSpeed), 0);
		int v = path.first.size()-1;
		path.first.push_back(start);

		// check if gate need to be open or closed at some point
		BattleHex openGateAtHex, gateMayCloseAtHex;
		if(canUseGate)
		{
			for(int i = path.first.size()-1; i >= 0; i--)
			{
				auto needOpenGates = [&](BattleHex hex) -> bool
				{
					if(gs->curB->town->subID == ETownType::FORTRESS && hex == ESiegeHex::GATE_BRIDGE)
						return true;
					if(hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] == ESiegeHex::GATE_OUTER)
						return true;
					else if(hex == ESiegeHex::GATE_OUTER || hex == ESiegeHex::GATE_INNER)
						return true;

					return false;
				};

				auto hex = path.first[i];
				if(!openGateAtHex.isValid() && dbState != EGateState::OPENED)
				{
					if(needOpenGates(hex))
						openGateAtHex = path.first[i+1];

					//TODO we need find batter way to handle double-wide stacks
					//currently if only second occupied stack part is standing on gate / bridge hex then stack will start to wait for bridge to lower before it's needed. Though this is just a visual bug.
					if(curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
						if(otherHex.isValid() && needOpenGates(otherHex))
							openGateAtHex = path.first[i+2];
					}

					//gate may be opened and then closed during stack movement, but not other way around
					if(openGateAtHex.isValid())
						dbState = EGateState::OPENED;
				}

				if(!gateMayCloseAtHex.isValid() && dbState != EGateState::CLOSED)
				{
					if(hex == ESiegeHex::GATE_INNER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
					if(gs->curB->town->subID == ETownType::FORTRESS)
					{
						if(hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
						else if(hex == ESiegeHex::GATE_OUTER && i-1 >= 0 &&
							path.first[i-1] != ESiegeHex::GATE_INNER &&
							path.first[i-1] != ESiegeHex::GATE_BRIDGE)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
					}
					else if(hex == ESiegeHex::GATE_OUTER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_INNER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
				}
			}
		}

		bool stackIsMoving = true;

		while(stackIsMoving)
		{
			if(v<tilesToMove)
			{
				logGlobal->error("Movement terminated abnormally");
				break;
			}

			bool gateStateChanging = false;
			//special handling for opening gate on from starting hex
			if(openGateAtHex.isValid() && openGateAtHex == start)
				gateStateChanging = true;
			else
			{
				for(bool obstacleHit = false; (!obstacleHit) && (!gateStateChanging) && (v >= tilesToMove); --v)
				{
					BattleHex hex = path.first[v];
					tiles.push_back(hex);

					if((openGateAtHex.isValid() && openGateAtHex == hex) ||
						(gateMayCloseAtHex.isValid() && gateMayCloseAtHex == hex))
					{
						gateStateChanging = true;
					}

					//if we walked onto something, finalize this portion of stack movement check into obstacle
					if((obstacle = battleGetObstacleOnPos(hex, false)))
						obstacleHit = true;

					if(curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);

						//two hex creature hit obstacle by backside
						if(otherHex.isValid() && ((obstacle2 = battleGetObstacleOnPos(otherHex, false))))
							obstacleHit = true;
					}
				}
			}

			if(tiles.size() > 0)
			{
				//commit movement
				BattleStackMoved sm;
				sm.stack = curStack->ID;
				sm.distance = path.second;
				sm.teleporting = false;
				sm.tilesToMove = tiles;
				sendAndApply(&sm);
				tiles.clear();
			}

			//we don't handle obstacle at the destination tile -> it's handled separately in the if at the end
			if(curStack->position != dest)
			{
				auto processObstacle = [&](std::shared_ptr<const CObstacleInstance> & obs)
				{
					if(obs)
					{
						handleDamageFromObstacle(*obs, curStack);

						//if stack die in explosion or interrupted by obstacle, abort movement
						if(obs->stopsMovement() || !curStack->alive())
							stackIsMoving = false;

						obs.reset();
					}
				};

				processObstacle(obstacle);
				if(curStack->alive())
					processObstacle(obstacle2);

				if(gateStateChanging)
				{
					if(curStack->position == openGateAtHex)
					{
						openGateAtHex = BattleHex();
						//only open gate if stack is still alive
						if(curStack->alive())
						{
							BattleUpdateGateState db;
							db.state = EGateState::OPENED;
							sendAndApply(&db);
						}
					}
					else if(curStack->position == gateMayCloseAtHex)
					{
						gateMayCloseAtHex = BattleHex();
						updateGateState();
					}
				}
			}
			else
				//movement finished normally: we reached destination
				stackIsMoving = false;
		}
	}

	//handling obstacle on the final field (separate, because it affects both flying and walking stacks)
	if(curStack->alive())
	{
		if(auto theLastObstacle = battleGetObstacleOnPos(curStack->position, false))
		{
			handleDamageFromObstacle(*theLastObstacle, curStack);
		}
	}

	if(curStack->alive() && curStack->doubleWide())
	{
		BattleHex otherHex = curStack->occupiedHex(curStack->position);

		if(otherHex.isValid())
			if(auto theLastObstacle = battleGetObstacleOnPos(otherHex, false))
			{
				//two hex creature hit obstacle by backside
				handleDamageFromObstacle(*theLastObstacle, curStack);
			}
	}
	return ret;
}

CGameHandler::CGameHandler(void)
{
	QID = 1;
	//gs = nullptr;
	IObjectInterface::cb = this;
	applier = new CApplier<CBaseForGHApply>;
	registerTypesServerPacks(*applier);
	visitObjectAfterVictory = false;
	queries.gh = this;

	spellEnv = new ServerSpellCastEnvironment(this);
}

CGameHandler::~CGameHandler(void)
{
	delete spellEnv;
	delete applier;
	applier = nullptr;
	delete gs;
}

void CGameHandler::init(StartInfo *si)
{
	if(si->seedToBeUsed == 0)
	{
		si->seedToBeUsed = std::time(nullptr);
	}

	gs = new CGameState();
	logGlobal->infoStream() << "Gamestate created!";
	gs->init(si);
	logGlobal->infoStream() << "Gamestate initialized!";

	// reset seed, so that clients can't predict any following random values
	gs->getRandomGenerator().resetSeed();

	for(auto & elem : gs->players)
	{
		states.addPlayer(elem.first);
	}
}

static bool evntCmp(const CMapEvent &a, const CMapEvent &b)
{
	return a.earlierThan(b);
}

void CGameHandler::setPortalDwelling(const CGTownInstance * town, bool forced=false, bool clear = false)
{// bool forced = true - if creature should be replaced, if false - only if no creature was set
	const PlayerState *p = gs->getPlayer(town->tempOwner);
	if(!p)
	{
		logGlobal->warnStream() << "There is no player owner of town " << town->name << " at " << town->pos;
		return;
	}

	if (forced || town->creatures.at(GameConstants::CREATURES_PER_TOWN).second.empty())//we need to change creature
		{
			SetAvailableCreatures ssi;
			ssi.tid = town->id;
			ssi.creatures = town->creatures;
			ssi.creatures[GameConstants::CREATURES_PER_TOWN].second.clear();//remove old one

			const std::vector<ConstTransitivePtr<CGDwelling> > &dwellings = p->dwellings;
			if (dwellings.empty())//no dwellings - just remove
			{
				sendAndApply(&ssi);
				return;
			}

			auto dwelling = *RandomGeneratorUtil::nextItem(dwellings, gs->getRandomGenerator());

			// for multi-creature dwellings like Golem Factory
			auto creatureId = RandomGeneratorUtil::nextItem(dwelling->creatures, gs->getRandomGenerator())->second[0];

			if(clear)
			{
				ssi.creatures[GameConstants::CREATURES_PER_TOWN].first = std::max((ui32)1, (VLC->creh->creatures.at(creatureId)->growth)/2);
			}
			else
			{
				ssi.creatures[GameConstants::CREATURES_PER_TOWN].first = VLC->creh->creatures.at(creatureId)->growth;
			}
			ssi.creatures[GameConstants::CREATURES_PER_TOWN].second.push_back(creatureId);
			sendAndApply(&ssi);
		}
}

void CGameHandler::newTurn()
{
	logGlobal->traceStream() << "Turn " << gs->day+1;
	NewTurn n;
	n.specialWeek = NewTurn::NO_ACTION;
	n.creatureid = CreatureID::NONE;
	n.day = gs->day + 1;

	bool firstTurn = !getDate(Date::DAY);
	bool newWeek = getDate(Date::DAY_OF_WEEK) == 7; //day numbers are confusing, as day was not yet switched
	bool newMonth = getDate(Date::DAY_OF_MONTH) == 28;

	std::map<PlayerColor, si32> hadGold;//starting gold - for buildings like dwarven treasury

	if (firstTurn)
	{
		for (auto obj : gs->map->objects)
		{
			if (obj && obj->ID == Obj::PRISON) //give imprisoned hero 0 exp to level him up. easiest to do at this point
			{
				changePrimSkill (getHero(obj->id), PrimarySkill::EXPERIENCE, 0);
			}
		}
	}

	if (newWeek && !firstTurn)
	{
		n.specialWeek = NewTurn::NORMAL;
		bool deityOfFireBuilt = false;
		for(const CGTownInstance *t : gs->map->towns)
		{
			if(t->hasBuilt(BuildingID::GRAIL, ETownType::INFERNO))
			{
				deityOfFireBuilt = true;
				break;
			}
		}

		if(deityOfFireBuilt)
		{
			n.specialWeek = NewTurn::DEITYOFFIRE;
			n.creatureid = CreatureID::IMP;
		}
		else
		{
			int monthType = gs->getRandomGenerator().nextInt(99);
			if(newMonth) //new month
			{
				if (monthType < 40) //double growth
				{
					n.specialWeek = NewTurn::DOUBLE_GROWTH;
					if (VLC->modh->settings.ALL_CREATURES_GET_DOUBLE_MONTHS)
					{
						std::pair<int, CreatureID> newMonster(54, VLC->creh->pickRandomMonster(gs->getRandomGenerator()));
						n.creatureid = newMonster.second;
					}
					else if(VLC->creh->doubledCreatures.size())
					{
						const std::vector<CreatureID> doubledCreatures (VLC->creh->doubledCreatures.begin(), VLC->creh->doubledCreatures.end());
						n.creatureid = *RandomGeneratorUtil::nextItem(doubledCreatures, gs->getRandomGenerator());
					}
					else
					{
						complain("Cannot find creature that can be spawned!");
						n.specialWeek = NewTurn::NORMAL;
					}
				}
				else if (monthType < 50)
					n.specialWeek = NewTurn::PLAGUE;
			}
			else //it's a week, but not full month
			{
				if (monthType < 25)
				{
					n.specialWeek = NewTurn::BONUS_GROWTH; //+5
					std::pair<int, CreatureID> newMonster(54, VLC->creh->pickRandomMonster(gs->getRandomGenerator()));
					//TODO do not pick neutrals
					n.creatureid = newMonster.second;
				}
			}
		}
	}

	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > pool = gs->hpool.heroesPool;

	for(auto& hp : pool)
	{
		auto hero = hp.second;
		if(hero->isInitialized() && hero->stacks.size())
		{
			// reset retreated or surrendered heroes
			auto maxmove = hero->maxMovePoints(true);
			// if movement is greater than maxmove, we should decrease it
			if(hero->movement != maxmove || hero->mana < hero->manaLimit())
			{
				NewTurn::Hero hth;
				hth.id = hero->id;
				hth.move = maxmove;
				hth.mana = hero->getManaNewTurn();
				n.heroes.insert(hth);
			}
		}
	}

	for (auto & elem : gs->players)
	{
		if(elem.first == PlayerColor::NEUTRAL)
			continue;
		else if(elem.first >= PlayerColor::PLAYER_LIMIT)
			assert(0); //illegal player number!

		std::pair<PlayerColor, si32> playerGold(elem.first, elem.second.resources.at(Res::GOLD));
		hadGold.insert(playerGold);

		if(newWeek) //new heroes in tavern
		{
			SetAvailableHeroes sah;
			sah.player = elem.first;

			//pick heroes and their armies
			CHeroClass *banned = nullptr;
			for (int j = 0; j < GameConstants::AVAILABLE_HEROES_PER_PLAYER; j++)
			{
				//first hero - native if possible, second hero -> any other class
				if(CGHeroInstance *h = gs->hpool.pickHeroFor(j == 0, elem.first, getNativeTown(elem.first), pool, gs->getRandomGenerator(), banned))
				{
					sah.hid[j] = h->subID;
					h->initArmy(&sah.army[j]);
					banned = h->type->heroClass;
				}
				else
				{
					sah.hid[j] = -1;
				}
			}

			sendAndApply(&sah);
		}

		n.res[elem.first] = elem.second.resources;

		for(CGHeroInstance *h : (elem).second.heroes)
		{
			if(h->visitedTown)
				giveSpells(h->visitedTown, h);

			NewTurn::Hero hth;
			hth.id = h->id;
			// TODO: this code executed when bonuses of previous day not yet updated (this happen in NewTurn::applyGs). See issue 2356
			hth.move = h->maxMovePoints(gs->map->getTile(h->getPosition(false)).terType != ETerrainType::WATER, new TurnInfo(h, 1));
			hth.mana = h->getManaNewTurn();

			n.heroes.insert(hth);

			if(!firstTurn) //not first day
			{
				n.res[elem.first][Res::GOLD] += h->valOfBonuses(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ESTATES)); //estates

				for (int k = 0; k < GameConstants::RESOURCE_QUANTITY; k++)
				{
					n.res[elem.first][k] += h->valOfBonuses(Bonus::GENERATE_RESOURCE, k);
				}
			}
		}
	}
	for(CGTownInstance *t : gs->map->towns)
	{
		PlayerColor player = t->tempOwner;
		handleTownEvents(t, n);
		if(newWeek) //first day of week
		{
			if(t->hasBuilt(BuildingID::PORTAL_OF_SUMMON, ETownType::DUNGEON))
				setPortalDwelling(t, true, (n.specialWeek == NewTurn::PLAGUE ? true : false)); //set creatures for Portal of Summoning

			if(!firstTurn)
				if (t->hasBuilt(BuildingID::TREASURY, ETownType::RAMPART) && player < PlayerColor::PLAYER_LIMIT)
						n.res[player][Res::GOLD] += hadGold.at(player)/10; //give 10% of starting gold

			if (!vstd::contains(n.cres, t->id))
			{
				n.cres[t->id].tid = t->id;
				n.cres[t->id].creatures = t->creatures;
			}
			auto & sac = n.cres.at(t->id);

			for (int k=0; k < GameConstants::CREATURES_PER_TOWN; k++) //creature growths
			{
				if (!t->creatures.at(k).second.empty()) // there are creatures at this level
				{
					ui32 &availableCount = sac.creatures.at(k).first;
					const CCreature *cre = VLC->creh->creatures.at(t->creatures.at(k).second.back());

					if (n.specialWeek == NewTurn::PLAGUE)
						availableCount = t->creatures.at(k).first / 2; //halve their number, no growth
					else
					{
						if(firstTurn) //first day of game: use only basic growths
							availableCount = cre->growth;
						else
							availableCount += t->creatureGrowth(k);

						//Deity of fire week - upgrade both imps and upgrades
						if (n.specialWeek == NewTurn::DEITYOFFIRE && vstd::contains(t->creatures.at(k).second, n.creatureid))
							availableCount += 15;

						if( cre->idNumber == n.creatureid ) //bonus week, effect applies only to identical creatures
						{
							if(n.specialWeek == NewTurn::DOUBLE_GROWTH)
								availableCount *= 2;
							else if(n.specialWeek == NewTurn::BONUS_GROWTH)
								availableCount += 5;
						}
					}
				}
			}
		}
		if(!firstTurn  &&  player < PlayerColor::PLAYER_LIMIT)//not the first day and town not neutral
		{
			n.res[player] = n.res[player] + t->dailyIncome();
		}
		if(t->hasBuilt(BuildingID::GRAIL, ETownType::TOWER))
		{
			// Skyship, probably easier to handle same as Veil of darkness
			//do it every new day after veils apply
			if (player != PlayerColor::NEUTRAL) //do not reveal fow for neutral player
			{
				FoWChange fw;
				fw.mode = 1;
				fw.player = player;
				// find all hidden tiles
				const auto & fow = gs->getPlayerTeam(player)->fogOfWarMap;
				for (size_t i=0; i<fow.size(); i++)
					for (size_t j=0; j<fow.at(i).size(); j++)
						for (size_t k=0; k<fow.at(i).at(j).size(); k++)
							if (!fow.at(i).at(j).at(k))
								fw.tiles.insert(int3(i,j,k));

				sendAndApply (&fw);
			}
		}
		if (t->hasBonusOfType (Bonus::DARKNESS))
		{
			for (auto & player : gameState()->players)
			{
				if (getPlayerStatus(player.first) == EPlayerStatus::INGAME &&
					getPlayerRelations(player.first, t->tempOwner) == PlayerRelations::ENEMIES)
					changeFogOfWar(t->visitablePos(), t->getBonusLocalFirst(Selector::type(Bonus::DARKNESS))->val, player.first, true);
			}
		}
	}

	if(newMonth)
	{
		SetAvailableArtifacts saa;
		saa.id = -1;
		pickAllowedArtsSet(saa.arts);
		sendAndApply(&saa);
	}
	sendAndApply(&n);

	if(newWeek)
	{
		//spawn wandering monsters
		if (newMonth && (n.specialWeek == NewTurn::DOUBLE_GROWTH || n.specialWeek == NewTurn::DEITYOFFIRE))
		{
			spawnWanderingMonsters(n.creatureid);
		}

		//new week info popup
		if(!firstTurn)
		{
			InfoWindow iw;
			switch (n.specialWeek)
			{
				case NewTurn::DOUBLE_GROWTH:
					iw.text.addTxt(MetaString::ARRAY_TXT, 131);
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, n.creatureid);
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, n.creatureid);
					break;
				case NewTurn::PLAGUE:
					iw.text.addTxt(MetaString::ARRAY_TXT, 132);
					break;
				case NewTurn::BONUS_GROWTH:
					iw.text.addTxt(MetaString::ARRAY_TXT, 134);
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, n.creatureid);
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, n.creatureid);
					break;
				case NewTurn::DEITYOFFIRE:
					iw.text.addTxt(MetaString::ARRAY_TXT, 135);
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, 42); //%s imp
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, 42); //%s imp
					iw.text.addReplacement2(15);							//%+d 15
					iw.text.addReplacement(MetaString::CRE_SING_NAMES, 43); //%s familiar
					iw.text.addReplacement2(15);							//%+d 15
					break;
				default:
					if (newMonth)
					{
						iw.text.addTxt(MetaString::ARRAY_TXT, (130));
						iw.text.addReplacement(MetaString::ARRAY_TXT, gs->getRandomGenerator().nextInt(32, 41));
					}
					else
					{
						iw.text.addTxt(MetaString::ARRAY_TXT, (133));
						iw.text.addReplacement(MetaString::ARRAY_TXT, gs->getRandomGenerator().nextInt(43, 57));
					}
			}
			for (auto & elem : gs->players)
			{
				iw.player = elem.first;
				sendAndApply(&iw);
			}
		}
	}

	logGlobal->traceStream() << "Info about turn " << n.day << "has been sent!";
	handleTimeEvents();
	//call objects
	for(auto & elem : gs->map->objects)
	{
		if(elem)
			elem->newTurn();
	}

	//count days without town for all players, regardless of their turn order
	for (auto &p : gs->players)
	{
		PlayerState & playerState = p.second;
		if (playerState.status == EPlayerStatus::INGAME)
		{
			if (playerState.towns.empty())
			{
				if (playerState.daysWithoutCastle)
					++(*playerState.daysWithoutCastle);
				else playerState.daysWithoutCastle = 0;
			}
			else
			{
				playerState.daysWithoutCastle = boost::none;
			}
		}
	}

	synchronizeArtifactHandlerLists(); //new day events may have changed them. TODO better of managing that
}
void CGameHandler::run(bool resume)
{
	LOG_TRACE_PARAMS(logGlobal, "resume=%d", resume);

	using namespace boost::posix_time;
	for(CConnection *cc : conns)
	{
		if(!resume)
		{
			(*cc) << gs->initialOpts; // gs->scenarioOps
		}

		std::set<PlayerColor> players;
		(*cc) >> players; //how many players will be handled at that client

		std::stringstream sbuffer;
		sbuffer << "Connection " << cc->connectionID << " will handle " << players.size() << " player: ";
		for(PlayerColor color : players)
		{
			sbuffer << color << " ";
			{
				boost::unique_lock<boost::recursive_mutex> lock(gsm);
				connections[color] = cc;
			}
		}
		logGlobal->infoStream() << sbuffer.str();

		cc->addStdVecItems(gs);
		cc->enableStackSendingByID();
		cc->disableSmartPointerSerialization();
	}

	for(auto & elem : conns)
	{
		std::set<PlayerColor> pom;
		for(auto j = connections.cbegin(); j!=connections.cend();j++)
			if(j->second == elem)
				pom.insert(j->first);

		boost::thread(std::bind(&CGameHandler::handleConnection,this,pom,std::ref(*elem)));
	}

	if(gs->scenarioOps->mode == StartInfo::DUEL)
	{
		runBattle();
		end2 = true;


		while(conns.size() && (*conns.begin())->isOpen())
			boost::this_thread::sleep(boost::posix_time::milliseconds(5)); //give time client to close socket

		return;
	}

	auto playerTurnOrder = generatePlayerTurnOrder();

	while(!end2)
	{
		if(!resume) newTurn();

		std::list<PlayerColor>::iterator it;
		if(resume)
		{
			it = std::find(playerTurnOrder.begin(), playerTurnOrder.end(), gs->currentPlayer);
		}
		else
		{
			it = playerTurnOrder.begin();
		}

		resume = false;
		for (; it != playerTurnOrder.end(); it++)
		{
			auto playerColor = *it;

			PlayerState * playerState = &gs->players[playerColor]; //can't copy CBonusSystemNode by value
			if (playerState->status == EPlayerStatus::INGAME)
			{
				//if player runs out of time, he shouldn't get the turn (especially AI)
				checkVictoryLossConditionsForAll();

				if (gs->players[playerColor].status != EPlayerStatus::INGAME)
				{ //player lost at the beginning of his turn
					continue;
				}
				else //give normal turn
				{
					states.setFlag(playerColor, &PlayerStatus::makingTurn, true);

					YourTurn yt;
					yt.player = playerColor;
					//Change local daysWithoutCastle counter for local interface message //TODO: needed?
					yt.daysWithoutCastle = playerState->daysWithoutCastle;
					applyAndSend(&yt);

					//wait till turn is done
					boost::unique_lock<boost::mutex> lock(states.mx);
					while (states.players.at(playerColor).makingTurn && !end2)
					{
						static time_duration p = milliseconds(100);
						states.cv.timed_wait(lock, p);
					}
				}
			}
		}
		//additional check that game is not finished
		bool activePlayer = false;
		for(auto player : playerTurnOrder)
		{
			if(gs->players[player].status == EPlayerStatus::INGAME)
					activePlayer = true;
		}
		if(!activePlayer)
			end2 = true;
	}
	while(conns.size() && (*conns.begin())->isOpen())
		boost::this_thread::sleep(boost::posix_time::milliseconds(5)); //give time client to close socket
}

std::list<PlayerColor> CGameHandler::generatePlayerTurnOrder() const
{
	// Generate player turn order
	std::list<PlayerColor> playerTurnOrder;

	for(const auto & player : gs->players) // add human players first
	{
		if(player.second.human)
			playerTurnOrder.push_back(player.first);
	}
	for(const auto & player : gs->players) // then add non-human players
	{
		if(!player.second.human)
			playerTurnOrder.push_back(player.first);
	}
	return playerTurnOrder;
}

void CGameHandler::setupBattle( int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town )
{
	battleResult.set(nullptr);

	//send info about battles
	BattleStart bs;
	bs.info = gs->setupBattle(tile, armies, heroes, creatureBank,	town);
	sendAndApply(&bs);
}

void CGameHandler::checkBattleStateChanges()
{
	//check if drawbridge state need to be changes
	if(battleGetSiegeLevel() > 0)
		updateGateState();

	//check if battle ended
	if(auto result = battleIsFinished())
	{
		setBattleResult(BattleResult::NORMAL, *result);
	}
}

void CGameHandler::giveSpells(const CGTownInstance *t, const CGHeroInstance *h)
{
	if(!h->hasSpellbook())
		return; //hero hasn't spellbook
	ChangeSpells cs;
	cs.hid = h->id;
	cs.learn = true;
	if(t->hasBuilt(BuildingID::GRAIL, ETownType::CONFLUX) && t->hasBuilt(BuildingID::MAGES_GUILD_1))
	{
		// Aurora Borealis give spells of all levels even if only level 1 mages guild built
		for(int i = 0; i < h->getSecSkillLevel(SecondarySkill::WISDOM)+2; i++)
		{
			std::vector<SpellID> spells;
			getAllowedSpells(spells, i+1);
			for(auto & spell : spells)
				cs.spells.insert(spell);
		}
	}
	else
	{
		for(int i = 0; i < std::min(t->mageGuildLevel(), h->getSecSkillLevel(SecondarySkill::WISDOM)+2); i++)
		{
			for(int j = 0; j < t->spellsAtLevel(i+1, true) && j < t->spells.at(i).size(); j++)
			{
				if(!vstd::contains(h->spells, t->spells.at(i).at(j)))
					cs.spells.insert(t->spells.at(i).at(j));
			}
		}
	}
	if(!cs.spells.empty())
		sendAndApply(&cs);
}

void CGameHandler::setBlockVis(ObjectInstanceID objid, bool bv)
{
	SetObjectProperty sop(objid,2,bv);
	sendAndApply(&sop);
}

bool CGameHandler::removeObject( const CGObjectInstance * obj )
{
	if(!obj || !getObj(obj->id))
	{
		logGlobal->errorStream() << "Something wrong, that object already has been removed or hasn't existed!";
		return false;
	}

	RemoveObject ro;
	ro.id = obj->id;
	sendAndApply(&ro);

	checkVictoryLossConditionsForAll(); //eg if monster escaped (removing objs after battle is done dircetly by endBattle, not this function)
	return true;
}

void CGameHandler::setAmount(ObjectInstanceID objid, ui32 val)
{
	SetObjectProperty sop(objid,3,val);
	sendAndApply(&sop);
}

bool CGameHandler::moveHero( ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit, PlayerColor asker /*= 255*/ )
{
	const CGHeroInstance *h = getHero(hid);

	if(!h  || (asker != PlayerColor::NEUTRAL && (teleporting  ||   h->getOwner() != gs->currentPlayer)) //not turn of that hero or player can't simply teleport hero (at least not with this function)
	  )
	{
		logGlobal->errorStream() << "Illegal call to move hero!";
		return false;
	}

	logGlobal->traceStream() << "Player " << asker << " wants to move hero "<< hid.getNum() << " from "<< h->pos << " to " << dst;
	const int3 hmpos = CGHeroInstance::convertPosition(dst, false);

	if(!gs->map->isInTheMap(hmpos))
	{
		logGlobal->errorStream() << "Destination tile is outside the map!";
		return false;
	}

	const TerrainTile t = *gs->getTile(hmpos);
	const int3 guardPos = gs->guardingCreaturePosition(hmpos);

	const bool embarking = !h->boat && !t.visitableObjects.empty() && t.visitableObjects.back()->ID == Obj::BOAT;
	const bool disembarking = h->boat && t.terType != ETerrainType::WATER && !t.blocked;

	//result structure for start - movement failed, no move points used
	TryMoveHero tmh;
	tmh.id = hid;
	tmh.start = h->pos;
	tmh.end = dst;
	tmh.result = TryMoveHero::FAILED;
	tmh.movePoints = h->movement;

	//check if destination tile is available
	auto ti = new TurnInfo(h);
	const bool canFly = ti->hasBonusOfType(Bonus::FLYING_MOVEMENT);
	const bool canWalkOnSea = ti->hasBonusOfType(Bonus::WATER_WALKING);
	const int cost = CPathfinderHelper::getMovementCost(h, h->getPosition(), hmpos, nullptr, nullptr, h->movement, ti);

	//it's a rock or blocked and not visitable tile
	//OR hero is on land and dest is water and (there is not present only one object - boat)
	if(((t.terType == ETerrainType::ROCK  ||  (t.blocked && !t.visitable && !canFly))
			&& complain("Cannot move hero, destination tile is blocked!"))
		|| ((!h->boat && !canWalkOnSea && !canFly && t.terType == ETerrainType::WATER && (t.visitableObjects.size() < 1 ||  (t.visitableObjects.back()->ID != Obj::BOAT && t.visitableObjects.back()->ID != Obj::HERO)))  //hero is not on boat/water walking and dst water tile doesn't contain boat/hero (objs visitable from land) -> we test back cause boat may be on top of another object (#276)
			&& complain("Cannot move hero, destination tile is on water!"))
		|| ((h->boat && t.terType != ETerrainType::WATER && t.blocked)
			&& complain("Cannot disembark hero, tile is blocked!"))
		|| ( (distance(h->pos, dst) >= 1.5 && !teleporting)
			&& complain("Tiles are not neighboring!"))
		|| ( (h->inTownGarrison)
			&& complain("Can not move garrisoned hero!"))
		|| ((h->movement < cost  &&  dst != h->pos  &&  !teleporting)
			&& complain("Hero doesn't have any movement points left!"))
		|| ((transit && !canFly && !CGTeleport::isTeleport(t.topVisitableObj()))
			&& complain("Hero cannot transit over this tile!"))
		/*|| (states.checkFlag(h->tempOwner, &PlayerStatus::engagedIntoBattle)
			&& complain("Cannot move hero during the battle"))*/)
	{
		//send info about movement failure
		sendAndApply(&tmh);
		return false;
	}

	//several generic blocks of code

	// should be called if hero changes tile but before applying TryMoveHero package
	auto leaveTile = [&]()
	{
		for(CGObjectInstance *obj : gs->map->getTile(int3(h->pos.x-1, h->pos.y, h->pos.z)).visitableObjects)
		{
			obj->onHeroLeave(h);
		}
		this->getTilesInRange(tmh.fowRevealed, h->getSightCenter()+(tmh.end-tmh.start), h->getSightRadius(), h->tempOwner, 1);
	};

	auto doMove = [&](TryMoveHero::EResult result, EGuardLook lookForGuards,
								EVisitDest visitDest, ELEaveTile leavingTile) -> bool
	{
		LOG_TRACE_PARAMS(logGlobal, "Hero %s starts movement from %s to %s", h->name % tmh.start % tmh.end);

		auto moveQuery = std::make_shared<CHeroMovementQuery>(tmh, h);
		queries.addQuery(moveQuery);

		if(leavingTile == LEAVING_TILE)
			leaveTile();

		tmh.result = result;
		sendAndApply(&tmh);

		if (visitDest == VISIT_DEST && t.topVisitableObj() && t.topVisitableObj()->id == h->id)
		{ // Hero should be always able to visit any object he staying on even if there guards around
			visitObjectOnTile(t, h);
		}
		else if(lookForGuards == CHECK_FOR_GUARDS && this->isInTheMap(guardPos))
		{
			tmh.attackedFrom = guardPos;

			const TerrainTile &guardTile = *gs->getTile(guardPos);
			objectVisited(guardTile.visitableObjects.back(), h);

			moveQuery->visitDestAfterVictory = visitDest==VISIT_DEST;
		}
		else if(visitDest == VISIT_DEST)
		{
			visitObjectOnTile(t, h);
		}

		queries.popIfTop(moveQuery);
		logGlobal->traceStream() << "Hero " << h->name << " ends movement";
		return result != TryMoveHero::FAILED;
	};

	//interaction with blocking object (like resources)
	auto blockingVisit = [&]() -> bool
	{
		for(CGObjectInstance *obj : t.visitableObjects)
		{
			if(obj != h  &&  obj->blockVisit  &&  !obj->passableFor(h->tempOwner))
			{
				return doMove(TryMoveHero::BLOCKING_VISIT, this->IGNORE_GUARDS, VISIT_DEST, REMAINING_ON_TILE);
				//this-> is needed for MVS2010 to recognize scope (?)
			}
		}
		return false;
	};


	if(!transit && embarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movement, cost, false, ti);
		return doMove(TryMoveHero::EMBARK, IGNORE_GUARDS, DONT_VISIT_DEST, LEAVING_TILE);
		// In H3 embark ignore guards
	}

	if(disembarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movement, cost, true, ti);
		return doMove(TryMoveHero::DISEMBARK, CHECK_FOR_GUARDS, VISIT_DEST, LEAVING_TILE);
	}

	if(teleporting)
	{
		if(blockingVisit()) // e.g. hero on the other side of teleporter
			return true;

		doMove(TryMoveHero::TELEPORTATION, IGNORE_GUARDS, DONT_VISIT_DEST, LEAVING_TILE);

		// visit town for town portal \ castle gates
		// do not use generic visitObjectOnTile to avoid double-teleporting
		// if this moveHero call was triggered by teleporter
		if (!t.visitableObjects.empty())
		{
			if (CGTownInstance * town = dynamic_cast<CGTownInstance *>(t.visitableObjects.back()))
				town->onHeroVisit(h);
		}

		return true;
	}

	//still here? it is standard movement!
	{
		tmh.movePoints = h->movement >= cost
						? h->movement - cost
						: 0;

		EGuardLook lookForGuards = CHECK_FOR_GUARDS;
		EVisitDest visitDest = VISIT_DEST;
		if(transit)
		{
			if(CGTeleport::isTeleport(t.topVisitableObj()))
				visitDest = DONT_VISIT_DEST;

			if(canFly)
			{
				lookForGuards = IGNORE_GUARDS;
				visitDest = DONT_VISIT_DEST;
			}
		}
		else if(blockingVisit())
			return true;

		doMove(TryMoveHero::SUCCESS, lookForGuards, visitDest, LEAVING_TILE);
		return true;
	}
}

bool CGameHandler::teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker/* = 255*/)
{
	const CGHeroInstance *h = getHero(hid);
	const CGTownInstance *t = getTown(dstid);

	if ( !h || !t || h->getOwner() != gs->currentPlayer )
		logGlobal->errorStream() << "Invalid call to teleportHero!";

	const CGTownInstance *from = h->visitedTown;
	if(((h->getOwner() != t->getOwner())
		&& complain("Cannot teleport hero to another player"))
	|| ((!from || !from->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
		&& complain("Hero must be in town with Castle gate for teleporting"))
	|| (!t->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO)
		&& complain("Cannot teleport hero to town without Castle gate in it")))
			return false;
	int3 pos = t->visitablePos();
	pos += h->getVisitableOffset();
	moveHero(hid,pos,1);
	return true;
}

void CGameHandler::setOwner(const CGObjectInstance * obj, PlayerColor owner)
{
	PlayerColor oldOwner = getOwner(obj->id);
	SetObjectProperty sop(obj->id, 1, owner.getNum());
	sendAndApply(&sop);

	std::set<PlayerColor> playerColors = {owner, oldOwner};
	checkVictoryLossConditions(playerColors);

	if(dynamic_cast<const CGTownInstance *>(obj)) //town captured
	{
		if (owner < PlayerColor::PLAYER_LIMIT) //new owner is real player
		{
			const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(obj);
			if (town->hasBuilt(BuildingID::PORTAL_OF_SUMMON, ETownType::DUNGEON))
				setPortalDwelling(town, true, false);

			gs->getPlayer(owner)->daysWithoutCastle = boost::none; // reset counter
		}

		if (oldOwner < PlayerColor::PLAYER_LIMIT) //old owner is real player
		{
			if (gs->getPlayer(oldOwner)->towns.empty())//previous player lost last last town
			{
				InfoWindow iw;
				iw.player = oldOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 6); //%s, you have lost your last town. If you do not conquer another town in the next week, you will be eliminated.
				iw.text.addReplacement(MetaString::COLOR, oldOwner.getNum());
				sendAndApply(&iw);
			}
		}
	}

	const PlayerState * p = gs->getPlayer(owner);

	if((obj->ID == Obj::CREATURE_GENERATOR1 || obj->ID == Obj::CREATURE_GENERATOR4 ) && p && p->dwellings.size()==1)//first dwelling captured
	{
		for(const CGTownInstance *t : gs->getPlayer(owner)->towns)
		{
			if (t->hasBuilt(BuildingID::PORTAL_OF_SUMMON, ETownType::DUNGEON))
				setPortalDwelling(t);//set initial creatures for all portals of summoning
		}
	}
}

void CGameHandler::showBlockingDialog( BlockingDialog *iw )
{
	auto dialogQuery = std::make_shared<CBlockingDialogQuery>(*iw);
	queries.addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(iw);
}

void CGameHandler::showTeleportDialog( TeleportDialog *iw )
{
	auto dialogQuery = std::make_shared<CTeleportDialogQuery>(*iw);
	queries.addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(iw);
}

void CGameHandler::giveResource(PlayerColor player, Res::ERes which, int val) //TODO: cap according to Bersy's suggestion
{
	if(!val) return; //don't waste time on empty call
	SetResource sr;
	sr.player = player;
	sr.resid = which;
	sr.val = gs->players.find(player)->second.resources.at(which) + val;
	sendAndApply(&sr);
}

void CGameHandler::giveResources(PlayerColor player, TResources resources)
{
	for(TResources::nziterator i(resources); i.valid(); i++)
		giveResource(player, i->resType, i->resVal);
}

void CGameHandler::giveCreatures(const CArmedInstance *obj, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove)
{
	COMPLAIN_RET_IF(!creatures.stacksCount(), "Strange, giveCreatures called without args!");
	COMPLAIN_RET_IF(obj->stacksCount(), "Cannot give creatures from not-cleared object!");
	COMPLAIN_RET_IF(creatures.stacksCount() > GameConstants::ARMY_SIZE, "Too many stacks to give!");

	//first we move creatures to give to make them army of object-source
	for (auto & elem : creatures.Slots())
	{
		addToSlot(StackLocation(obj, obj->getSlotFor(elem.second->type)), elem.second->type, elem.second->count);
	}

	tryJoiningArmy(obj, h, remove, true);
}

void CGameHandler::takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures)
{
	std::vector<CStackBasicDescriptor> cres = creatures;
	if (cres.size() <= 0)
		return;
	const CArmedInstance* obj = static_cast<const CArmedInstance*>(getObj(objid));

	for(CStackBasicDescriptor &sbd : cres)
	{
		TQuantity collected = 0;
		while(collected < sbd.count)
		{
			bool foundSth = false;
			for(auto i = obj->Slots().begin(); i != obj->Slots().end(); i++)
			{
				if(i->second->type == sbd.type)
				{
					TQuantity take = std::min(sbd.count - collected, i->second->count); //collect as much cres as we can
					changeStackCount(StackLocation(obj, i->first), -take, false);
					collected += take;
					foundSth = true;
					break;
				}
			}

			if(!foundSth) //we went through the whole loop and haven't found appropriate cres
			{
				complain("Unexpected failure during taking creatures!");
				return;
			}
		}
	}
}

void CGameHandler::showCompInfo(ShowInInfobox * comp)
{
	sendToAllClients(comp);
}
void CGameHandler::heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)
{
	HeroVisitCastle vc;
	vc.hid = hero->id;
	vc.tid = obj->id;
	vc.flags |= 1;
	sendAndApply(&vc);
	vistiCastleObjects (obj, hero);
	giveSpells (obj, hero);

	checkVictoryLossConditionsForPlayer(hero->tempOwner); //transported artifact?
}

void CGameHandler::vistiCastleObjects (const CGTownInstance *t, const CGHeroInstance *h)
{
	std::vector<CGTownBuilding*>::const_iterator i;
	for (i = t->bonusingBuildings.begin(); i != t->bonusingBuildings.end(); i++)
		(*i)->onHeroVisit (h);
}

void CGameHandler::stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)
{
	HeroVisitCastle vc;
	vc.hid = hero->id;
	vc.tid = obj->id;
	sendAndApply(&vc);
}

void CGameHandler::removeArtifact(const ArtifactLocation &al)
{
	EraseArtifact ea;
	ea.al = al;
	sendAndApply(&ea);
}
void CGameHandler::startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile,
								const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank,
								const CGTownInstance *town) //use hero=nullptr for no hero
{
	engageIntoBattle(army1->tempOwner);
	engageIntoBattle(army2->tempOwner);

	static const CArmedInstance *armies[2];
	armies[0] = army1;
	armies[1] = army2;
	static const CGHeroInstance*heroes[2];
	heroes[0] = hero1;
	heroes[1] = hero2;


	setupBattle(tile, armies, heroes, creatureBank, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces

	auto battleQuery = std::make_shared<CBattleQuery>(gs->curB);
	queries.addQuery(battleQuery);

	boost::thread(&CGameHandler::runBattle, this);
}

void CGameHandler::startBattleI( const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank )
{
	startBattlePrimary(army1, army2, tile,
		army1->ID == Obj::HERO ? static_cast<const CGHeroInstance*>(army1) : nullptr,
		army2->ID == Obj::HERO ? static_cast<const CGHeroInstance*>(army2) : nullptr,
		creatureBank);
}

void CGameHandler::startBattleI( const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank)
{
	startBattleI(army1, army2, army2->visitablePos(), creatureBank);
}

void CGameHandler::changeSpells( const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells )
{
	ChangeSpells cs;
	cs.hid = hero->id;
	cs.spells = spells;
	cs.learn = give;
	sendAndApply(&cs);
}

void CGameHandler::sendMessageTo( CConnection &c, const std::string &message )
{
	SystemMessage sm;
	sm.text = message;
	boost::unique_lock<boost::mutex> lock(*c.wmx);
	c << &sm;
}

void CGameHandler::giveHeroBonus( GiveBonus * bonus )
{
	sendAndApply(bonus);
}

void CGameHandler::setMovePoints( SetMovePoints * smp )
{
	sendAndApply(smp);
}

void CGameHandler::setManaPoints( ObjectInstanceID hid, int val )
{
	SetMana sm;
	sm.hid = hid;
	sm.val = val;
	sm.absolute = true;
	sendAndApply(&sm);
}

void CGameHandler::giveHero( ObjectInstanceID id, PlayerColor player )
{
	GiveHero gh;
	gh.id = id;
	gh.player = player;
	sendAndApply(&gh);
}

void CGameHandler::changeObjPos( ObjectInstanceID objid, int3 newPos, ui8 flags )
{
	ChangeObjPos cop;
	cop.objid = objid;
	cop.nPos = newPos;
	cop.flags = flags;
	sendAndApply(&cop);
}

void CGameHandler::useScholarSkill(ObjectInstanceID fromHero, ObjectInstanceID toHero)
{
	const CGHeroInstance * h1 = getHero(fromHero);
	const CGHeroInstance * h2 = getHero(toHero);

	if ( h1->getSecSkillLevel(SecondarySkill::SCHOLAR) < h2->getSecSkillLevel(SecondarySkill::SCHOLAR) )
	{
		std::swap (h1,h2);//1st hero need to have higher scholar level for correct message
		std::swap(fromHero, toHero);
	}

	int ScholarLevel = h1->getSecSkillLevel(SecondarySkill::SCHOLAR);//heroes can trade up to this level
	if (!ScholarLevel || !h1->hasSpellbook() || !h2->hasSpellbook() )
		return;//no scholar skill or no spellbook

	int h1Lvl = std::min(ScholarLevel+1, h1->getSecSkillLevel(SecondarySkill::WISDOM)+2),
	    h2Lvl = std::min(ScholarLevel+1, h2->getSecSkillLevel(SecondarySkill::WISDOM)+2);//heroes can receive this levels

	ChangeSpells cs1;
	cs1.learn = true;
	cs1.hid = toHero;//giving spells to first hero
	for(auto it : h1->spells)
		if ( h2Lvl >= it.toSpell()->level && !vstd::contains(h2->spells, it))//hero can learn it and don't have it yet
			cs1.spells.insert(it);//spell to learn

	ChangeSpells cs2;
	cs2.learn = true;
	cs2.hid = fromHero;

	for(auto it : h2->spells)
		if ( h1Lvl >= it.toSpell()->level && !vstd::contains(h1->spells, it))
			cs2.spells.insert(it);

	if (!cs1.spells.empty() || !cs2.spells.empty())//create a message
	{
		InfoWindow iw;
		iw.player = h1->tempOwner;
		iw.components.push_back(Component(Component::SEC_SKILL, 18, ScholarLevel, 0));

		iw.text.addTxt(MetaString::GENERAL_TXT, 139);//"%s, who has studied magic extensively,
		iw.text.addReplacement(h1->name);

		if (!cs2.spells.empty())//if found new spell - apply
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 140);//learns
			int size = cs2.spells.size();
			for(auto it : cs2.spells)
			{
				iw.components.push_back(Component(Component::SPELL, it, 1, 0));
				iw.text.addTxt(MetaString::SPELL_NAME, it.toEnum());
				switch (size--)
				{
					case 2:	iw.text.addTxt(MetaString::GENERAL_TXT, 141);
					case 1:	break;
					default:	iw.text << ", ";
				}
			}
			iw.text.addTxt(MetaString::GENERAL_TXT, 142);//from %s
			iw.text.addReplacement(h2->name);
			sendAndApply(&cs2);
		}

		if (!cs1.spells.empty() && !cs2.spells.empty() )
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 141);//and
		}

		if (!cs1.spells.empty())
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 147);//teaches
			int size = cs1.spells.size();
			for(auto it : cs1.spells)
			{
				iw.components.push_back(Component(Component::SPELL, it, 1, 0));
				iw.text.addTxt(MetaString::SPELL_NAME, it.toEnum());
				switch (size--)
				{
					case 2:	iw.text.addTxt(MetaString::GENERAL_TXT, 141);
					case 1:	break;
					default:	iw.text << ", ";
				}			}
			iw.text.addTxt(MetaString::GENERAL_TXT, 148);//from %s
			iw.text.addReplacement(h2->name);
			sendAndApply(&cs1);
		}
		sendAndApply(&iw);
	}
}

void CGameHandler::heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)
{
	auto h1 = getHero(hero1), h2 = getHero(hero2);

	if( gameState()->getPlayerRelations(h1->getOwner(), h2->getOwner()))
	{
		auto exchange = std::make_shared<CGarrisonDialogQuery>(h1, h2);
		ExchangeDialog hex;
		hex.queryID = exchange->queryID;
		hex.heroes[0] = getHero(hero1);
		hex.heroes[1] = getHero(hero2);
		sendAndApply(&hex);
		useScholarSkill(hero1,hero2);
		queries.addQuery(exchange);
	}
}

void CGameHandler::sendToAllClients( CPackForClient * info )
{
	logGlobal->traceStream() << "Sending to all clients a package of type " << typeid(*info).name();
	for(auto & elem : conns)
	{
		boost::unique_lock<boost::mutex> lock(*(elem)->wmx);
		*elem << info;
	}
}

void CGameHandler::sendAndApply(CPackForClient * info)
{
	sendToAllClients(info);
	gs->apply(info);
}

void CGameHandler::applyAndSend(CPackForClient * info)
{
	gs->apply(info);
	sendToAllClients(info);
}

void CGameHandler::sendAndApply(CGarrisonOperationPack * info)
{
	sendAndApply(static_cast<CPackForClient*>(info));
	checkVictoryLossConditionsForAll();
}

void CGameHandler::sendAndApply( SetResource * info )
{
	sendAndApply(static_cast<CPackForClient*>(info));
	checkVictoryLossConditionsForPlayer(info->player);
}

void CGameHandler::sendAndApply( SetResources * info )
{
	sendAndApply(static_cast<CPackForClient*>(info));
	checkVictoryLossConditionsForPlayer(info->player);
}

void CGameHandler::sendAndApply( NewStructures * info )
{
	sendAndApply(static_cast<CPackForClient*>(info));
	checkVictoryLossConditionsForPlayer(getTown(info->tid)->tempOwner);
}

void CGameHandler::save(const std::string & filename )
{
	logGlobal->infoStream() << "Saving to " << filename;
	const auto stem	= FileInfo::GetPathStem(filename);
	const auto savefname = stem.to_string() + ".vsgm1";
	CResourceHandler::get("local")->createResource(savefname);

	{
		logGlobal->infoStream() << "Ordering clients to serialize...";
		SaveGame sg(savefname);
		sendToAllClients(&sg);
	}

	try
	{
// 		{
// 			logGlobal->infoStream() << "Serializing game info...";
// 			CSaveFile save(CResourceHandler::get("local")->getResourceName(ResourceID(info.getStem(), EResType::LIB_SAVEGAME)));
// // 			char hlp[8] = "VCMISVG";
// // 			save << hlp;
// 			saveCommonState(save);
// 		}

		{
			CSaveFile save(*CResourceHandler::get("local")->getResourceName(ResourceID(stem.to_string(), EResType::SERVER_SAVEGAME)));
			saveCommonState(save);
			logGlobal->infoStream() << "Saving server state";
			save << *this;
		}
		logGlobal->infoStream() << "Game has been successfully saved!";
	}
	catch(std::exception &e)
	{
		logGlobal->errorStream() << "Failed to save game: " << e.what();
	}
}

void CGameHandler::close()
{
	logGlobal->infoStream() << "We have been requested to close.";

	if(gs->initialOpts->mode == StartInfo::DUEL)
	{
		exit(0);
	}

	//for(CConnection *cc : conns)
	//	if(cc && cc->socket && cc->socket->is_open())
	//		cc->socket->close();
	//exit(0);
}

bool CGameHandler::arrangeStacks( ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player )
{
	const CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->getObjInstance(id1)),
		*s2 = static_cast<CArmedInstance*>(gs->getObjInstance(id2));
	const CCreatureSet &S1 = *s1, &S2 = *s2;
	StackLocation sl1(s1, p1), sl2(s2, p2);
	if(!sl1.slot.validSlot()  ||  !sl2.slot.validSlot())
	{
		complain("Invalid slot accessed!");
		return false;
	}

	if(!isAllowedExchange(id1,id2))
	{
		complain("Cannot exchange stacks between these two objects!\n");
		return false;
	}

	if(what==1) //swap
	{
		if ( ((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s1->getStackCount(p1))
		  || ((s2->tempOwner != player && s2->tempOwner != PlayerColor::UNFLAGGABLE) && s2->getStackCount(p2)))
		{
			complain("Can't take troops from another player!");
			return false;
		}

		if (sl1.army == sl2.army && sl1.slot == sl2.slot)
		{
			complain("Cannot swap stacks - slots are the same!");
			return false;
		}

		swapStacks(sl1, sl2);
	}
	else if(what==2)//merge
	{
		if (( s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot merge different creatures stacks!"))
		|| (((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s2->getStackCount(p2)) && complain("Can't take troops from another player!")))
			return false;

		moveStack(sl1, sl2);
	}
	else if(what==3) //split
	{
		const int countToMove = val - s2->getStackCount(p2);
		const int countLeftOnSrc = s1->getStackCount(p1) - countToMove;

		if (   (s1->tempOwner != player && countLeftOnSrc < s1->getStackCount(p1) )
			|| (s2->tempOwner != player && val < s2->getStackCount(p2) ) )
		{
			complain("Can't move troops of another player!");
			return false;
		}

		//general conditions checking
		if((!vstd::contains(S1.stacks,p1) && complain("no creatures to split"))
			|| (val<1  && complain("no creatures to split"))  )
		{
			return false;
		}


		if(vstd::contains(S2.stacks,p2))	 //dest. slot not free - it must be "rebalancing"...
		{
			int total = s1->getStackCount(p1) + s2->getStackCount(p2);
			if( (total < val   &&   complain("Cannot split that stack, not enough creatures!"))
				|| (s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot rebalance different creatures stacks!"))
			)
			{
				return false;
			}

			moveStack(sl1, sl2, countToMove);
			//S2.slots[p2]->count = val;
			//S1.slots[p1]->count = total - val;
		}
		else //split one stack to the two
		{
			if(s1->getStackCount(p1) < val)//not enough creatures
			{
				complain("Cannot split that stack, not enough creatures!");
				return false;
			}


			moveStack(sl1, sl2, val);
		}

	}
	return true;
}

PlayerColor CGameHandler::getPlayerAt( CConnection *c ) const
{
	std::set<PlayerColor> all;
	for(auto i=connections.cbegin(); i!=connections.cend(); i++)
		if(i->second == c)
			all.insert(i->first);

	switch(all.size())
	{
	case 0:
		return PlayerColor::NEUTRAL;
	case 1:
		return *all.begin();
	default:
		{
			//if we have more than one player at this connection, try to pick active one
			if(vstd::contains(all, gs->currentPlayer))
				return gs->currentPlayer;
			else
				return PlayerColor::CANNOT_DETERMINE; //cannot say which player is it
		}
	}
}

bool CGameHandler::disbandCreature( ObjectInstanceID id, SlotID pos )
{
	CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->getObjInstance(id));
	if(!vstd::contains(s1->stacks,pos))
	{
		complain("Illegal call to disbandCreature - no such stack in army!");
		return false;
	}

	eraseStack(StackLocation(s1, pos));
	return true;
}

bool CGameHandler::buildStructure( ObjectInstanceID tid, BuildingID requestedID, bool force /*=false*/ )
{
	const CGTownInstance * t = getTown(tid);
	if(!t)
		COMPLAIN_RETF("No such town (ID=%s)!", tid);
	if(!t->town->buildings.count(requestedID))
		COMPLAIN_RETF("Town of faction %s does not have info about building ID=%s!", t->town->faction->name % tid);
	if (t->hasBuilt(requestedID))
		COMPLAIN_RETF("Building %s is already built in %s", t->town->buildings.at(requestedID)->Name() % t->name);

	const CBuilding * requestedBuilding = t->town->buildings.at(requestedID);

	//Vector with future list of built building and buildings in auto-mode that are not yet built.
	std::vector<const CBuilding*> remainingAutoBuildings;
	std::set<BuildingID> buildingsThatWillBe;

	//Check validity of request
	if(!force)
	{
		switch (requestedBuilding->mode)
		{
		case CBuilding::BUILD_NORMAL :
			if (gs->canBuildStructure(t, requestedID) != EBuildingState::ALLOWED)
				COMPLAIN_RET("Cannot build that building!");
			break;

		case CBuilding::BUILD_AUTO   :
		case CBuilding::BUILD_SPECIAL:
			COMPLAIN_RET("This building can not be constructed normally!");

		case CBuilding::BUILD_GRAIL  :
			if(requestedBuilding->mode == CBuilding::BUILD_GRAIL) //needs grail
			{
				if(!t->visitingHero || !t->visitingHero->hasArt(ArtifactID::GRAIL))
					COMPLAIN_RET("Cannot build this without grail!")
				else
					removeArtifact(ArtifactLocation(t->visitingHero, t->visitingHero->getArtPos(ArtifactID::GRAIL, false)));
			}
			break;
		}
	}

	//Performs stuff that has to be done before new building is built
	auto processBeforeBuiltStructure = [t, this](const BuildingID buildingID)
	{
		if(buildingID >= BuildingID::DWELL_FIRST) //dwelling
		{
			int level = (buildingID - BuildingID::DWELL_FIRST) % GameConstants::CREATURES_PER_TOWN;
			int upgradeNumber = (buildingID - BuildingID::DWELL_FIRST) / GameConstants::CREATURES_PER_TOWN;

			if (upgradeNumber >= t->town->creatures.at(level).size())
			{
				complain(boost::str(boost::format("Error ecountered when building dwelling (bid=%s):"
													"no creature found (upgrade number %d, level %d!")
												% buildingID % upgradeNumber % level));
				return;
			}

			CCreature * crea = VLC->creh->creatures.at(t->town->creatures.at(level).at(upgradeNumber));

			SetAvailableCreatures ssi;
			ssi.tid = t->id;
			ssi.creatures = t->creatures;
			if (ssi.creatures[level].second.empty()) // first creature in a dwelling
				ssi.creatures[level].first = crea->growth;
			ssi.creatures[level].second.push_back(crea->idNumber);
			sendAndApply(&ssi);
		}
		if ( t->subID == ETownType::DUNGEON && buildingID == BuildingID::PORTAL_OF_SUMMON )
		{
			setPortalDwelling(t);
		}
	};

	//Performs stuff that has to be done after new building is built
	auto processAfterBuiltStructure = [t, this](const BuildingID buildingID)
	{
		if(buildingID <= BuildingID::MAGES_GUILD_5 || //it's mage guild
			(t->subID == ETownType::TOWER && buildingID == BuildingID::LIBRARY) ||
			(t->subID == ETownType::CONFLUX && buildingID == BuildingID::GRAIL))
		{
			if(t->visitingHero)
				giveSpells(t,t->visitingHero);
			if(t->garrisonHero)
				giveSpells(t,t->garrisonHero);
		}
	};

	//Checks if all requirements will be met with expected building list "buildingsThatWillBe"
	auto areRequirementsFullfilled = [&](const BuildingID & buildID)
	{
		return buildingsThatWillBe.count(buildID);
	};

	//Init the vectors
	for(auto & build : t->town->buildings)
	{
		if(t->hasBuilt(build.first))
			buildingsThatWillBe.insert(build.first);
		else
		{
			if(build.second->mode == CBuilding::BUILD_AUTO) //not built auto building
				remainingAutoBuildings.push_back(build.second);
		}
	}

	//Prepare structure (list of building ids will be filled later)
	NewStructures ns;
	ns.tid = tid;
	ns.builded = force ? t->builded : (t->builded+1);

	std::queue<const CBuilding*> buildingsToAdd;
	buildingsToAdd.push(requestedBuilding);

	while(!buildingsToAdd.empty())
	{
		auto b = buildingsToAdd.front();
		buildingsToAdd.pop();

		ns.bid.insert(b->bid);
		buildingsThatWillBe.insert(b->bid);
		remainingAutoBuildings -= b;

		for(auto autoBuilding : remainingAutoBuildings)
		{
			if (autoBuilding->requirements.test(areRequirementsFullfilled))
				buildingsToAdd.push(autoBuilding);
		}
	}

	// FIXME: it's done before NewStructures applied because otherwise town window wont be properly updated on client. That should be actually fixed on client and not on server.
	for(auto builtID : ns.bid)
		processBeforeBuiltStructure(builtID);

	//Take cost
	if (!force)
	{
		SetResources sr;
		sr.player = t->tempOwner;
		sr.res = gs->getPlayer(t->tempOwner)->resources - requestedBuilding->resources;
		sendAndApply(&sr);
	}

	//We know what has been built, appluy changes. Do this as final step to properly update town window
	sendAndApply(&ns);

	//Other post-built events. To some logic like giving spells to work gamestate changes for new building must be already in place!
	for(auto builtID : ns.bid)
		processAfterBuiltStructure(builtID);

	// now when everything is built - reveal tiles for lookout tower
	FoWChange fw;
	fw.player = t->tempOwner;
	fw.mode = 1;
	getTilesInRange(fw.tiles, t->getSightCenter(), t->getSightRadius(), t->tempOwner, 1);
	sendAndApply(&fw);

	if(t->visitingHero)
		vistiCastleObjects (t, t->visitingHero);
	if(t->garrisonHero)
		vistiCastleObjects (t, t->garrisonHero);

	checkVictoryLossConditionsForPlayer(t->tempOwner);
	return true;
}
bool CGameHandler::razeStructure (ObjectInstanceID tid, BuildingID bid)
{
///incomplete, simply erases target building
	const CGTownInstance * t = getTown(tid);
	if (!vstd::contains(t->builtBuildings, bid))
		return false;
	RazeStructures rs;
	rs.tid = tid;
	rs.bid.insert(bid);
	rs.destroyed = t->destroyed + 1;
	sendAndApply(&rs);
//TODO: Remove dwellers
// 	if (t->subID == 4 && bid == 17) //Veil of Darkness
// 	{
// 		RemoveBonus rb(RemoveBonus::TOWN);
// 		rb.whoID = t->id;
// 		rb.source = Bonus::TOWN_STRUCTURE;
// 		rb.id = 17;
// 		sendAndApply(&rb);
// 	}
	return true;
}

void CGameHandler::sendMessageToAll( const std::string &message )
{
	SystemMessage sm;
	sm.text = message;
	sendToAllClients(&sm);
}

bool CGameHandler::recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dstid, CreatureID crid, ui32 cram, si32 fromLvl )
{
	const CGDwelling *dw = static_cast<const CGDwelling*>(gs->getObj(objid));
	const CArmedInstance *dst = nullptr;
	const CCreature *c = VLC->creh->creatures.at(crid);
	bool warMachine = c->hasBonusOfType(Bonus::SIEGE_WEAPON);

	//TODO: test for owning
	//TODO: check if dst can recruit objects (e.g. hero is actually visiting object, town and source are same, etc)
	dst = dynamic_cast<const CArmedInstance*>(getObj(dstid));

	assert(dw && dst);

	//verify
	bool found = false;
	int level = 0;

	for(; level < dw->creatures.size(); level++) //iterate through all levels
	{
		if ( (fromLvl != -1) && ( level !=fromLvl ) )
			continue;
		const auto &cur = dw->creatures.at(level); //current level info <amount, list of cr. ids>
		int i = 0;
		for(; i < cur.second.size(); i++) //look for crid among available creatures list on current level
			if(cur.second.at(i) == crid)
				break;

		if(i < cur.second.size())
		{
			found = true;
			cram = std::min(cram, cur.first); //reduce recruited amount up to available amount
			break;
		}
	}
	SlotID slot = dst->getSlotFor(crid);

	if( (!found && complain("Cannot recruit: no such creatures!"))
		|| (cram  >  VLC->creh->creatures.at(crid)->maxAmount(gs->getPlayer(dst->tempOwner)->resources) && complain("Cannot recruit: lack of resources!"))
		|| (cram<=0  &&  complain("Cannot recruit: cram <= 0!"))
		|| (!slot.validSlot()  && !warMachine && complain("Cannot recruit: no available slot!")))
	{
		return false;
	}

	//recruit
	SetResources sr;
	sr.player = dst->tempOwner;
	sr.res = gs->getPlayer(dst->tempOwner)->resources - (c->cost * cram);

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = dw->creatures;
	sac.creatures[level].first -= cram;

	sendAndApply(&sr);
	sendAndApply(&sac);

	if(warMachine)
	{
		const CGHeroInstance *h = dynamic_cast<const CGHeroInstance*>(dst);
		if(!h)
			COMPLAIN_RET("Only hero can buy war machines");

		switch(crid)
		{
		case CreatureID::BALLISTA:
			giveHeroNewArtifact(h, VLC->arth->artifacts[ArtifactID::BALLISTA], ArtifactPosition::MACH1);
			break;
		case CreatureID::FIRST_AID_TENT:
			giveHeroNewArtifact(h, VLC->arth->artifacts[ArtifactID::FIRST_AID_TENT], ArtifactPosition::MACH3);
			break;
		case CreatureID::AMMO_CART:
			giveHeroNewArtifact(h, VLC->arth->artifacts[ArtifactID::AMMO_CART], ArtifactPosition::MACH2);
			break;
		default:
			complain("This war machine cannot be recruited!");
			return false;
		}
	}
	else
	{
		addToSlot(StackLocation(dst, slot), c, cram);
	}
	return true;
}

bool CGameHandler::upgradeCreature( ObjectInstanceID objid, SlotID pos, CreatureID upgID )
{
	CArmedInstance *obj = static_cast<CArmedInstance*>(gs->getObjInstance(objid));
	if (!obj->hasStackAtSlot(pos))
	{
		COMPLAIN_RET("Cannot upgrade, no stack at slot " + boost::to_string(pos));
	}
	UpgradeInfo ui = gs->getUpgradeInfo(obj->getStack(pos));
	PlayerColor player = obj->tempOwner;
	const PlayerState *p = getPlayer(player);
	int crQuantity = obj->stacks.at(pos)->count;
	int newIDpos= vstd::find_pos(ui.newID, upgID);//get position of new id in UpgradeInfo

	//check if upgrade is possible
	if( (ui.oldID<0 || newIDpos == -1 ) && complain("That upgrade is not possible!"))
	{
		return false;
	}
	TResources totalCost = ui.cost.at(newIDpos) * crQuantity;

	//check if player has enough resources
	if(!p->resources.canAfford(totalCost))
		COMPLAIN_RET("Cannot upgrade, not enough resources!");

	//take resources
	SetResources sr;
	sr.player = player;
	sr.res = p->resources - totalCost;
	sendAndApply(&sr);

	//upgrade creature
	changeStackType(StackLocation(obj, pos), VLC->creh->creatures.at(upgID));
	return true;
}

bool CGameHandler::changeStackType(const StackLocation &sl, const CCreature *c)
{
	if(!sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to change type");

	SetStackType sst;
	sst.sl = sl;
	sst.type = c;
	sendAndApply(&sst);
	return true;
}

void CGameHandler::moveArmy(const CArmedInstance *src, const CArmedInstance *dst, bool allowMerging)
{
	assert(src->canBeMergedWith(*dst, allowMerging));
	while(src->stacksCount())//while there are unmoved creatures
	{
		auto i = src->Slots().begin(); //iterator to stack to move
		StackLocation sl(src, i->first); //location of stack to move

		SlotID pos = dst->getSlotFor(i->second->type);
		if(!pos.validSlot())
		{
			//try to merge two other stacks to make place
			std::pair<SlotID, SlotID> toMerge;
			if(dst->mergableStacks(toMerge, i->first) && allowMerging)
			{
				moveStack(StackLocation(dst, toMerge.first), StackLocation(dst, toMerge.second)); //merge toMerge.first into toMerge.second
				assert(!dst->hasStackAtSlot(toMerge.first)); //we have now a new free slot
				moveStack(sl, StackLocation(dst, toMerge.first)); //move stack to freed slot
			}
			else
			{
				complain("Unexpected failure during an attempt to move army from " + src->nodeName() + " to " + dst->nodeName() + "!");
				return;
			}
		}
		else
		{
			moveStack(sl, StackLocation(dst, pos));
		}
	}
}

bool CGameHandler::garrisonSwap( ObjectInstanceID tid )
{
	CGTownInstance *town = gs->getTown(tid);
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{

		if(!town->visitingHero->canBeMergedWith(*town))
		{
			complain("Cannot make garrison swap, not enough free slots!");
			return false;
		}

		moveArmy(town, town->visitingHero, true);

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.visiting = ObjectInstanceID();
		intown.garrison = town->visitingHero->id;
		sendAndApply(&intown);
		return true;
	}
	else if (town->garrisonHero && !town->visitingHero) //move hero out of the garrison
	{
		//check if moving hero out of town will break 8 wandering heroes limit
		if(getHeroCount(town->garrisonHero->tempOwner,false) >= 8)
		{
			complain("Cannot move hero out of the garrison, there are already 8 wandering heroes!");
			return false;
		}

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = ObjectInstanceID();
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(&intown);
		return true;
	}
	else if(!!town->garrisonHero && town->visitingHero) //swap visiting and garrison hero
	{
		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = town->visitingHero->id;
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(&intown);
		return true;
	}
	else
	{
		complain("Cannot swap garrison hero!");
		return false;
	}
}

// With the amount of changes done to the function, it's more like transferArtifacts.
// Function moves artifact from src to dst. If dst is not a backpack and is already occupied, old dst art goes to backpack and is replaced.
bool CGameHandler::moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2)
{
	ArtifactLocation src = al1, dst = al2;
	const PlayerColor srcPlayer = src.owningPlayer(), dstPlayer = dst.owningPlayer();
	const CArmedInstance *srcObj = src.relatedObj(), *dstObj = dst.relatedObj();

	// Make sure exchange is even possible between the two heroes.
	if(!isAllowedExchange(srcObj->id, dstObj->id))
		COMPLAIN_RET("That heroes cannot make any exchange!");

	const CArtifactInstance *srcArtifact = src.getArt();
	const CArtifactInstance *destArtifact = dst.getArt();

	if (srcArtifact == nullptr)
		COMPLAIN_RET("No artifact to move!");
	if (destArtifact && srcPlayer != dstPlayer)
		COMPLAIN_RET("Can't touch artifact on hero of another player!");

	// Check if src/dest slots are appropriate for the artifacts exchanged.
	// Moving to the backpack is always allowed.
	if ((!srcArtifact || dst.slot < GameConstants::BACKPACK_START)
		&& srcArtifact && !srcArtifact->canBePutAt(dst, true))
		COMPLAIN_RET("Cannot move artifact!");

	if ((srcArtifact && srcArtifact->artType->id == ArtifactID::ART_LOCK) || (destArtifact && destArtifact->artType->id == ArtifactID::ART_LOCK))
		COMPLAIN_RET("Cannot move artifact locks.");

	if (dst.slot >= GameConstants::BACKPACK_START && srcArtifact->artType->isBig())
		COMPLAIN_RET("Cannot put big artifacts in backpack!");
	if (src.slot == ArtifactPosition::MACH4 || dst.slot == ArtifactPosition::MACH4)
		COMPLAIN_RET("Cannot move catapult!");

	if(dst.slot >= GameConstants::BACKPACK_START)
		vstd::amin(dst.slot, ArtifactPosition(GameConstants::BACKPACK_START + dst.getHolderArtSet()->artifactsInBackpack.size()));

	if (src.slot == dst.slot  &&  src.artHolder == dst.artHolder)
		COMPLAIN_RET("Won't move artifact: Dest same as source!");

	if(dst.slot < GameConstants::BACKPACK_START  &&  destArtifact) //moving art to another slot
	{
		//old artifact must be removed first
		moveArtifact(dst, ArtifactLocation(dst.artHolder, ArtifactPosition(
			dst.getHolderArtSet()->artifactsInBackpack.size() + GameConstants::BACKPACK_START)));
	}

	MoveArtifact ma;
	ma.src = src;
	ma.dst = dst;
	sendAndApply(&ma);
	return true;
}

/**
 * Assembles or disassembles a combination artifact.
 * @param heroID ID of hero holding the artifact(s).
 * @param artifactSlot The worn slot ID of the combination- or constituent artifact.
 * @param assemble True for assembly operation, false for disassembly.
 * @param assembleTo If assemble is true, this represents the artifact ID of the combination
 * artifact to assemble to. Otherwise it's not used.
 */
bool CGameHandler::assembleArtifacts (ObjectInstanceID heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)
{

	CGHeroInstance *hero = gs->getHero(heroID);
	const CArtifactInstance *destArtifact = hero->getArt(artifactSlot);

	if(!destArtifact)
		COMPLAIN_RET("assembleArtifacts: there is no such artifact instance!");

	if(assemble)
	{
		CArtifact *combinedArt = VLC->arth->artifacts[assembleTo];
		if(!combinedArt->constituents)
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to assemble is not a combined artifacts!");
		if(!vstd::contains(destArtifact->assemblyPossibilities(hero), combinedArt))
			COMPLAIN_RET("assembleArtifacts: It's impossible to assemble requested artifact!");

		AssembledArtifact aa;
		aa.al = ArtifactLocation(hero, artifactSlot);
		aa.builtArt = combinedArt;
		sendAndApply(&aa);
	}
	else
	{
		if(!destArtifact->artType->constituents)
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble is not a combined artifact!");

		DisassembledArtifact da;
		da.al = ArtifactLocation(hero, artifactSlot);
		sendAndApply(&da);
	}

	return true;
}

bool CGameHandler::buyArtifact( ObjectInstanceID hid, ArtifactID aid )
{
	CGHeroInstance *hero = gs->getHero(hid);
	CGTownInstance *town = hero->visitedTown;
	if(aid==ArtifactID::SPELLBOOK)
	{
		if((!town->hasBuilt(BuildingID::MAGES_GUILD_1) && complain("Cannot buy a spellbook, no mage guild in the town!"))
		    || (getResource(hero->getOwner(), Res::GOLD) < GameConstants::SPELLBOOK_GOLD_COST && complain("Cannot buy a spellbook, not enough gold!") )
		    || (hero->getArt(ArtifactPosition::SPELLBOOK) && complain("Cannot buy a spellbook, hero already has a one!"))
		    )
			return false;

		giveResource(hero->getOwner(),Res::GOLD,-GameConstants::SPELLBOOK_GOLD_COST);
		giveHeroNewArtifact(hero, VLC->arth->artifacts[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);
		assert(hero->getArt(ArtifactPosition::SPELLBOOK));
		giveSpells(town,hero);
		return true;
	}
	else if(aid < 7  &&  aid > 3) //war machine
	{
		int price = VLC->arth->artifacts[aid]->price;

		if(( hero->getArt(ArtifactPosition(9+aid)) && complain("Hero already has this machine!"))
		 || (gs->getPlayer(hero->getOwner())->resources.at(Res::GOLD) < price && complain("Not enough gold!")))
		{
			return false;
		}
		if  ((town->hasBuilt(BuildingID::BLACKSMITH) && town->town->warMachine == aid )
		 || ((town->hasBuilt(BuildingID::BALLISTA_YARD, ETownType::STRONGHOLD)) && aid == ArtifactID::BALLISTA))
		{
			giveResource(hero->getOwner(),Res::GOLD,-price);
			giveHeroNewArtifact(hero, VLC->arth->artifacts[aid], ArtifactPosition(9+aid));
			return true;
		}
		else
			COMPLAIN_RET("This machine is unavailable here!");
	}
	return false;
}

bool CGameHandler::buyArtifact(const IMarket *m, const CGHeroInstance *h, Res::ERes rid, ArtifactID aid)
{
	if(!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), aid))
		COMPLAIN_RET("That artifact is unavailable!");

	int b1, b2;
	m->getOffer(rid, aid, b1, b2, EMarketMode::RESOURCE_ARTIFACT);

	if(getResource(h->tempOwner, rid) < b1)
		COMPLAIN_RET("You can't afford to buy this artifact!");

	SetResource sr;
	sr.player = h->tempOwner;
	sr.resid = rid;
	sr.val = getResource(h->tempOwner, rid) - b1;
	sendAndApply(&sr);


	SetAvailableArtifacts saa;
	if(m->o->ID == Obj::TOWN)
	{
		saa.id = -1;
		saa.arts = CGTownInstance::merchantArtifacts;
	}
	else if(const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(m->o)) //black market
	{
		saa.id = bm->id.getNum();
		saa.arts = bm->artifacts;
	}
	else
		COMPLAIN_RET("Wrong marktet...");

	bool found = false;
	for(const CArtifact *&art : saa.arts)
	{
		if(art && art->id == aid)
		{
			art = nullptr;
			found = true;
			break;
		}
	}

	if(!found)
		COMPLAIN_RET("Cannot find selected artifact on the list");

	sendAndApply(&saa);

	giveHeroNewArtifact(h, VLC->arth->artifacts[aid], ArtifactPosition::FIRST_AVAILABLE);
	return true;
}

bool CGameHandler::sellArtifact( const IMarket *m, const CGHeroInstance *h, ArtifactInstanceID aid, Res::ERes rid )
{
	const CArtifactInstance *art = h->getArtByInstanceId(aid);
	if(!art)
		COMPLAIN_RET("There is no artifact to sell!");
	if(!art->artType->isTradable())
		COMPLAIN_RET("Cannot sell a war machine or spellbook!");

	int resVal = 0, dump = 1;
	m->getOffer(art->artType->id, rid, dump, resVal, EMarketMode::ARTIFACT_RESOURCE);

	removeArtifact(ArtifactLocation(h, h->getArtPos(art)));
	giveResource(h->tempOwner, rid, resVal);
	return true;
}

//void CGameHandler::lootArtifacts (TArtHolder source, TArtHolder dest, std::vector<ui32> &arts)
//{
//	//const CGHeroInstance * h1 = dynamic_cast<CGHeroInstance *> source;
//	//auto s = boost::apply_visitor(GetArtifactSetPtr(), source);
//	{
//	}
//}

bool CGameHandler::buySecSkill( const IMarket *m, const CGHeroInstance *h, SecondarySkill skill)
{
	if (!h)
		COMPLAIN_RET("You need hero to buy a skill!");

	if (h->getSecSkillLevel(SecondarySkill(skill)))
		COMPLAIN_RET("Hero already know this skill");

	if (!h->canLearnSkill())
		COMPLAIN_RET("Hero can't learn any more skills");

	if (h->type->heroClass->secSkillProbability.at(skill)==0)//can't learn this skill (like necromancy for most of non-necros)
		COMPLAIN_RET("The hero can't learn this skill!");

	if(!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_SKILL), skill))
		COMPLAIN_RET("That skill is unavailable!");

	if(getResource(h->tempOwner, Res::GOLD) < 2000)//TODO: remove hardcoded resource\summ?
		COMPLAIN_RET("You can't afford to buy this skill");

	SetResource sr;
	sr.player = h->tempOwner;
	sr.resid = Res::GOLD;
	sr.val = getResource(h->tempOwner, Res::GOLD) - 2000;
	sendAndApply(&sr);

	changeSecSkill(h, skill, 1, true);
	return true;
}

bool CGameHandler::tradeResources(const IMarket *market, ui32 val, PlayerColor player, ui32 id1, ui32 id2)
{
	int r1 = gs->getPlayer(player)->resources.at(id1),
		r2 = gs->getPlayer(player)->resources.at(id2);

	vstd::amin(val, r1); //can't trade more resources than have

	int b1, b2; //base quantities for trade
	market->getOffer(id1, id2, b1, b2, EMarketMode::RESOURCE_RESOURCE);
	int units = val / b1; //how many base quantities we trade

	if(val%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		//TODO: complain?
		assert(0);
	}

	SetResource sr;
	sr.player = player;
	sr.resid = static_cast<Res::ERes>(id1);
	sr.val = r1 - b1 * units;
	sendAndApply(&sr);

	sr.resid = static_cast<Res::ERes>(id2);
	sr.val = r2 + b2 * units;
	sendAndApply(&sr);

	return true;
}

bool CGameHandler::sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, SlotID slot, Res::ERes resourceID)
{
	if(!vstd::contains(hero->Slots(), slot))
		COMPLAIN_RET("Hero doesn't have any creature in that slot!");

	const CStackInstance &s = hero->getStack(slot);

	if( s.count < count //can't sell more creatures than have
		|| (hero->stacksCount() == 1 && hero->needsLastStack() && s.count == count)) //can't sell last stack
	{
		COMPLAIN_RET("Not enough creatures in army!");
	}

	int b1, b2; //base quantities for trade
 	market->getOffer(s.type->idNumber, resourceID, b1, b2, EMarketMode::CREATURE_RESOURCE);
 	int units = count / b1; //how many base quantities we trade

 	if(count%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
 	{
 		//TODO: complain?
 		assert(0);
 	}

	changeStackCount(StackLocation(hero, slot), -count);

 	SetResource sr;
 	sr.player = hero->tempOwner;
 	sr.resid = resourceID;
 	sr.val = getResource(hero->tempOwner, resourceID) + b2 * units;
 	sendAndApply(&sr);

	return true;
}

bool CGameHandler::transformInUndead(const IMarket *market, const CGHeroInstance * hero, SlotID slot)
{
	const CArmedInstance *army = nullptr;
	if (hero)
		army = hero;
	else
		army = dynamic_cast<const CGTownInstance *>(market->o);

	if (!army)
		COMPLAIN_RET("Incorrect call to transform in undead!");
	if(!army->hasStackAtSlot(slot))
		COMPLAIN_RET("Army doesn't have any creature in that slot!");


	const CStackInstance &s = army->getStack(slot);

	//resulting creature - bone dragons or skeletons
	CreatureID resCreature = CreatureID::SKELETON;

	if(s.hasBonusOfType(Bonus::DRAGON_NATURE)
			|| (s.getCreatureID() == CreatureID::HYDRA)
			|| (s.getCreatureID() == CreatureID::CHAOS_HYDRA))
		resCreature = CreatureID::BONE_DRAGON;
	changeStackType(StackLocation(army, slot), resCreature.toCreature());
	return true;
}

bool CGameHandler::sendResources(ui32 val, PlayerColor player, Res::ERes r1, PlayerColor r2)
{
	const PlayerState *p2 = gs->getPlayer(r2, false);
	if(!p2  ||  p2->status != EPlayerStatus::INGAME)
	{
		complain("Dest player must be in game!");
		return false;
	}

	si32 curRes1 = gs->getPlayer(player)->resources.at(r1),
	     curRes2 = gs->getPlayer(r2)->resources.at(r1);
	val = std::min(si32(val),curRes1);

	SetResource sr;
	sr.player = player;
	sr.resid = r1;
	sr.val = curRes1 - val;
	sendAndApply(&sr);

	sr.player = r2;
	sr.val = curRes2 + val;
	sendAndApply(&sr);

	return true;
}

bool CGameHandler::setFormation( ObjectInstanceID hid, ui8 formation )
{
	gs->getHero(hid)-> formation = formation;
	return true;
}

bool CGameHandler::hireHero(const CGObjectInstance *obj, ui8 hid, PlayerColor player)
{
	const PlayerState *p = gs->getPlayer(player);
	const CGTownInstance *t = gs->getTown(obj->id);

	//common preconditions
//	if( (p->resources.at(Res::GOLD)<GOLD_NEEDED  && complain("Not enough gold for buying hero!"))
//		|| (getHeroCount(player, false) >= GameConstants::MAX_HEROES_PER_PLAYER && complain("Cannot hire hero, only 8 wandering heroes are allowed!")))
	if((p->resources.at(Res::GOLD) < GameConstants::HERO_GOLD_COST && complain("Not enough gold for buying hero!"))
		|| ((!t) && (getHeroCount(player, false) >= VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER && complain("Cannot hire hero, too many wandering heroes already!")))
		|| ((t) && (getHeroCount(player, true) >= VLC->modh->settings.MAX_HEROES_AVAILABLE_PER_PLAYER && complain("Cannot hire hero, too many heroes garrizoned and wandering already!"))) )
	{
		return false;
	}

	if(t) //tavern in town
	{
		if((!t->hasBuilt(BuildingID::TAVERN) && complain("No tavern!"))
			 || (t->visitingHero  && complain("There is visiting hero - no place!")))
		{
			return false;
		}
	}
	else if(obj->ID == Obj::TAVERN)
	{
		if(getTile(obj->visitablePos())->visitableObjects.back() != obj && complain("Tavern entry must be unoccupied!"))
		{
			return false;
		}
	}

	const CGHeroInstance *nh = p->availableHeroes.at(hid);
	if (!nh)
	{
		complain ("Hero is not available for hiring!");
		return false;
	}

	HeroRecruited hr;
	hr.tid = obj->id;
	hr.hid = nh->subID;
	hr.player = player;
	hr.tile = obj->visitablePos() + nh->getVisitableOffset();
	sendAndApply(&hr);

	std::map<ui32, ConstTransitivePtr<CGHeroInstance> > pool = gs->unusedHeroesFromPool();

	const CGHeroInstance *theOtherHero = p->availableHeroes.at(!hid);
	const CGHeroInstance *newHero = nullptr;
	if (theOtherHero) //on XXL maps all heroes can be imprisoned :(
	{
		newHero = gs->hpool.pickHeroFor(false, player, getNativeTown(player), pool, gs->getRandomGenerator(), theOtherHero->type->heroClass);
	}

	SetAvailableHeroes sah;
	sah.player = player;

	if(newHero)
	{
		sah.hid[hid] = newHero->subID;
		sah.army[hid].clear();
		sah.army[hid].setCreature(SlotID(0), newHero->type->initialArmy[0].creature, 1);
	}
	else
	{
		sah.hid[hid] = -1;
	}

	sah.hid[!hid] = theOtherHero ? theOtherHero->subID : -1;
	sendAndApply(&sah);

	SetResource sr;
	sr.player = player;
	sr.resid = Res::GOLD;
	sr.val = p->resources.at(Res::GOLD) - GameConstants::HERO_GOLD_COST;
	sendAndApply(&sr);

	if(t)
	{
		vistiCastleObjects (t, nh);
		giveSpells (t,nh);
	}
	return true;
}

bool CGameHandler::queryReply(QueryID qid, ui32 answer, PlayerColor player)
{
	boost::unique_lock<boost::recursive_mutex> lock(gsm);

	logGlobal->traceStream()  << boost::format("Player %s attempts answering query %d with answer %d") % player % qid % answer;

	auto topQuery = queries.topQuery(player);
	COMPLAIN_RET_FALSE_IF(!topQuery, "This player doesn't have any queries!");
	COMPLAIN_RET_FALSE_IF(topQuery->queryID != qid, "This player top query has different ID!");
	COMPLAIN_RET_FALSE_IF(!topQuery->endsByPlayerAnswer(), "This query cannot be ended by player's answer!");

	if(auto dialogQuery = std::dynamic_pointer_cast<CDialogQuery>(topQuery))
		dialogQuery->answer = answer;

	queries.popQuery(topQuery);
	return true;
}

static EndAction end_action;

void CGameHandler::updateGateState()
{
	BattleUpdateGateState db;
	db.state = gs->curB->si.gateState;
	if(gs->curB->si.wallState[EWallPart::GATE] == EWallState::DESTROYED)
	{
		db.state = EGateState::DESTROYED;
	}
	else if(db.state == EGateState::OPENED)
	{
		if(!gs->curB->battleGetStackByPos(BattleHex(ESiegeHex::GATE_OUTER), false) &&
			!gs->curB->battleGetStackByPos(BattleHex(ESiegeHex::GATE_INNER), false))
		{
			if(gs->curB->town->subID == ETownType::FORTRESS)
			{
				if(!gs->curB->battleGetStackByPos(BattleHex(ESiegeHex::GATE_BRIDGE), false))
					db.state = EGateState::CLOSED;
			}
			else if(gs->curB->battleGetStackByPos(BattleHex(ESiegeHex::GATE_BRIDGE)))
				db.state = EGateState::BLOCKED;
			else
				db.state = EGateState::CLOSED;
		}
	}
	else if(gs->curB->battleGetStackByPos(BattleHex(ESiegeHex::GATE_BRIDGE), false))
		db.state = EGateState::BLOCKED;
	else
		db.state = EGateState::CLOSED;

	if(db.state != gs->curB->si.gateState)
		sendAndApply(&db);
}

bool CGameHandler::makeBattleAction( BattleAction &ba )
{
	bool ok = true;

	const CStack *stack = battleGetStackByID(ba.stackNumber); //may be nullptr if action is not about stack
	const CStack *destinationStack = ba.actionType == Battle::WALK_AND_ATTACK ? gs->curB->battleGetStackByPos(ba.additionalInfo)
								   : ba.actionType == Battle::SHOOT			  ? gs->curB->battleGetStackByPos(ba.destinationTile)
																			  : nullptr;
	const bool isAboutActiveStack = stack && (stack == battleActiveStack());

	logGlobal->traceStream() << boost::format(
		"Making action: type=%d; side=%d; stack=%s; dst=%s; additionalInfo=%d; stackAtDst=%s")
		% ba.actionType % (int)ba.side % (stack ? stack->getName() : std::string("none"))
		% ba.destinationTile % ba.additionalInfo % (destinationStack ? destinationStack->getName() : std::string("none"));

	switch(ba.actionType)
	{
	case Battle::WALK: //walk
	case Battle::DEFEND: //defend
	case Battle::WAIT: //wait
	case Battle::WALK_AND_ATTACK: //walk or attack
	case Battle::SHOOT: //shoot
	case Battle::CATAPULT: //catapult
	case Battle::STACK_HEAL: //healing with First Aid Tent
	case Battle::DAEMON_SUMMONING:
	case Battle::MONSTER_SPELL:

		if(!stack)
		{
			complain("No such stack!");
			return false;
		}
		if(!stack->alive())
		{
			complain("This stack is dead: " + stack->nodeName());
			return false;
		}

		if(battleTacticDist())
		{
			if(stack && !stack->attackerOwned != battleGetTacticsSide())
			{
				complain("This is not a stack of side that has tactics!");
				return false;
			}
		}
		else if(!isAboutActiveStack)
		{
			complain("Action has to be about active stack!");
			return false;
		}
	}


	switch(ba.actionType)
	{
	case Battle::END_TACTIC_PHASE: //wait
	case Battle::BAD_MORALE:
	case Battle::NO_ACTION:
		{
			StartAction start_action(ba);
			sendAndApply(&start_action);
			sendAndApply(&end_action);
			break;
		}
	case Battle::WALK:
		{
			StartAction start_action(ba);
			sendAndApply(&start_action); //start movement
			int walkedTiles = moveStack(ba.stackNumber,ba.destinationTile); //move
			if(!walkedTiles)
				complain("Stack failed movement!");

			sendAndApply(&end_action);
			break;
		}
	case Battle::DEFEND:
		{
			//defensive stance //TODO: remove this bonus when stack becomes active
			SetStackEffect sse;
			sse.effect.push_back( Bonus(Bonus::STACK_GETS_TURN, Bonus::PRIMARY_SKILL, Bonus::OTHER, 20, -1, PrimarySkill::DEFENSE, Bonus::PERCENT_TO_ALL) );
			sse.effect.push_back( Bonus(Bonus::STACK_GETS_TURN, Bonus::PRIMARY_SKILL, Bonus::OTHER, gs->curB->battleGetStackByID(ba.stackNumber)->valOfBonuses(Bonus::DEFENSIVE_STANCE),
				 -1, PrimarySkill::DEFENSE, Bonus::ADDITIVE_VALUE));
			sse.stacks.push_back(ba.stackNumber);
			sendAndApply(&sse);

			//don't break - we share code with next case
		}
	case Battle::WAIT:
		{
			StartAction start_action(ba);
			sendAndApply(&start_action);
			sendAndApply(&end_action);
			break;
		}
	case Battle::RETREAT: //retreat/flee
		{
			if(!gs->curB->battleCanFlee(gs->curB->sides.at(ba.side).color))
				complain("Cannot retreat!");
			else
				setBattleResult(BattleResult::ESCAPE, !ba.side); //surrendering side loses
			break;
		}
	case Battle::SURRENDER:
		{
			PlayerColor player = gs->curB->sides.at(ba.side).color;
			int cost = gs->curB->battleGetSurrenderCost(player);
			if(cost < 0)
				complain("Cannot surrender!");
			else if(getResource(player, Res::GOLD) < cost)
				complain("Not enough gold to surrender!");
			else
			{
				giveResource(player, Res::GOLD, -cost);
				setBattleResult(BattleResult::SURRENDER, !ba.side); //surrendering side loses
			}
			break;
		}
	case Battle::WALK_AND_ATTACK: //walk or attack
		{
			StartAction start_action(ba);
			sendAndApply(&start_action); //start movement and attack

			if(!stack || !destinationStack)
			{
				sendAndApply(&end_action);
				break;
			}

			BattleHex startingPos = stack->position;
			int distance = moveStack(ba.stackNumber, ba.destinationTile);

			logGlobal->traceStream() << stack->nodeName() << " will attack " << destinationStack->nodeName();

			if(stack->position != ba.destinationTile //we wasn't able to reach destination tile
				&& !(stack->doubleWide()
					&&  ( stack->position == ba.destinationTile + (stack->attackerOwned ?  +1 : -1 ) )
						) //nor occupy specified hex
				)
			{
				std::string problem = "We cannot move this stack to its destination " + stack->getCreature()->namePl;
				logGlobal->warnStream() << problem;
				complain(problem);
				ok = false;
				sendAndApply(&end_action);
				break;
			}

			if(destinationStack && stack && stack->ID == destinationStack->ID) //we should just move, it will be handled by following check
			{
				destinationStack = nullptr;
			}

			if(!destinationStack)
			{
				complain(boost::str(boost::format("walk and attack error: no stack at additionalInfo tile (%d)!\n") % ba.additionalInfo));
				ok = false;
				sendAndApply(&end_action);
				break;
			}

			if( !CStack::isMeleeAttackPossible(stack, destinationStack) )
			{
				complain("Attack cannot be performed!");
				sendAndApply(&end_action);
				ok = false;
				break;
			}

			//attack

			int totalAttacks = 1 + stack->getBonuses(Selector::type (Bonus::ADDITIONAL_ATTACK),
				(Selector::effectRange(Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))))->totalValue(); //all unspicified attacks + melee attacks

			for (int i = 0; i < totalAttacks; ++i)
			{
				if (stack &&
					stack->alive() && //move can cause death, eg. by walking into the moat
					destinationStack->alive())
				{
					BattleAttack bat;
					prepareAttack(bat, stack, destinationStack, (i ? 0 : distance),  ba.additionalInfo); //no distance travelled on second attack
					//prepareAttack(bat, stack, stackAtEnd, 0, ba.additionalInfo);
					handleAttackBeforeCasting(bat); //only before first attack
					sendAndApply(&bat);
					handleAfterAttackCasting(bat);
				}

				//counterattack
				if (destinationStack
					&& !stack->hasBonusOfType(Bonus::BLOCKS_RETALIATION)
					&& destinationStack->ableToRetaliate()
					&& stack->alive()) //attacker may have died (fire shield)
				{
					BattleAttack bat;
					prepareAttack(bat, destinationStack, stack, 0, stack->position);
					bat.flags |= BattleAttack::COUNTER;
					sendAndApply(&bat);
					handleAfterAttackCasting(bat);
				}
			}

			//return
			if(stack->hasBonusOfType(Bonus::RETURN_AFTER_STRIKE) && startingPos != stack->position && stack->alive())
			{
				moveStack(ba.stackNumber, startingPos);
				//NOTE: curStack->ID == ba.stackNumber (rev 1431)
			}

			sendAndApply(&end_action);
			break;
		}
	case Battle::SHOOT:
		{
			if( !gs->curB->battleCanShoot(stack, ba.destinationTile) )
			{
				complain("Cannot shoot!");
				break;
			}

			StartAction start_action(ba);
			sendAndApply(&start_action); //start shooting

			{
				BattleAttack bat;
				bat.flags |= BattleAttack::SHOT;
				prepareAttack(bat, stack, destinationStack, 0, ba.destinationTile);
				handleAttackBeforeCasting(bat);
				sendAndApply(&bat);
				handleAfterAttackCasting(bat);
			}

			//second shot for ballista, only if hero has advanced artillery

			const CGHeroInstance * attackingHero = gs->curB->battleGetFightingHero(ba.side);

			if( destinationStack->alive()
			    && (stack->getCreature()->idNumber == CreatureID::BALLISTA)
			    && (attackingHero->getSecSkillLevel(SecondarySkill::ARTILLERY) >= SecSkillLevel::ADVANCED)
			   )
			{
				BattleAttack bat2;
				bat2.flags |= BattleAttack::SHOT;
				prepareAttack(bat2, stack, destinationStack, 0, ba.destinationTile);
				sendAndApply(&bat2);
			}
			//allow more than one additional attack

			int additionalAttacks = stack->getBonuses(Selector::type (Bonus::ADDITIONAL_ATTACK),
				(Selector::effectRange(Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT))))->totalValue();
			for (int i = 0; i < additionalAttacks; ++i)
			{
				if(
					stack->alive()
					&& destinationStack->alive()
					&& stack->shots
					)
				{
					BattleAttack bat;
					bat.flags |= BattleAttack::SHOT;
					prepareAttack(bat, stack, destinationStack, 0, ba.destinationTile);
					sendAndApply(&bat);
					handleAfterAttackCasting(bat);
				}
			}

			sendAndApply(&end_action);
			break;
		}
	case Battle::CATAPULT:
		{
			auto getCatapultHitChance = [&](EWallPart::EWallPart part, const CHeroHandler::SBallisticsLevelInfo & sbi) -> int
			{
				switch(part)
				{
				case EWallPart::GATE:
					return sbi.gate;
				case EWallPart::KEEP:
					return sbi.keep;
				case EWallPart::BOTTOM_TOWER:
				case EWallPart::UPPER_TOWER:
					return sbi.tower;
				case EWallPart::BOTTOM_WALL:
				case EWallPart::BELOW_GATE:
				case EWallPart::OVER_GATE:
				case EWallPart::UPPER_WALL:
					return sbi.wall;
				default:
					return 0;
				}
			};

			StartAction start_action(ba);
			sendAndApply(&start_action);
			auto onExit = vstd::makeScopeGuard([&]{ sendAndApply(&end_action); }); //if we started than we have to finish

			const CGHeroInstance * attackingHero = gs->curB->battleGetFightingHero(ba.side);
			CHeroHandler::SBallisticsLevelInfo sbi = VLC->heroh->ballistics.at(attackingHero->getSecSkillLevel(SecondarySkill::BALLISTICS));

			auto wallPart = gs->curB->battleHexToWallPart(ba.destinationTile);
			if(!gs->curB->isWallPartPotentiallyAttackable(wallPart))
			{
				complain("catapult tried to attack non-catapultable hex!");
				break;
			}

			//in successive iterations damage is dealt but not yet subtracted from wall's HPs
			auto &currentHP = gs->curB->si.wallState;

			if (currentHP.at(wallPart) == EWallState::DESTROYED  ||  currentHP.at(wallPart) == EWallState::NONE)
			{
				complain("catapult tried to attack already destroyed wall part!");
				break;
			}

			for(int g=0; g<sbi.shots; ++g)
			{
				bool hitSuccessfull = false;
				auto attackedPart = wallPart;

				do // catapult has chance to attack desired target. Othervice - attacks randomly
				{
					if(currentHP.at(attackedPart) != EWallState::DESTROYED && // this part can be hit
					   currentHP.at(attackedPart) != EWallState::NONE &&
					   gs->getRandomGenerator().nextInt(99) < getCatapultHitChance(attackedPart, sbi))//hit is successful
					{
						hitSuccessfull = true;
					}
					else // select new target
					{
						std::vector<EWallPart::EWallPart> allowedTargets;
						for (size_t i=0; i< currentHP.size(); i++)
						{
							if (currentHP.at(i) != EWallState::DESTROYED &&
							    currentHP.at(i) != EWallState::NONE)
								allowedTargets.push_back(EWallPart::EWallPart(i));
						}
						if (allowedTargets.empty())
							break;
						attackedPart = *RandomGeneratorUtil::nextItem(allowedTargets, gs->getRandomGenerator());
					}
				}
				while (!hitSuccessfull);

				if (!hitSuccessfull) // break triggered - no target to shoot at
					break;

				CatapultAttack ca; //package for clients
				CatapultAttack::AttackInfo attack;
				attack.attackedPart = attackedPart;
				attack.destinationTile = ba.destinationTile;
				attack.damageDealt = 0;

				int dmgChance[] = { sbi.noDmg, sbi.oneDmg, sbi.twoDmg }; //dmgChance[i] - chance for doing i dmg when hit is successful

				int dmgRand = gs->getRandomGenerator().nextInt(99);
				//accumulating dmgChance
				dmgChance[1] += dmgChance[0];
				dmgChance[2] += dmgChance[1];
				//calculating dealt damage
				for(int damage = 0; damage < ARRAY_COUNT(dmgChance); ++damage)
				{
					if(dmgRand <= dmgChance[damage])
					{
						attack.damageDealt = damage;
						break;
					}
				}
				// attacked tile may have changed - update destination
				attack.destinationTile = gs->curB->wallPartToBattleHex(EWallPart::EWallPart(attack.attackedPart));

				logGlobal->traceStream() << "Catapult attacks " << (int)attack.attackedPart
										 << " dealing " << (int)attack.damageDealt << " damage";

				//removing creatures in turrets / keep if one is destroyed
				if(attack.damageDealt > 0 && (attackedPart == EWallPart::KEEP ||
					attackedPart == EWallPart::BOTTOM_TOWER || attackedPart == EWallPart::UPPER_TOWER))
				{
					int posRemove = -1;
					switch(attackedPart)
					{
					case EWallPart::KEEP:
						posRemove = -2;
						break;
					case EWallPart::BOTTOM_TOWER:
						posRemove = -3;
						break;
					case EWallPart::UPPER_TOWER:
						posRemove = -4;
						break;
					}

					BattleStacksRemoved bsr;
					for(auto & elem : gs->curB->stacks)
					{
						if(elem->position == posRemove)
						{
							bsr.stackIDs.insert( elem->ID );
							break;
						}
					}

					sendAndApply(&bsr);
				}
				ca.attacker = ba.stackNumber;
				ca.attackedParts.push_back(attack);

				sendAndApply(&ca);
			}
			//finish by scope guard
			break;
		}
		case Battle::STACK_HEAL: //healing with First Aid Tent
		{
			StartAction start_action(ba);
			sendAndApply(&start_action);
			const CGHeroInstance * attackingHero = gs->curB->battleGetFightingHero(ba.side);
			const CStack *healer = gs->curB->battleGetStackByID(ba.stackNumber),
				*destStack = gs->curB->battleGetStackByPos(ba.destinationTile);

			ui32 healed = 0;

			if(healer == nullptr || destStack == nullptr || !healer->hasBonusOfType(Bonus::HEALER))
			{
				complain("There is either no healer, no destination, or healer cannot heal :P");
			}
			else
			{
				ui32 maxiumHeal = healer->count * std::max(10, attackingHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::FIRST_AID));
				healed = destStack->calculateHealedHealthPoints(maxiumHeal, false);
			}

			if(healed == 0)
			{
				//nothing to heal.. should we complain?
			}
			else
			{
				StacksHealedOrResurrected shr;
				shr.lifeDrain = false;
				shr.tentHealing = true;
				shr.drainedFrom = ba.stackNumber;

				StacksHealedOrResurrected::HealInfo hi;
				hi.healedHP = healed;
				hi.lowLevelResurrection = false;
				hi.stackID = destStack->ID;

				shr.healedStacks.push_back(hi);
				sendAndApply(&shr);
			}
			sendAndApply(&end_action);
			break;
		}
		case Battle::DAEMON_SUMMONING:
			//TODO: From Strategija:
			//Summon Demon is a level 2 spell.
		{
			const CStack *summoner = gs->curB->battleGetStackByID(ba.stackNumber),
				*destStack = gs->curB->battleGetStackByPos(ba.destinationTile, false);

			CreatureID summonedType(summoner->getBonusLocalFirst(Selector::type(Bonus::DAEMON_SUMMONING))->subtype);//in case summoner can summon more than one type of monsters... scream!
			BattleStackAdded bsa;
			bsa.attacker = summoner->attackerOwned;

			bsa.creID = summonedType;
			ui64 risedHp = summoner->count * summoner->valOfBonuses(Bonus::DAEMON_SUMMONING, bsa.creID.toEnum());
			ui64 targetHealth = destStack->getCreature()->MaxHealth() * destStack->baseAmount;//todo: ignore AGE effect

			ui64 canRiseHp = std::min(targetHealth, risedHp);
			ui32 canRiseAmount = canRiseHp / VLC->creh->creatures.at(bsa.creID)->MaxHealth();

			bsa.amount = std::min(canRiseAmount, destStack->baseAmount);

			bsa.pos = gs->curB->getAvaliableHex(bsa.creID, bsa.attacker, destStack->position);
			bsa.summoned = false;

			if (bsa.amount) //there's rare possibility single creature cannot rise desired type
			{
				StartAction start_action(ba);
				sendAndApply(&start_action);

				BattleStacksRemoved bsr; //remove body
				bsr.stackIDs.insert(destStack->ID);
				sendAndApply(&bsr);
				sendAndApply(&bsa);

				BattleSetStackProperty ssp;
				ssp.stackID = ba.stackNumber;
				ssp.which = BattleSetStackProperty::CASTS; //reduce number of casts
				ssp.val = -1;
				ssp.absolute = false;
				sendAndApply(&ssp);

				sendAndApply(&end_action);
			}
			break;
		}
		case Battle::MONSTER_SPELL:
		{
			StartAction start_action(ba);
			sendAndApply(&start_action);

			const CStack * stack = gs->curB->battleGetStackByID(ba.stackNumber);
			SpellID spellID = SpellID(ba.additionalInfo);
			BattleHex destination(ba.destinationTile);

			const Bonus *randSpellcaster = stack->getBonusLocalFirst(Selector::type(Bonus::RANDOM_SPELLCASTER));
			const Bonus * spellcaster = stack->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPELLCASTER, spellID));

			//TODO special bonus for genies ability
			if(randSpellcaster && battleGetRandomStackSpell(stack, CBattleInfoCallback::RANDOM_AIMED) < 0)
				spellID = battleGetRandomStackSpell(stack, CBattleInfoCallback::RANDOM_GENIE);

			if(spellID < 0)
				complain("That stack can't cast spells!");
			else
			{
				const CSpell * spell = SpellID(spellID).toSpell();
				BattleSpellCastParameters parameters(gs->curB, stack, spell);
				parameters.spellLvl = 0;
				if (spellcaster)
					vstd::amax(parameters.spellLvl, spellcaster->val);
				if (randSpellcaster)
					vstd::amax(parameters.spellLvl, randSpellcaster->val);
				vstd::amin (parameters.spellLvl, 3);
				parameters.effectLevel = parameters.spellLvl;
				parameters.mode = ECastingMode::CREATURE_ACTIVE_CASTING;
				parameters.aimToHex(destination);//todo: allow multiple destinations
				parameters.selectedStack = nullptr;
				spell->battleCast(spellEnv, parameters);
			}
			sendAndApply(&end_action);
			break;
		}
	}
	if(ba.stackNumber == gs->curB->activeStack  ||  battleResult.get()) //active stack has moved or battle has finished
		battleMadeAction.setn(true);
	return ok;
}

void CGameHandler::playerMessage( PlayerColor player, const std::string &message, ObjectInstanceID currObj )
{
	bool cheated=true;
	PlayerMessage temp_message(player, message, ObjectInstanceID(-1)); // don't inform other client on selected object

	sendAndApply(&temp_message);
	if(message == "vcmiistari") //give all spells and 999 mana
	{
		SetMana sm;
		GiveBonus giveBonus(GiveBonus::HERO);

		CGHeroInstance *h = gs->getHero(currObj);
		if(!h && complain("Cannot realize cheat, no hero selected!")) return;

		sm.hid = h->id;

		giveBonus.id = h->id.getNum();

		//give all spells with bonus (to allow banned spells)
		giveBonus.bonus = Bonus(Bonus::PERMANENT, Bonus::SPELLS_OF_LEVEL, Bonus::OTHER, 0, 0);
		//start with level 0 to skip abilities
		for(int level = 1; level <= GameConstants::SPELL_LEVELS; level++)
		{
			giveBonus.bonus.subtype = level;
			sendAndApply(&giveBonus);
		}
		//give mana
		sm.val = 999;
		sm.absolute = true;

		if(!h->hasSpellbook()) //hero doesn't have spellbook
			giveHeroNewArtifact(h, VLC->arth->artifacts[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK); //give spellbook

		sendAndApply(&sm);
	}
	else if (message == "vcmiarmenelos") //build all buildings in selected town
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		CGTownInstance *town;

		if (hero)
			town = hero->visitedTown;
		else
			town = gs->getTown(currObj);

		if (town)
		{
			for (auto & build : town->town->buildings)
			{
				if (!town->hasBuilt(build.first)
				    && !build.second->Name().empty()
				    && build.first != BuildingID::SHIP)
				{
					buildStructure(town->id, build.first, true);
				}
			}
		}
	}
	else if(message == "vcmiainur") //gives 5 archangels into each slot
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		const CCreature *archangel = VLC->creh->creatures.at(13);
		if(!hero) return;

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
			if(!hero->hasStackAtSlot(SlotID(i)))
				insertNewStack(StackLocation(hero, SlotID(i)), archangel, 5);
	}
	else if(message == "vcmiangband") //gives 10 black knight into each slot
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		const CCreature *blackKnight = VLC->creh->creatures.at(66);
		if(!hero) return;

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
			if(!hero->hasStackAtSlot(SlotID(i)))
				insertNewStack(StackLocation(hero, SlotID(i)), blackKnight, 10);
	}
	else if(message == "vcmiglaurung") //gives 5000 crystal dragons into each slot
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		const CCreature *crystalDragon = VLC->creh->creatures.at(133);
		if(!hero) return;

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
			if(!hero->hasStackAtSlot(SlotID(i)))
				insertNewStack(StackLocation(hero, SlotID(i)), crystalDragon, 5000);
	}
	else if(message == "vcminoldor") //all war machines
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		if(!hero) return;

		if(!hero->getArt(ArtifactPosition::MACH1))
			giveHeroNewArtifact(hero, VLC->arth->artifacts[ArtifactID::BALLISTA], ArtifactPosition::MACH1);
		if(!hero->getArt(ArtifactPosition::MACH2))
			giveHeroNewArtifact(hero, VLC->arth->artifacts[ArtifactID::AMMO_CART], ArtifactPosition::MACH2);
		if(!hero->getArt(ArtifactPosition::MACH3))
			giveHeroNewArtifact(hero, VLC->arth->artifacts[ArtifactID::FIRST_AID_TENT], ArtifactPosition::MACH3);
	}
	else if (message == "vcmiforgeofnoldorking") //hero gets all artifacts except war machines, spell scrolls and spell book
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		if(!hero) return;
		for (int g = 7; g < VLC->arth->artifacts.size(); ++g) //including artifacts from mods
			giveHeroNewArtifact(hero, VLC->arth->artifacts[g], ArtifactPosition::PRE_FIRST);
	}
	else if(message == "vcmiglorfindel") //selected hero gains a new level
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		changePrimSkill(hero, PrimarySkill::EXPERIENCE, VLC->heroh->reqExp(hero->level+1) - VLC->heroh->reqExp(hero->level));
	}
	else if(message == "vcminahar") //1000000 movement points
	{
		CGHeroInstance *hero = gs->getHero(currObj);
		if(!hero) return;
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = 1000000;
		sendAndApply(&smp);
	}
	else if(message == "vcmiformenos") //give resources
	{
		SetResources sr;
		sr.player = player;
		sr.res = gs->getPlayer(player)->resources;
		for(int i=0;i<Res::GOLD;i++)
			sr.res[i] += 100;
		sr.res[Res::GOLD] += 100000; //100k
		sendAndApply(&sr);
	}
	else if(message == "vcmieagles") //reveal FoW
	{
		FoWChange fc;
		fc.mode = 1;
		fc.player = player;
		auto  hlp_tab = new int3[gs->map->width * gs->map->height * (gs->map->twoLevel ? 2 : 1)];
		int lastUnc = 0;
		for(int i=0;i<gs->map->width;i++)
			for(int j=0;j<gs->map->height;j++)
				for(int k = 0; k < (gs->map->twoLevel ? 2 : 1); k++)
					if(!gs->getPlayerTeam(fc.player)->fogOfWarMap.at(i).at(j).at(k))
						hlp_tab[lastUnc++] = int3(i,j,k);
		fc.tiles.insert(hlp_tab, hlp_tab + lastUnc);
		delete [] hlp_tab;
		sendAndApply(&fc);
	}
	else if(message == "vcmisilmaril") //player wins
	{
		gs->getPlayer(player)->enteredWinningCheatCode = 1;
	}
	else if(message == "vcmimelkor") //player looses
	{
		gs->getPlayer(player)->enteredLosingCheatCode = 1;
	}
	else
		cheated = false;
	if(cheated)
	{
		SystemMessage temp_message(VLC->generaltexth->allTexts.at(260));
		sendAndApply(&temp_message);
		checkVictoryLossConditionsForPlayer(player);//Player enter win code or got required art\creature
	}
}

bool CGameHandler::makeCustomAction( BattleAction &ba )
{
	switch(ba.actionType)
	{
	case Battle::HERO_SPELL:
		{
			COMPLAIN_RET_FALSE_IF(ba.side > 1, "Side must be 0 or 1!");


			const CGHeroInstance *h = gs->curB->battleGetFightingHero(ba.side);
			if(!h)
			{
				logGlobal->warnStream() << "Wrong caster!";
				return false;
			}
			if(ba.additionalInfo >= VLC->spellh->objects.size())
			{
				logGlobal->warnStream() << "Wrong spell id (" << ba.additionalInfo << ")!";
				return false;
			}

			const CSpell * s = SpellID(ba.additionalInfo).toSpell();

			BattleSpellCastParameters parameters(gs->curB, h, s);
			parameters.aimToHex(ba.destinationTile);//todo: allow multiple destinations
			parameters.mode = ECastingMode::HERO_CASTING;
			parameters.selectedStack = gs->curB->battleGetStackByID(ba.selectedStack, false);

			ESpellCastProblem::ESpellCastProblem escp = gs->curB->battleCanCastThisSpell(h, s, ECastingMode::HERO_CASTING);//todo: should we check aimed cast(battleCanCastThisSpellHere)?
			if(escp != ESpellCastProblem::OK)
			{
				logGlobal->warnStream() << "Spell cannot be cast!";
				logGlobal->warnStream() << "Problem : " << escp;
				return false;
			}

			StartAction start_action(ba);
			sendAndApply(&start_action); //start spell casting

			s->battleCast(spellEnv, parameters);

			sendAndApply(&end_action);
			if( !gs->curB->battleGetStackByID(gs->curB->activeStack))
			{
				battleMadeAction.setn(true);
			}
			checkBattleStateChanges();
			if(battleResult.get())
			{
				battleMadeAction.setn(true);
				//battle will be ended by startBattle function
				//endBattle(gs->curB->tile, gs->curB->heroes[0], gs->curB->heroes[1]);
			}

			return true;

		}
	}
	return false;
}

void CGameHandler::stackTurnTrigger(const CStack * st)
{
	BattleTriggerEffect bte;
	bte.stackID = st->ID;
	bte.effect = -1;
	bte.val = 0;
	bte.additionalInfo = 0;
	if (st->alive())
	{
		//unbind
		if (st->getEffect (SpellID::BIND))
		{
			bool unbind = true;
			BonusList bl = *(st->getBonuses(Selector::type(Bonus::BIND_EFFECT)));
			std::set<const CStack*> stacks = gs->curB-> batteAdjacentCreatures(st);

			for(Bonus * b : bl)
			{
				const CStack * stack = gs->curB->battleGetStackByID(b->additionalInfo); //binding stack must be alive and adjacent
				if (stack)
				{
					if (vstd::contains(stacks, stack)) //binding stack is still present
					{
						unbind = false;
					}
				}
			}
			if (unbind)
			{
				BattleSetStackProperty ssp;
				ssp.which = BattleSetStackProperty::UNBIND;
				ssp.stackID = st->ID;
				sendAndApply(&ssp);
			}
		}
		//regeneration
		if(st->hasBonusOfType(Bonus::HP_REGENERATION))
		{
			bte.effect = Bonus::HP_REGENERATION;
			bte.val = std::min((int)(st->MaxHealth() - st->firstHPleft), st->valOfBonuses(Bonus::HP_REGENERATION));
		}
		if(st->hasBonusOfType(Bonus::FULL_HP_REGENERATION))
		{
			bte.effect = Bonus::HP_REGENERATION;
			bte.val = st->MaxHealth() - st->firstHPleft;
		}
		if (bte.val) //anything to heal
			sendAndApply(&bte);

		if(st->hasBonusOfType(Bonus::POISON))
		{
			const Bonus * b = st->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT, SpellID::POISON).And(Selector::type(Bonus::STACK_HEALTH)));
			if (b) //TODO: what if not?...
			{
				bte.val = std::max (b->val - 10, -(st->valOfBonuses(Bonus::POISON)));
				if (bte.val < b->val) //(negative) poison effect increases - update it
				{
					bte.effect = Bonus::POISON;
					sendAndApply(&bte);
				}
			}
		}
		if (st->hasBonusOfType(Bonus::MANA_DRAIN) && !vstd::contains(st->state, EBattleStackState::DRAINED_MANA))
		{
			const PlayerColor opponent = gs->curB->theOtherPlayer(st->owner);
			const CGHeroInstance * opponentHero = gs->curB->getHero(opponent);
			if (opponentHero)
			{
				ui32 manaDrained = st->valOfBonuses(Bonus::MANA_DRAIN);
				vstd::amin(manaDrained, opponentHero->mana);
				if (manaDrained)
				{
					bte.effect = Bonus::MANA_DRAIN;
					bte.val = manaDrained;
					bte.additionalInfo = opponentHero->id.getNum(); //for sanity
					sendAndApply(&bte);
				}
			}
		}
		if (st->isLiving() && !st->hasBonusOfType(Bonus::FEARLESS))
		{
			bool fearsomeCreature = false;
			for(CStack * stack : gs->curB->stacks)
			{
				if (stack->owner != st->owner && stack->alive() && stack->hasBonusOfType(Bonus::FEAR))
				{
					fearsomeCreature = true;
					break;
				}
			}
			if (fearsomeCreature)
			{
				if (gs->getRandomGenerator().nextInt(99) < 10) //fixed 10%
				{
					bte.effect = Bonus::FEAR;
					sendAndApply(&bte);
				}
			}
		}
		BonusList bl = *(st->getBonuses(Selector::type(Bonus::ENCHANTER)));
		int side = gs->curB->whatSide(st->owner);
		if (st->casts && !gs->curB->sides.at(side).enchanterCounter)
		{
			bool cast = false;
			while (!bl.empty() && !cast)
			{
				auto bonus = *RandomGeneratorUtil::nextItem(bl, gs->getRandomGenerator());
				auto spellID = SpellID(bonus->subtype);
				const CSpell * spell = SpellID(spellID).toSpell();
				bl.remove_if([&bonus](Bonus * b){return b==bonus;});

				if (gs->curB->battleCanCastThisSpell(st, spell, ECastingMode::ENCHANTER_CASTING) == ESpellCastProblem::OK)
				{
					BattleSpellCastParameters parameters(gs->curB, st, spell);
					parameters.spellLvl = bonus->val;
					parameters.effectLevel = bonus->val;//todo: recheck
					parameters.aimToHex(BattleHex::INVALID);
					parameters.mode = ECastingMode::ENCHANTER_CASTING;
					parameters.selectedStack = nullptr;

					spell->battleCast(spellEnv, parameters);

					//todo: move to mechanics
					BattleSetStackProperty ssp;
					ssp.which = BattleSetStackProperty::ENCHANTER_COUNTER;
					ssp.absolute = false;
					ssp.val = bonus->additionalInfo; //increase cooldown counter
					ssp.stackID = st->ID;
					sendAndApply(&ssp);

					cast = true;
				}
			};
		}
		bl = *(st->getBonuses(Selector::type(Bonus::ENCHANTED)));
		for (auto b : bl)
		{
			SetStackEffect sse;
			int val = bl.valOfBonuses (Selector::typeSubtype(b->type, b->subtype));
			if (val > 3)
			{
				for (auto s : gs->curB->battleGetAllStacks())
				{
					if (st->owner == s->owner && s->isValidTarget()) //all allied
						sse.stacks.push_back (s->ID);
				}
			}
			else
				sse.stacks.push_back (st->ID);

			Bonus pseudoBonus;
			pseudoBonus.sid = b->subtype;
			pseudoBonus.val = ((val > 3) ?  (val - 3) : val);
			pseudoBonus.turnsRemain = 50;
			st->stackEffectToFeature (sse.effect, pseudoBonus);
			if (sse.effect.size())
				sendAndApply (&sse);
		}
	}
}

void CGameHandler::handleDamageFromObstacle(const CObstacleInstance &obstacle, const CStack * curStack)
{
	//we want to determine following vars depending on obstacle type
	int damage = -1;
	int effect = -1;
	bool oneTimeObstacle = false;

	//helper info
	const SpellCreatedObstacle *spellObstacle = dynamic_cast<const SpellCreatedObstacle*>(&obstacle); //not nice but we may need spell params
	const ui8 side = !curStack->attackerOwned; //if enemy is defending (false = 0), side of enemy hero is 1 (true)
	const CGHeroInstance *hero = gs->curB->battleGetFightingHero(side);//FIXME: there may be no hero - landmines in Tower

	if(obstacle.obstacleType == CObstacleInstance::MOAT)
	{
		damage = battleGetMoatDmg();
	}
	else if(obstacle.obstacleType == CObstacleInstance::LAND_MINE)
	{
		//You don't get hit by a Mine you can see.
		if(gs->curB->battleIsObstacleVisibleForSide(obstacle, (BattlePerspective::BattlePerspective)side))
			return;

		oneTimeObstacle = true;
		effect = 82; //makes

		const CSpell * sp = SpellID(SpellID::LAND_MINE).toSpell();

		if(sp->isImmuneByStack(hero, curStack))
			return;

		damage = sp->calculateDamage(hero, curStack,
											 spellObstacle->spellLevel, spellObstacle->casterSpellPower);
		//TODO even if obstacle wasn't created by hero (Tower "moat") it should deal dmg as if cast by hero,
		//if it is bigger than default dmg. Or is it just irrelevant H3 implementation quirk
	}
	else if(obstacle.obstacleType == CObstacleInstance::FIRE_WALL)
	{
		const CSpell * sp = SpellID(SpellID::FIRE_WALL).toSpell();

		if(sp->isImmuneByStack(hero, curStack))
			return;

		damage = sp->calculateDamage(hero, curStack,
											 spellObstacle->spellLevel, spellObstacle->casterSpellPower);
	}
	else
	{
		//no other obstacle does damage to stack
		return;
	}

	BattleStackAttacked bsa;
	if(effect >= 0)
	{
		bsa.flags |= BattleStackAttacked::EFFECT;
		bsa.effect = effect; //makes POOF
	}
	bsa.damageAmount = damage;
	bsa.stackAttacked = curStack->ID;
	bsa.attackerID = -1;
	curStack->prepareAttacked(bsa, gameState()->getRandomGenerator());

	StacksInjured si;
	si.stacks.push_back(bsa);
	sendAndApply(&si);

	if(oneTimeObstacle)
		removeObstacle(obstacle);
}

void CGameHandler::handleTimeEvents()
{
	gs->map->events.sort(evntCmp);
	while(gs->map->events.size() && gs->map->events.front().firstOccurence+1 == gs->day)
	{
		CMapEvent ev = gs->map->events.front();

		for (int player = 0; player < PlayerColor::PLAYER_LIMIT_I; player++)
		{
			auto color = PlayerColor(player);

			PlayerState *pinfo = gs->getPlayer(color, false); //do not output error if player does not exist

			if( pinfo  //player exists
				&& (ev.players & 1<<player) //event is enabled to this player
				&& ((ev.computerAffected && !pinfo->human)
					|| (ev.humanAffected && pinfo->human)
				)
			)
			{
				//give resources
				SetResources sr;
				sr.player = color;
				sr.res = pinfo->resources + ev.resources;

				//prepare dialog
				InfoWindow iw;
				iw.player = color;
				iw.text << ev.message;

				for (int i=0; i<ev.resources.size(); i++)
				{
					if(ev.resources.at(i)) //if resource is changed, we add it to the dialog
						iw.components.push_back(Component(Component::RESOURCE,i,ev.resources.at(i),0));
				}

				if (iw.components.size())
				{
					sr.res.amax(0); // If removing too much resources, adjust the amount so the total doesn't become negative.
					sendAndApply(&sr); //update player resources if changed
				}

				sendAndApply(&iw); //show dialog
			}
		} //PLAYERS LOOP

		if(ev.nextOccurence)
		{
			gs->map->events.pop_front();

			ev.firstOccurence += ev.nextOccurence;
			auto it = gs->map->events.begin();
			while(it != gs->map->events.end() && it->earlierThanOrEqual(ev))
				it++;
			gs->map->events.insert(it, ev);
		}
		else
		{
			gs->map->events.pop_front();
		}
	}

	//TODO send only if changed
	UpdateMapEvents ume;
	ume.events = gs->map->events;
	sendAndApply(&ume);
}

void CGameHandler::handleTownEvents(CGTownInstance * town, NewTurn &n)
{
	town->events.sort(evntCmp);
	while(town->events.size() && town->events.front().firstOccurence == gs->day)
	{
		PlayerColor player = town->tempOwner;
		CCastleEvent ev = town->events.front();
		PlayerState *pinfo = gs->getPlayer(player, false);

		if( pinfo  //player exists
			&& (ev.players & 1<<player.getNum()) //event is enabled to this player
			&& ((ev.computerAffected && !pinfo->human)
				|| (ev.humanAffected && pinfo->human) ) )
		{


			// dialog
			InfoWindow iw;
			iw.player = player;
			iw.text << ev.message;

			if(ev.resources.nonZero())
			{
				TResources was = n.res[player];
				n.res[player] += ev.resources;
				n.res[player].amax(0);

				for (int i=0; i<ev.resources.size(); i++)
					if(ev.resources.at(i) && pinfo->resources.at(i) != n.res.at(player).at(i)) //if resource had changed, we add it to the dialog
						iw.components.push_back(Component(Component::RESOURCE,i,n.res.at(player).at(i)-was.at(i),0));

			}

			for(auto & i : ev.buildings)
			{
				if(!town->hasBuilt(i))
				{
					buildStructure(town->id, i, true);
					iw.components.push_back(Component(Component::BUILDING, town->subID, i, 0));
				}
			}

			if (!ev.creatures.empty() && !vstd::contains(n.cres, town->id))
			{
				n.cres[town->id].tid = town->id;
				n.cres[town->id].creatures = town->creatures;
			}
			auto & sac = n.cres[town->id];

			for(si32 i=0;i<ev.creatures.size();i++) //creature growths
			{
				if(!town->creatures.at(i).second.empty() && ev.creatures.at(i) > 0)//there is dwelling
				{
					sac.creatures[i].first += ev.creatures.at(i);
					iw.components.push_back(Component(Component::CREATURE,
							town->creatures.at(i).second.back(), ev.creatures.at(i), 0));
				}
			}
			sendAndApply(&iw); //show dialog
		}

		if(ev.nextOccurence)
		{
			town->events.pop_front();

			ev.firstOccurence += ev.nextOccurence;
			auto it = town->events.begin();
			while(it != town->events.end() && it->earlierThanOrEqual(ev))
				it++;
			town->events.insert(it, ev);
		}
		else
		{
			town->events.pop_front();
		}
	}

	//TODO send only if changed
	UpdateCastleEvents uce;
	uce.town = town->id;
	uce.events = town->events;
	sendAndApply(&uce);
}

bool CGameHandler::complain( const std::string &problem )
{
	sendMessageToAll("Server encountered a problem: " + problem);
	logGlobal->errorStream() << problem;
	return true;
}

void CGameHandler::showGarrisonDialog( ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits)
{
	//PlayerColor player = getOwner(hid);
	auto upperArmy = dynamic_cast<const CArmedInstance*>(getObj(upobj));
	auto lowerArmy = dynamic_cast<const CArmedInstance*>(getObj(hid));

	assert(lowerArmy);
	assert(upperArmy);

	auto garrisonQuery = std::make_shared<CGarrisonDialogQuery>(upperArmy, lowerArmy);
	queries.addQuery(garrisonQuery);

	GarrisonDialog gd;
	gd.hid = hid;
	gd.objid = upobj;
	gd.removableUnits = removableUnits;
	gd.queryID = garrisonQuery->queryID;
	sendAndApply(&gd);
}

void CGameHandler::showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId)
{
	OpenWindow ow;
	ow.window = OpenWindow::THIEVES_GUILD;
	ow.id1 = player.getNum();
	ow.id2 = requestingObjId.getNum();
	sendAndApply(&ow);
}

bool CGameHandler::isAllowedExchange( ObjectInstanceID id1, ObjectInstanceID id2 )
{
	if(id1 == id2)
		return true;

	const CGObjectInstance *o1 = getObj(id1), *o2 = getObj(id2);
	if(!o1 || !o2)
		return true; //arranging stacks within an object should be always allowed

	if (o1 && o2)
	{
		if(o1->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o1);
			if(t->visitingHero == o2  ||  t->garrisonHero == o2)
				return true;
		}
		if(o2->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o2);
			if(t->visitingHero == o1  ||  t->garrisonHero == o1)
				return true;
		}

		if (o1->ID == Obj::HERO && o2->ID == Obj::HERO)
		{
			const CGHeroInstance *h1 = static_cast<const CGHeroInstance*>(o1);
			const CGHeroInstance *h2 = static_cast<const CGHeroInstance*>(o2);

			// two heroes in same town (garrisoned and visiting)
			if (h1->visitedTown != nullptr && h2->visitedTown != nullptr && h1->visitedTown == h2->visitedTown)
				return true;
		}

		//Ongoing garrison exchange
		if(auto dialog = std::dynamic_pointer_cast<CGarrisonDialogQuery>(queries.topQuery(o1->tempOwner)))
		{
			if(dialog->exchangingArmies.at(0) == o1 && dialog->exchangingArmies.at(1) == o2)
				return true;

			if(dialog->exchangingArmies.at(1) == o1 && dialog->exchangingArmies.at(0) == o2)
				return true;
		}
	}

	return false;
}

void CGameHandler::objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h )
{
	logGlobal->debugStream()  << h->nodeName() << " visits " << obj->getObjectName() << "(" << obj->ID << ":" << obj->subID << ")";
	auto visitQuery = std::make_shared<CObjectVisitQuery>(obj, h, obj->visitablePos());
	queries.addQuery(visitQuery); //TODO real visit pos

	HeroVisit hv;
	hv.obj = obj;
	hv.hero = h;
	hv.player = h->tempOwner;
	hv.starting = true;
	sendAndApply(&hv);

	obj->onHeroVisit(h);

	queries.popIfTop(visitQuery); //visit ends here if no queries were created
}

void CGameHandler::objectVisitEnded(const CObjectVisitQuery &query)
{
	logGlobal->traceStream() << query.visitingHero->nodeName() << " visit ends.\n";

	HeroVisit hv;
	hv.player = query.players.front();
	hv.obj = nullptr; //not necessary, moreover may have been deleted in the meantime
	hv.hero = query.visitingHero;
	assert(hv.hero);
	hv.starting = false;
	sendAndApply(&hv);
}

bool CGameHandler::buildBoat( ObjectInstanceID objid )
{
	const IShipyard *obj = IShipyard::castFrom(getObj(objid));

	if(obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		complain("Cannot build boat in this shipyard!");
		return false;
	}
	else if(obj->o->ID == Obj::TOWN
	        && !static_cast<const CGTownInstance*>(obj)->hasBuilt(BuildingID::SHIPYARD))
	{
		complain("Cannot build boat in the town - no shipyard!");
		return false;
	}

	const PlayerColor playerID = obj->o->tempOwner;
	TResources boatCost;
	obj->getBoatCost(boatCost);
	TResources aviable = gs->getPlayer(playerID)->resources;

	if (!aviable.canAfford(boatCost))
	{
		complain("Not enough resources to build a boat!");
		return false;
	}

	int3 tile = obj->bestLocation();
	if(!gs->map->isInTheMap(tile))
	{
		complain("Cannot find appropriate tile for a boat!");
		return false;
	}

	//take boat cost
	SetResources sr;
	sr.player = playerID;
	sr.res = (aviable - boatCost);
	sendAndApply(&sr);

	//create boat
	NewObject no;
	no.ID = Obj::BOAT;
	no.subID = obj->getBoatType();
	no.pos = tile + int3(1,0,0);
	sendAndApply(&no);

	return true;
}

void CGameHandler::engageIntoBattle( PlayerColor player )
{
	//notify interfaces
	PlayerBlocked pb;
	pb.player = player;
	pb.reason = PlayerBlocked::UPCOMING_BATTLE;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	sendAndApply(&pb);
}

void CGameHandler::checkVictoryLossConditions(const std::set<PlayerColor> & playerColors)
{
	for(auto playerColor : playerColors)
	{
		if(gs->getPlayer(playerColor, false))
			checkVictoryLossConditionsForPlayer(playerColor);
	}
}

void CGameHandler::checkVictoryLossConditionsForAll()
{
	std::set<PlayerColor> playerColors;
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		playerColors.insert(PlayerColor(i));
	}
	checkVictoryLossConditions(playerColors);
}

void CGameHandler::checkVictoryLossConditionsForPlayer(PlayerColor player)
{
	const PlayerState *p = gs->getPlayer(player);
	if(p->status != EPlayerStatus::INGAME) return;

	auto victoryLossCheckResult = gs->checkForVictoryAndLoss(player);

	if(victoryLossCheckResult.victory() || victoryLossCheckResult.loss())
	{
		InfoWindow iw;
		getVictoryLossMessage(player, victoryLossCheckResult, iw);
		sendAndApply(&iw);

		PlayerEndsGame peg;
		peg.player = player;
		peg.victoryLossCheckResult = victoryLossCheckResult;
		sendAndApply(&peg);

		if(victoryLossCheckResult.victory())
		{
			//one player won -> all enemies lost
			for (auto i = gs->players.cbegin(); i!=gs->players.cend(); i++)
			{
				if(i->first != player && gs->getPlayer(i->first)->status == EPlayerStatus::INGAME)
				{
					peg.player = i->first;
					peg.victoryLossCheckResult = gameState()->getPlayerRelations(player, i->first) == PlayerRelations::ALLIES ?
								victoryLossCheckResult : victoryLossCheckResult.invert(); // ally of winner

					InfoWindow iw;
					getVictoryLossMessage(player, peg.victoryLossCheckResult, iw);
					iw.player = i->first;

					sendAndApply(&iw);
					sendAndApply(&peg);
				}
			}

			if(p->human)
			{
				end2 = true;

				if(gs->scenarioOps->campState)
				{
					std::vector<CGHeroInstance *> crossoverHeroes;
					for(CGHeroInstance * hero : gs->map->heroesOnMap)
					{
						if(hero->tempOwner == player)
						{
							// keep all heroes from the winning player
							crossoverHeroes.push_back(hero);
						}
						else if(vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(hero->subID)))
						{
							// keep hero whether lost or won (like Xeron in AB campaign)
							crossoverHeroes.push_back(hero);
						}
					}
					// keep lost heroes which are in heroes pool
					for(auto & heroPair : gs->hpool.heroesPool)
					{
						if(vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(heroPair.first)))
						{
							crossoverHeroes.push_back(heroPair.second.get());
						}
					}

					gs->scenarioOps->campState->setCurrentMapAsConquered(crossoverHeroes);

					//Request clients to change connection mode
					PrepareForAdvancingCampaign pfac;
					sendAndApply(&pfac);
					//Change connection mode
					if(getPlayer(player)->human && getStartInfo()->campState)
					{
						for(auto connection : conns)
							connection->prepareForSendingHeroes();
					}

					UpdateCampaignState ucs;
					ucs.camp = gs->scenarioOps->campState;
					sendAndApply(&ucs);
				}
			}
		}
		else
		{
			//copy heroes vector to avoid iterator invalidation as removal change PlayerState
			auto hlp = p->heroes;
			for(auto h : hlp) //eliminate heroes
			{
				if(h.get())
					removeObject(h);
			}

			//player lost -> all his objects become unflagged (neutral)
			for (auto obj : gs->map->objects) //unflag objs
			{
				if(obj.get() && obj->tempOwner == player)
					setOwner(obj, PlayerColor::NEUTRAL);
			}

			//eliminating one player may cause victory of another:
			std::set<PlayerColor> playerColors;

			//do not copy player state (CBonusSystemNode) by value
			for (auto &p : gs->players) //players may have different colors, iterate over players and not integers
			{
				if (p.first != player)
					playerColors.insert(p.first);
			}

			//notify all players
			for (auto pc : playerColors)
			{
				if (gs->getPlayer(pc)->status == EPlayerStatus::INGAME)
				{
					InfoWindow iw;
					getVictoryLossMessage(player, victoryLossCheckResult.invert(), iw);
					iw.player = pc;
					sendAndApply(&iw);
				}
			}
			checkVictoryLossConditions(playerColors);
		}

		auto playerInfo = gs->getPlayer(gs->currentPlayer, false);
		// If we are called before the actual game start, there might be no current player
		if (playerInfo && playerInfo->status != EPlayerStatus::INGAME)
		{
			// If player making turn has lost his turn must be over as well
			states.setFlag(gs->currentPlayer, &PlayerStatus::makingTurn, false);
		}
	}
}

void CGameHandler::getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const
{
	out.player = player;
	out.text.clear();
	out.text << victoryLossCheckResult.messageToSelf;
	// hackish, insert one player-specific string, if applicable
	if (victoryLossCheckResult.messageToSelf.find("%s") != std::string::npos)
		out.text.addReplacement(MetaString::COLOR, player.getNum());

	out.components.push_back(Component(Component::FLAG, player.getNum(), 0, 0));
}

bool CGameHandler::dig( const CGHeroInstance *h )
{
	for (auto i = gs->map->objects.cbegin(); i != gs->map->objects.cend(); i++) //unflag objs
	{
		if(*i && (*i)->ID == Obj::HOLE  &&  (*i)->pos == h->getPosition())
		{
			complain("Cannot dig - there is already a hole under the hero!");
			return false;
		}
	}

	if(h->diggingStatus() != EDiggingStatus::CAN_DIG) //checks for terrain and movement
		COMPLAIN_RETF("Hero cannot dig (error code %d)!", h->diggingStatus());

	//create a hole
	NewObject no;
	no.ID = Obj::HOLE;
	no.pos = h->getPosition();
	no.subID = 0;
	sendAndApply(&no);

	//take MPs
	SetMovePoints smp;
	smp.hid = h->id;
	smp.val = 0;
	sendAndApply(&smp);

	InfoWindow iw;
	iw.player = h->tempOwner;
	if(gs->map->grailPos == h->getPosition())
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 58); //"Congratulations! After spending many hours digging here, your hero has uncovered the "
		iw.text.addTxt(MetaString::ART_NAMES, ArtifactID::GRAIL);
		iw.soundID = soundBase::ULTIMATEARTIFACT;
		giveHeroNewArtifact(h, VLC->arth->artifacts[ArtifactID::GRAIL], ArtifactPosition::PRE_FIRST); //give grail
		sendAndApply(&iw);

		iw.soundID = soundBase::invalid;
		iw.text.clear();
		iw.text.addTxt(MetaString::ART_DESCR, ArtifactID::GRAIL);
		sendAndApply(&iw);
	}
	else
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 59); //"Nothing here. \n Where could it be?"
		iw.soundID = soundBase::Dig;
		sendAndApply(&iw);
	}

	return true;
}

void CGameHandler::attackCasting(const BattleAttack & bat, Bonus::BonusType attackMode, const CStack * attacker)
{
	if(attacker->hasBonusOfType(attackMode))
	{
		std::set<SpellID> spellsToCast;
		TBonusListPtr spells = attacker->getBonuses(Selector::type(attackMode));
		for(const Bonus *sf : *spells)
		{
			spellsToCast.insert (SpellID(sf->subtype));
		}
		for(SpellID spellID : spellsToCast)
		{
			const CStack * oneOfAttacked = nullptr;
			for (auto & elem : bat.bsa)
			{
				if (elem.newAmount > 0 && !elem.isSecondary()) //apply effects only to first target stack if it's alive
				{
					oneOfAttacked = gs->curB->battleGetStackByID(elem.stackAttacked);
					break;
				}
			}
			bool castMe = false;
			if(oneOfAttacked == nullptr) //all attacked creatures have been killed
				return;
			int spellLevel = 0;
			TBonusListPtr spellsByType = attacker->getBonuses(Selector::typeSubtype(attackMode, spellID));
			for(const Bonus *sf : *spellsByType)
			{
				vstd::amax(spellLevel, sf->additionalInfo % 1000); //pick highest level
				int meleeRanged = sf->additionalInfo / 1000;
				if (meleeRanged == 0 || (meleeRanged == 1 && bat.shot()) || (meleeRanged == 2 && !bat.shot()))
					castMe = true;
			}
			int chance = attacker->valOfBonuses((Selector::typeSubtype(attackMode, spellID)));
			vstd::amin (chance, 100);

			const CSpell * spell = SpellID(spellID).toSpell();
			if(gs->curB->battleCanCastThisSpellHere(attacker, spell, ECastingMode::AFTER_ATTACK_CASTING, oneOfAttacked->position) != ESpellCastProblem::OK)
				continue;

			//check if spell should be cast (probability handling)
			if(gs->getRandomGenerator().nextInt(99) >= chance)
				continue;

			//casting
			if (castMe) //stacks use 0 spell power. If needed, default = 3 or custom value is used
			{
				BattleSpellCastParameters parameters(gs->curB, attacker, spell);
				parameters.spellLvl = spellLevel;
				parameters.effectLevel = spellLevel;
				parameters.aimToStack(oneOfAttacked);
				parameters.mode = ECastingMode::AFTER_ATTACK_CASTING;
				parameters.selectedStack = nullptr;

				spell->battleCast(spellEnv, parameters);
			}
		}
	}
}

void CGameHandler::handleAttackBeforeCasting (const BattleAttack & bat)
{
	const CStack * attacker = gs->curB->battleGetStackByID(bat.stackAttacking);
	attackCasting(bat, Bonus::SPELL_BEFORE_ATTACK, attacker); //no death stare / acid breath needed?
}

void CGameHandler::handleAfterAttackCasting( const BattleAttack & bat )
{
	const CStack * attacker = gs->curB->battleGetStackByID(bat.stackAttacking);
	if (!attacker) //could be already dead
		return;

	auto cast = [=](SpellID spellID, int power)
	{
		const CSpell * spell = SpellID(spellID).toSpell();

		BattleSpellCastParameters parameters(gs->curB, attacker, spell);
		parameters.spellLvl = 0;
		parameters.effectLevel = 0;
		parameters.aimToStack(gs->curB->battleGetStackByID(bat.bsa.at(0).stackAttacked));
		parameters.effectPower = power;
		parameters.mode = ECastingMode::AFTER_ATTACK_CASTING;
		parameters.selectedStack = nullptr;

		spell->battleCast(this->spellEnv, parameters);
	};

	attackCasting(bat, Bonus::SPELL_AFTER_ATTACK, attacker);

	if(bat.bsa.at(0).newAmount <= 0)
	{
		//don't try death stare or acid breath on dead stack (crash!)
		return;
	}

	if (attacker->hasBonusOfType(Bonus::DEATH_STARE) && bat.bsa.size())
	{
		// mechanics of Death Stare as in H3:
		// each gorgon have 10% chance to kill (counted separately in H3) -> binomial distribution
		//original formula x = min(x, (gorgons_count + 9)/10);

		double chanceToKill = attacker->valOfBonuses(Bonus::DEATH_STARE, 0) / 100.0f;
		vstd::amin(chanceToKill, 1); //cap at 100%

		std::binomial_distribution<> distribution(attacker->count, chanceToKill);
		std::mt19937 rng(std::time(nullptr));

		int staredCreatures = distribution(rng);

		double cap = 1 / std::max(chanceToKill, (double)(0.01));//don't divide by 0
		int maxToKill = (attacker->count + cap - 1) / cap; //not much more than chance * count
		vstd::amin(staredCreatures, maxToKill);

		staredCreatures += (attacker->level() * attacker->valOfBonuses(Bonus::DEATH_STARE, 1)) / gs->curB->battleGetStackByID(bat.bsa.at(0).stackAttacked)->level();
		if (staredCreatures)
		{
			if (bat.bsa.at(0).newAmount > 0) //TODO: death stare was not originally available for multiple-hex attacks, but...
				cast(SpellID::DEATH_STARE, staredCreatures);
		}
	}

	int acidDamage = 0;
	TBonusListPtr acidBreath = attacker->getBonuses(Selector::type(Bonus::ACID_BREATH));
	for(const Bonus *b : *acidBreath)
	{
		if (b->additionalInfo > gs->getRandomGenerator().nextInt(99))
			acidDamage += b->val;
	}
	if (acidDamage)
	{
		cast(SpellID::ACID_BREATH_DAMAGE, acidDamage * attacker->count);
	}
}

bool CGameHandler::castSpell(const CGHeroInstance *h, SpellID spellID, const int3 &pos)
{
	const CSpell *s = spellID.toSpell();

	AdventureSpellCastParameters p;
	p.caster = h;
	p.pos = pos;

	return s->adventureCast(spellEnv, p);
}

void CGameHandler::visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h)
{
	if (!t.visitableObjects.empty())
	{
		//to prevent self-visiting heroes on space press
		if(t.visitableObjects.back() != h)
			objectVisited(t.visitableObjects.back(), h);
		else if(t.visitableObjects.size() > 1)
			objectVisited(*(t.visitableObjects.end()-2),h);
	}
}

bool CGameHandler::sacrificeCreatures(const IMarket *market, const CGHeroInstance *hero, SlotID slot, ui32 count)
{
	int oldCount = hero->getStackCount(slot);

	if(oldCount < count)
		COMPLAIN_RET("Not enough creatures to sacrifice!")
	else if(oldCount == count && hero->stacksCount() == 1 && hero->needsLastStack())
		COMPLAIN_RET("Cannot sacrifice last creature!");

	int crid = hero->getStack(slot).type->idNumber;

	changeStackCount(StackLocation(hero, slot), -count);

	int dump, exp;
	market->getOffer(crid, 0, dump, exp, EMarketMode::CREATURE_EXP);
	exp *= count;
	changePrimSkill(hero, PrimarySkill::EXPERIENCE, hero->calculateXp(exp));

	return true;
}

bool CGameHandler::sacrificeArtifact(const IMarket * m, const CGHeroInstance * hero, ArtifactPosition slot)
{
	ArtifactLocation al(hero, slot);
	const CArtifactInstance *a = al.getArt();

	if(!a)
		COMPLAIN_RET("Cannot find artifact to sacrifice!");


	int dmp, expToGive;
	m->getOffer(hero->getArtTypeId(slot), 0, dmp, expToGive, EMarketMode::ARTIFACT_EXP);

	removeArtifact(al);
	changePrimSkill(hero, PrimarySkill::EXPERIENCE, expToGive);
	return true;
}

void CGameHandler::makeStackDoNothing(const CStack * next)
{
	BattleAction doNothing;
	doNothing.actionType = Battle::NO_ACTION;
	doNothing.additionalInfo = 0;
	doNothing.destinationTile = -1;
	doNothing.side = !next->attackerOwned;
	doNothing.stackNumber = next->ID;

	makeAutomaticAction(next, doNothing);
}

bool CGameHandler::insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	if(sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Slot is already taken!");

	if(!sl.slot.validSlot())
		COMPLAIN_RET("Cannot insert stack to that slot!");

	InsertNewStack ins;
	ins.sl = sl;
	ins.stack = CStackBasicDescriptor(c, count);
	sendAndApply(&ins);
	return true;
}

bool CGameHandler::eraseStack(const StackLocation &sl, bool forceRemoval/* = false*/)
{
	if(!sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to erase");

	if(sl.army->stacksCount() == 1 //from the last stack
		&& sl.army->needsLastStack() //that must be left
		&& !forceRemoval) //ignore above conditions if we are forcing removal
	{
		COMPLAIN_RET("Cannot erase the last stack!");
	}

	EraseStack es;
	es.sl = sl;
	sendAndApply(&es);
	return true;
}

bool CGameHandler::changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue /*= false*/)
{
	TQuantity currentCount = sl.army->getStackCount(sl.slot);
	if((absoluteValue && count < 0)
		|| (!absoluteValue && -count > currentCount))
	{
		COMPLAIN_RET("Cannot take more stacks than present!");
	}

	if((currentCount == -count  &&  !absoluteValue)
	   || (!count && absoluteValue))
	{
		eraseStack(sl);
	}
	else
	{
		ChangeStackCount csc;
		csc.sl = sl;
		csc.count = count;
		csc.absoluteValue = absoluteValue;
		sendAndApply(&csc);
	}
	return true;
}

bool CGameHandler::addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	const CCreature *slotC = sl.army->getCreature(sl.slot);
	if(!slotC) //slot is empty
		insertNewStack(sl, c, count);
	else if(c == slotC)
		changeStackCount(sl, count);
	else
	{
		COMPLAIN_RET("Cannot add " + c->namePl + " to slot " + boost::lexical_cast<std::string>(sl.slot) + "!");
	}
	return true;
}

void CGameHandler::tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging)
{
	if(removeObjWhenFinished)
		removeAfterVisit(src);

	if(!src->canBeMergedWith(*dst, allowMerging))
	{
		if (allowMerging) //do that, add all matching creatures.
		{
			bool cont = true;
			while (cont)
			{
				for(auto i = src->stacks.begin(); i != src->stacks.end(); i++)//while there are unmoved creatures
				{
					SlotID pos = dst->getSlotFor(i->second->type);
					if(pos.validSlot())
					{
						moveStack(StackLocation(src, i->first), StackLocation(dst, pos));
						cont = true;
						break; //or iterator crashes
					}
					cont = false;
				}
			}
		}
		showGarrisonDialog(src->id, dst->id, true); //show garrison window and optionally remove ourselves from map when player ends
	}
	else //merge
	{
		moveArmy(src, dst, allowMerging);
	}
}

bool CGameHandler::moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count)
{
	if(!src.army->hasStackAtSlot(src.slot))
		COMPLAIN_RET("No stack to move!");

	if(dst.army->hasStackAtSlot(dst.slot) && dst.army->getCreature(dst.slot) != src.army->getCreature(src.slot))
		COMPLAIN_RET("Cannot move: stack of different type at destination pos!");

	if(!dst.slot.validSlot())
		COMPLAIN_RET("Cannot move stack to that slot!");

	if(count == -1)
	{
		count = src.army->getStackCount(src.slot);
	}

	if(src.army != dst.army  //moving away
		&&  count == src.army->getStackCount(src.slot) //all creatures
		&& src.army->stacksCount() == 1 //from the last stack
		&& src.army->needsLastStack()) //that must be left
	{
		COMPLAIN_RET("Cannot move away the last creature!");
	}

	RebalanceStacks rs;
	rs.src = src;
	rs.dst = dst;
	rs.count = count;
	sendAndApply(&rs);
	return true;
}

bool CGameHandler::swapStacks(const StackLocation &sl1, const StackLocation &sl2)
{

	if(!sl1.army->hasStackAtSlot(sl1.slot))
		return moveStack(sl2, sl1);
	else if(!sl2.army->hasStackAtSlot(sl2.slot))
		return moveStack(sl1, sl2);
	else
	{
		SwapStacks ss;
		ss.sl1 = sl1;
		ss.sl2 = sl2;
		sendAndApply(&ss);
		return true;
	}
}

void CGameHandler::runBattle()
{
	setBattle(gs->curB);
	assert(gs->curB);
	//TODO: pre-tactic stuff, call scripts etc.

	//tactic round
	{
		while(gs->curB->tacticDistance && !battleResult.get())
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}

	//spells opening battle
	for(int i = 0; i < 2; ++i)
	{
		auto h = gs->curB->battleGetFightingHero(i);
		if(h && h->hasBonusOfType(Bonus::OPENING_BATTLE_SPELL))
		{
			TBonusListPtr bl = h->getBonuses(Selector::type(Bonus::OPENING_BATTLE_SPELL));

			for (Bonus *b : *bl)
			{
				const CSpell * spell = SpellID(b->subtype).toSpell();
				BattleSpellCastParameters parameters(gs->curB, h, spell);
				parameters.spellLvl = 3;
				parameters.effectLevel = 3;
				parameters.aimToHex(BattleHex::INVALID);
				parameters.mode = ECastingMode::PASSIVE_CASTING;
				parameters.selectedStack = nullptr;
				parameters.enchantPower = b->val;
				spell->battleCast(spellEnv, parameters);
			}
		}
	}

	//main loop
	while(!battleResult.get()) //till the end of the battle ;]
	{
		NEW_ROUND;
		auto obstacles = gs->curB->obstacles; //we copy container, because we're going to modify it
		for(auto &obstPtr : obstacles)
		{
			if(const SpellCreatedObstacle *sco = dynamic_cast<const SpellCreatedObstacle *>(obstPtr.get()))
				if(sco->turnsRemaining == 0)
					removeObstacle(*obstPtr);
		}

		const BattleInfo & curB = *gs->curB;

		//stack loop

		const CStack *next;
		while(!battleResult.get() && (next = curB.getNextStack()) && next->willMove())
		{

			std::set <const CStack *> stacksToRemove;
			for(auto stack : curB.stacks)
			{
				if(vstd::contains(stack->state, EBattleStackState::GHOST_PENDING))
					stacksToRemove.insert(stack);
			}

			for(auto stack : stacksToRemove)
			{
				BattleStacksRemoved bsr;
				bsr.stackIDs.insert(stack->ID);
				sendAndApply(&bsr);
			}

			//check for bad morale => freeze
			int nextStackMorale = next->MoraleVal();
			if( nextStackMorale < 0 &&
				!(NBonus::hasOfType(gs->curB->battleGetFightingHero(0), Bonus::BLOCK_MORALE)
				   || NBonus::hasOfType(gs->curB->battleGetFightingHero(1), Bonus::BLOCK_MORALE)) //checking if gs->curB->heroes have (or don't have) morale blocking bonuses)
				)
			{
				if(gs->getRandomGenerator().nextInt(23) < -2 * nextStackMorale)
				{
					//unit loses its turn - empty freeze action
					BattleAction ba;
					ba.actionType = Battle::BAD_MORALE;
					ba.additionalInfo = 1;
					ba.side = !next->attackerOwned;
					ba.stackNumber = next->ID;

					makeAutomaticAction(next, ba);
					continue;
				}
			}

			if(next->hasBonusOfType(Bonus::ATTACKS_NEAREST_CREATURE)) //while in berserk
			{ //fixme: stack should not attack itself
				std::pair<const CStack *, int> attackInfo = curB.getNearestStack(next, boost::logic::indeterminate);
				if(attackInfo.first != nullptr)
				{
					BattleAction attack;
					attack.actionType = Battle::WALK_AND_ATTACK;
					attack.side = !next->attackerOwned;
					attack.stackNumber = next->ID;
					attack.additionalInfo = attackInfo.first->position;
					attack.destinationTile = attackInfo.second;

					makeAutomaticAction(next, attack);
				}
				else
				{
					makeStackDoNothing(next);
				}
				continue;
			}

			const CGHeroInstance * curOwner = gs->curB->battleGetOwner(next);

			if( (next->position < 0 || next->getCreature()->idNumber == CreatureID::BALLISTA)	//arrow turret or ballista
				&& (!curOwner || curOwner->getSecSkillLevel(SecondarySkill::ARTILLERY) == 0)) //hero has no artillery
			{
				BattleAction attack;
				attack.actionType = Battle::SHOOT;
				attack.side = !next->attackerOwned;
				attack.stackNumber = next->ID;

				for(auto & elem : gs->curB->stacks)
				{
					if(elem->owner != next->owner && elem->isValidTarget())
					{
						attack.destinationTile = elem->position;
						break;
					}
				}

				makeAutomaticAction(next, attack);
				continue;
			}

			if(next->getCreature()->idNumber == CreatureID::CATAPULT)
			{
				const auto & attackableBattleHexes = curB.getAttackableBattleHexes();

				if(attackableBattleHexes.empty())
				{
					makeStackDoNothing(next);
					continue;
				}

				if(!curOwner || curOwner->getSecSkillLevel(SecondarySkill::BALLISTICS) == 0)
				{
					BattleAction attack;
					attack.destinationTile = *RandomGeneratorUtil::nextItem(attackableBattleHexes,
												gs->getRandomGenerator());
					attack.actionType = Battle::CATAPULT;
					attack.additionalInfo = 0;
					attack.side = !next->attackerOwned;
					attack.stackNumber = next->ID;

					makeAutomaticAction(next, attack);
					continue;
				}
			}

			if(next->getCreature()->idNumber == CreatureID::FIRST_AID_TENT)
			{
				TStacks possibleStacks = battleGetStacksIf([=](const CStack * s)
				{
					return s->owner == next->owner && s->canBeHealed();
				});

				if(!possibleStacks.size())
				{
					makeStackDoNothing(next);
					continue;
				}

				if(!curOwner || curOwner->getSecSkillLevel(SecondarySkill::FIRST_AID) == 0) //no hero or hero has no first aid
				{
					range::random_shuffle(possibleStacks);
					const CStack * toBeHealed = possibleStacks.front();

					BattleAction heal;
					heal.actionType = Battle::STACK_HEAL;
					heal.additionalInfo = 0;
					heal.destinationTile = toBeHealed->position;
					heal.side = !next->attackerOwned;
					heal.stackNumber = next->ID;

					makeAutomaticAction(next, heal);
					continue;
				}
			}

			int numberOfAsks = 1;
			bool breakOuter = false;
			do
			{//ask interface and wait for answer
				if(!battleResult.get())
				{
					stackTurnTrigger(next); //various effects

					if (vstd::contains(next->state, EBattleStackState::FEAR))
					{
						makeStackDoNothing(next); //end immediately if stack was affected by fear
					}
					else
					{
						logGlobal->traceStream() << "Activating " << next->nodeName();
						auto nextId = next->ID;
						BattleSetActiveStack sas;
						sas.stack = nextId;
						sendAndApply(&sas);

						auto actionWasMade = [&]() -> bool
						{
							if(battleMadeAction.data)//active stack has made its action
								return true;
							if(battleResult.get())// battle is finished
								return true;
							if(next == nullptr)//active stack was been removed
								return true;
							return !next->alive();//active stack is dead
						};

						boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
						battleMadeAction.data = false;
						while(!actionWasMade())
						{
							battleMadeAction.cond.wait(lock);
							if(battleGetStackByID(nextId, false) != next)
								next = nullptr; //it may be removed, while we wait
						}
					}
				}

				if(battleResult.get()) //don't touch it, battle could be finished while waiting got action
				{
					breakOuter = true;
					break;
				}
				//we're after action, all results applied
				checkBattleStateChanges(); //check if this action ended the battle

				if(next != nullptr)
				{
					//check for good morale
					nextStackMorale = next->MoraleVal();
					if(!vstd::contains(next->state,EBattleStackState::HAD_MORALE)  //only one extra move per turn possible
						&& !vstd::contains(next->state,EBattleStackState::DEFENDING)
						&& !next->waited()
						&& !vstd::contains(next->state, EBattleStackState::FEAR)
						&&  next->alive()
						&&  nextStackMorale > 0
						&& !(NBonus::hasOfType(gs->curB->battleGetFightingHero(0), Bonus::BLOCK_MORALE)
							|| NBonus::hasOfType(gs->curB->battleGetFightingHero(1), Bonus::BLOCK_MORALE)) //checking if gs->curB->heroes have (or don't have) morale blocking bonuses
						)
					{
						if(gs->getRandomGenerator().nextInt(23) < nextStackMorale) //this stack hasn't got morale this turn

							{
								BattleTriggerEffect bte;
								bte.stackID = next->ID;
								bte.effect = Bonus::MORALE;
								bte.val = 1;
								bte.additionalInfo = 0;
								sendAndApply(&bte); //play animation

								++numberOfAsks; //move this stack once more
							}
					}
				}
				--numberOfAsks;
			} while (numberOfAsks > 0);

			if (breakOuter)
			{
				break;
			}

		}
	}

	endBattle(gs->curB->tile, gs->curB->battleGetFightingHero(0), gs->curB->battleGetFightingHero(1));
}

bool CGameHandler::makeAutomaticAction(const CStack *stack, BattleAction &ba)
{
	BattleSetActiveStack bsa;
	bsa.stack = stack->ID;
	bsa.askPlayerInterface = false;
	sendAndApply(&bsa);

	bool ret = makeBattleAction(ba);
	checkBattleStateChanges();
	return ret;
}

void CGameHandler::giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, ArtifactPosition pos)
{
	assert(a->artType);
	ArtifactLocation al;
	al.artHolder = const_cast<CGHeroInstance*>(h);

	ArtifactPosition slot = ArtifactPosition::PRE_FIRST;
	if(pos < 0)
	{
		if(pos == ArtifactPosition::FIRST_AVAILABLE)
			slot = a->firstAvailableSlot(h);
		else
			slot = a->firstBackpackSlot(h);
	}
	else
	{
		slot = pos;
	}

	al.slot = slot;

	if(slot < 0 || !a->canBePutAt(al))
	{
		complain("Cannot put artifact in that slot!");
		return;
	}

	putArtifact(al, a);
}
void CGameHandler::putArtifact(const ArtifactLocation &al, const CArtifactInstance *a)
{
	PutArtifact pa;
	pa.art = a;
	pa.al = al;
	sendAndApply(&pa);
}

void CGameHandler::giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, ArtifactPosition pos)
{
	CArtifactInstance *a = nullptr;
	if(!artType->constituents)
	{
		a = new CArtifactInstance();
	}
	else
	{
		a = new CCombinedArtifactInstance();
	}
	a->artType = artType; //*NOT* via settype -> all bonus-related stuff must be done by NewArtifact apply

	NewArtifact na;
	na.art = a;
	sendAndApply(&na); // -> updates a!!!, will create a on other machines

	giveHeroArtifact(h, a, pos);
}

void CGameHandler::setBattleResult(BattleResult::EResult resultType, int victoriusSide)
{
	boost::unique_lock<boost::mutex> guard(battleResult.mx);
	if(battleResult.data)
	{
		complain((boost::format("The battle result has been already set (to %d, asked to %d)")
		          % battleResult.data->result % resultType).str());
		return;
	}
	auto br = new BattleResult;
	br->result = resultType;
	br->winner = victoriusSide; //surrendering side loses
	gs->curB->calculateCasualties(br->casualties);
	battleResult.data = br;
}

void CGameHandler::commitPackage( CPackForClient *pack )
{
	sendAndApply(pack);
}

void CGameHandler::spawnWanderingMonsters(CreatureID creatureID)
{
	std::vector<int3>::iterator tile;
	std::vector<int3> tiles;
	getFreeTiles(tiles);
	ui32 amount = tiles.size() / 200; //Chance is 0.5% for each tile
	std::random_shuffle(tiles.begin(), tiles.end());
	logGlobal->traceStream() << "Spawning wandering monsters. Found " << tiles.size() << " free tiles. Creature type: " << creatureID;
	const CCreature *cre = VLC->creh->creatures.at(creatureID);
	for (int i = 0; i < amount; ++i)
	{
		tile = tiles.begin();
		logGlobal->traceStream() << "\tSpawning monster at " << *tile;
		putNewMonster(creatureID, cre->getRandomAmount(std::rand), *tile);
		tiles.erase(tile); //not use it again
	}
}

void CGameHandler::removeObstacle(const CObstacleInstance &obstacle)
{
	ObstaclesRemoved obsRem;
	obsRem.obstacles.insert(obstacle.uniqueID);
	sendAndApply(&obsRem);
}

void CGameHandler::synchronizeArtifactHandlerLists()
{
	UpdateArtHandlerLists uahl;
	uahl.treasures = VLC->arth->treasures;
	uahl.minors = VLC->arth->minors;
	uahl.majors = VLC->arth->majors;
	uahl.relics = VLC->arth->relics;
	sendAndApply(&uahl);
}

bool CGameHandler::isValidObject(const CGObjectInstance *obj) const
{
	return vstd::contains(gs->map->objects, obj);
}

bool CGameHandler::isBlockedByQueries(const CPack *pack, PlayerColor player)
{
	if(!strcmp(typeid(*pack).name(), typeid(PlayerMessage).name()))
		return false;

	auto query = queries.topQuery(player);
	if(query && query->blocksPack(pack))
	{
		complain(boost::str(boost::format("Player %s has to answer queries  before attempting any further actions. Top query is %s!") % player % query->toString()));
		return true;
	}

	return false;
}

void CGameHandler::removeAfterVisit(const CGObjectInstance *object)
{
	//If the object is being visited, there must be a matching query
	for(const auto &query : queries.allQueries())
	{
		if(auto someVistQuery = std::dynamic_pointer_cast<CObjectVisitQuery>(query))
		{
			if(someVistQuery->visitedObject == object)
			{
				someVistQuery->removeObjectAfterVisit = true;
				return;
			}
		}
	};

	//If we haven't returned so far, there is no query and no visit, call was wrong
	assert("This function needs to be called during the object visit!");
}

void CGameHandler::changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide)
{
	std::unordered_set<int3, ShashInt3> tiles;
	getTilesInRange(tiles, center, radius, player, hide? -1 : 1);
	if (hide)
	{
		std::unordered_set<int3, ShashInt3> observedTiles; //do not hide tiles observed by heroes. May lead to disastrous AI problems
		auto p = gs->getPlayer(player);
		for (auto h : p->heroes)
		{
			getTilesInRange(observedTiles, h->getSightCenter(), h->getSightRadius(), h->tempOwner, -1);
		}
		for (auto t : p->towns)
		{
			getTilesInRange(observedTiles, t->getSightCenter(), t->getSightRadius(), t->tempOwner, -1);
		}
		for (auto tile : observedTiles)
			vstd::erase_if_present (tiles, tile);
	}
	changeFogOfWar(tiles, player, hide);
}

void CGameHandler::changeFogOfWar(std::unordered_set<int3, ShashInt3> &tiles, PlayerColor player, bool hide)
{
	FoWChange fow;
	fow.tiles = tiles;
	fow.player = player;
	fow.mode = hide? 0 : 1;
	sendAndApply(&fow);
}

bool CGameHandler::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	if(auto topQuery = queries.topQuery(hero->getOwner()))
		if(auto visit = std::dynamic_pointer_cast<const CObjectVisitQuery>(topQuery))
			return !(visit->visitedObject == obj && visit->visitingHero == hero);

	return true;
}

void CGameHandler::duelFinished()
{
	auto si = getStartInfo();
	auto getName = [&](int i){ return si->getIthPlayersSettings(gs->curB->sides.at(i).color).name; };

	int casualtiesPoints = 0;
	logGlobal->debugStream() << boost::format("Winner side %d\nWinner casualties:")
		% (int)battleResult.data->winner;

	for(auto & elem : battleResult.data->casualties[battleResult.data->winner])
	{
		const CCreature *c = VLC->creh->creatures[elem.first];
		logGlobal->debugStream() << boost::format("\t* %d of %s") % elem.second % c->namePl;
		casualtiesPoints += c->AIValue * elem.second;
	}
	logGlobal->debugStream() << boost::format("Total casualties points: %d") % casualtiesPoints;


	time_t timeNow;
	time(&timeNow);

	std::ofstream out(cmdLineOptions["resultsFile"].as<std::string>(), std::ios::app);
	if(out)
	{
		out << boost::format("%s\t%s\t%s\t%d\t%d\t%d\t%s\n") % si->mapname % getName(0) % getName(1)
			% battleResult.data->winner % battleResult.data->result % casualtiesPoints
			% asctime(localtime(&timeNow));
	}
	else
	{
		logGlobal->errorStream() << "Cannot open to write " << cmdLineOptions["resultsFile"].as<std::string>();
	}

	CSaveFile resultFile("result.vdrst");
	resultFile << *battleResult.data;

	BattleResultsApplied resultsApplied;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	sendAndApply(&resultsApplied);
	return;
}

CasualtiesAfterBattle::CasualtiesAfterBattle(const CArmedInstance * _army, BattleInfo *bat):
	army(_army)
{
	heroWithDeadCommander = ObjectInstanceID();

	PlayerColor color = army->tempOwner;
	if(color == PlayerColor::UNFLAGGABLE)
		color = PlayerColor::NEUTRAL;

	for(CStack *st : bat->stacks)
	{
		if(vstd::contains(st->state, EBattleStackState::SUMMONED)) //don't take into account temporary summoned stacks
			continue;
		if (st->owner != color) //remove only our stacks
			continue;

		logGlobal->debugStream() << "Calculating casualties for " << st->nodeName();

		//FIXME: this info is also used in BattleInfo::calculateCasualties, refactor
		st->count = std::max (0, st->count - st->resurrected);

		if(st->slot == SlotID::ARROW_TOWERS_SLOT)
		{
			//do nothing
			logGlobal->debug("Ignored arrow towers stack.");
		}
		else if(st->slot == SlotID::WAR_MACHINES_SLOT)
		{
			auto warMachine = VLC->arth->creatureToMachineID(st->type->idNumber);

			if(warMachine == ArtifactID::NONE)
			{
				logGlobal->errorStream() << "Invalid creature in war machine virtual slot: " << st->nodeName();
			}
			//catapult artifact remain even if "creature" killed in siege
			else if(warMachine != ArtifactID::CATAPULT && !st->count)
			{
				logGlobal->debug("War machine has been destroyed");
				auto hero = dynamic_ptr_cast<CGHeroInstance> (army);
				if (hero)
					removedWarMachines.push_back (ArtifactLocation(hero, hero->getArtPos(warMachine, true)));
				else
					logGlobal->error("War machine in army without hero");
			}
		}
		else if(st->slot == SlotID::SUMMONED_SLOT_PLACEHOLDER)
		{
			if(st->alive() && st->count > 0)
			{
				logGlobal->debugStream() << "Permanently summoned " << st->count << " units.";
				const CreatureID summonedType = st->type->idNumber;
				summoned[summonedType] += st->count;
			}
		}
		else if(st->slot == SlotID::COMMANDER_SLOT_PLACEHOLDER)
		{
			if(nullptr == st->base)
			{
				logGlobal->error("Stack with no base in commander slot.");
			}
			else
			{
				auto c = dynamic_cast <const CCommanderInstance *>(st->base);
				if(c)
				{
					auto h = dynamic_cast <const CGHeroInstance *>(army);
					if (h && h->commander == c)
					{
						logGlobal->debug("Commander is dead.");
						heroWithDeadCommander = army->id; //TODO: unify commander handling
					}
				}
				else
					logGlobal->error("Stack with invalid instance in commander slot.");
			}
		}
		else if(st->base && !army->slotEmpty(st->slot))
		{
			if(st->count == 0 || !st->alive())
			{
				logGlobal->debug("Stack has been destroyed.");
				StackLocation sl(army, st->slot);
				newStackCounts.push_back(TStackAndItsNewCount(sl, 0));
			}
			else if(st->count < army->getStackCount(st->slot))
			{
				logGlobal->debugStream() << "Stack lost " << (army->getStackCount(st->slot) - st->count) << " units.";
				StackLocation sl(army, st->slot);
				newStackCounts.push_back(TStackAndItsNewCount(sl, st->count));
			}
		}
		else
		{
			logGlobal->warnStream() << "Unable to process stack: " << st->nodeName();
		}
	}
}

void CasualtiesAfterBattle::updateArmy(CGameHandler *gh)
{
	for(TStackAndItsNewCount &ncount : newStackCounts)
	{
		if(ncount.second > 0)
			gh->changeStackCount(ncount.first, ncount.second, true);
		else
			gh->eraseStack(ncount.first, true);
	}
	for(auto summoned_iter : summoned)
	{
		SlotID slot = army->getSlotFor(summoned_iter.first);
		if(slot.validSlot())
		{
			StackLocation location(army, slot);
			gh->addToSlot(location, summoned_iter.first.toCreature(), summoned_iter.second);
		}
		else
		{
			//even if it will be possible to summon anything permanently it should be checked for free slot
			//necromancy is handled separately
			gh->complain("No free slot to put summoned creature");
		}
	}
	for (auto al : removedWarMachines)
	{
		gh->removeArtifact(al);
	}
	if (heroWithDeadCommander != ObjectInstanceID())
	{
		SetCommanderProperty scp;
		scp.heroid = heroWithDeadCommander;
		scp.which = SetCommanderProperty::ALIVE;
		scp.amount = 0;
		gh->sendAndApply (&scp);
	}
}

CGameHandler::FinishingBattleHelper::FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, bool Duel, int RemainingBattleQueriesCount)
{
	assert(Query->result);
	assert(Query->bi);
	auto &result = *Query->result;
	auto &info = *Query->bi;

	winnerHero = result.winner != 0 ? info.sides[1].hero : info.sides[0].hero;
	loserHero = result.winner != 0 ? info.sides[0].hero : info.sides[1].hero;
	victor = info.sides[result.winner].color;
	loser = info.sides[!result.winner].color;
	duel = Duel;
	remainingBattleQueriesCount = RemainingBattleQueriesCount;
}

CGameHandler::FinishingBattleHelper::FinishingBattleHelper()
{
	winnerHero = loserHero = nullptr;
}

///ServerSpellCastEnvironment
ServerSpellCastEnvironment::ServerSpellCastEnvironment(CGameHandler * gh): gh(gh)
{

}

void ServerSpellCastEnvironment::sendAndApply(CPackForClient * info) const
{
	gh->sendAndApply(info);
}

CRandomGenerator & ServerSpellCastEnvironment::getRandomGenerator() const
{
	return gh->gameState()->getRandomGenerator();
}

void ServerSpellCastEnvironment::complain(const std::string& problem) const
{
	gh->complain(problem);
}

const CGameInfoCallback * ServerSpellCastEnvironment::getCb() const
{
	return gh;
}


const CMap * ServerSpellCastEnvironment::getMap() const
{
	return gh->gameState()->map;
}

bool ServerSpellCastEnvironment::moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, PlayerColor asker) const
{
	return gh->moveHero(hid, dst, teleporting, false, asker);
}
