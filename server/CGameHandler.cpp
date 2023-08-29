/*
 * CGameHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameHandler.h"

#include "CVCMIServer.h"
#include "ServerNetPackVisitors.h"
#include "ServerSpellCastEnvironment.h"
#include "battles/BattleProcessor.h"
#include "processors/HeroPoolProcessor.h"
#include "processors/PlayerMessageProcessor.h"
#include "processors/TurnOrderProcessor.h"
#include "queries/QueriesProcessor.h"
#include "queries/MapQueries.h"

#include "../lib/ArtifactUtils.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CCreatureSet.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CSoundBase.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CTownHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/UnlockGuard.h"
#include "../lib/GameSettings.h"
#include "../lib/ScriptHandler.h"
#include "../lib/StartInfo.h"
#include "../lib/TerrainHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/int3.h"

#include "../lib/filesystem/FileInfo.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/gameState/CGameState.h"

#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/pathfinder/CPathfinder.h"
#include "../lib/pathfinder/PathfinderOptions.h"
#include "../lib/pathfinder/TurnInfo.h"

#include "../lib/registerTypes/RegisterTypes.h"

#include "../lib/rmg/CMapGenOptions.h"

#include "../lib/serializer/CTypeList.h"
#include "../lib/serializer/Cast.h"
#include "../lib/serializer/Connection.h"
#include "../lib/serializer/JsonSerializer.h"

#include "../lib/spells/CSpellHandler.h"

#include "vstd/CLoggerBase.h"
#include <vcmi/events/EventBus.h>
#include <vcmi/events/GenericEvents.h>
#include <vcmi/events/AdventureEvents.h>

#define COMPLAIN_RET_IF(cond, txt) do {if (cond){complain(txt); return;}} while(0)
#define COMPLAIN_RET_FALSE_IF(cond, txt) do {if (cond){complain(txt); return false;}} while(0)
#define COMPLAIN_RET(txt) {complain(txt); return false;}
#define COMPLAIN_RETF(txt, FORMAT) {complain(boost::str(boost::format(txt) % FORMAT)); return false;}

template <typename T> class CApplyOnGH;

class CBaseForGHApply
{
public:
	virtual bool applyOnGH(CGameHandler * gh, CGameState * gs, void * pack) const =0;
	virtual ~CBaseForGHApply(){}
	template<typename U> static CBaseForGHApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnGH<U>();
	}
};

template <typename T> class CApplyOnGH : public CBaseForGHApply
{
public:
	bool applyOnGH(CGameHandler * gh, CGameState * gs, void * pack) const override
	{
		T *ptr = static_cast<T*>(pack);
		try
		{
			ApplyGhNetPackVisitor applier(*gh);

			ptr->visit(applier);

			return applier.getResult();
		}
		catch(ExceptionNotAllowedAction & e)
		{
            (void)e;
			return false;
		}
	}
};

template <>
class CApplyOnGH<CPack> : public CBaseForGHApply
{
public:
	bool applyOnGH(CGameHandler * gh, CGameState * gs, void * pack) const override
	{
		logGlobal->error("Cannot apply on GH plain CPack!");
		assert(0);
		return false;
	}
};

static inline double distance(int3 a, int3 b)
{
	return std::sqrt((double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

template <typename T>
void callWith(std::vector<T> args, std::function<void(T)> fun, ui32 which)
{
	fun(args[which]);
}

const Services * CGameHandler::services() const
{
	return VLC;
}

const CGameHandler::BattleCb * CGameHandler::battle() const
{
	return this;
}

const CGameHandler::GameCb * CGameHandler::game() const
{
	return this;
}

vstd::CLoggerBase * CGameHandler::logger() const
{
	return logGlobal;
}

events::EventBus * CGameHandler::eventBus() const
{
	return serverEventBus.get();
}

CVCMIServer * CGameHandler::gameLobby() const
{
	return lobby;
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero, SecondarySkill skill)
{
	changeSecSkill(hero, skill, 1, 0);
	expGiven(hero);
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero)
{
	// required exp for at least 1 lvl-up hasn't been reached
	if (!hero->gainsLevel())
	{
		return;
	}

	// give primary skill
	logGlobal->trace("%s got level %d", hero->getNameTranslated(), hero->level);
	auto primarySkill = hero->nextPrimarySkill(getRandomGenerator());

	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = primarySkill;
	sps.abs = false;
	sps.val = 1;
	sendAndApply(&sps);

	PrepareHeroLevelUp pre;
	pre.heroId = hero->id;
	sendAndApply(&pre);

	HeroLevelUp hlu;
	hlu.player = hero->tempOwner;
	hlu.heroId = hero->id;
	hlu.primskill = primarySkill;
	hlu.skills = pre.skills;

	if (hlu.skills.size() == 0)
	{
		sendAndApply(&hlu);
		levelUpHero(hero);
	}
	else if (hlu.skills.size() == 1)
	{
		sendAndApply(&hlu);
		levelUpHero(hero, pre.skills.front());
	}
	else if (hlu.skills.size() > 1)
	{
		auto levelUpQuery = std::make_shared<CHeroLevelUpDialogQuery>(this, hlu, hero);
		hlu.queryID = levelUpQuery->queryID;
		queries->addQuery(levelUpQuery);
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
	scp.accumulatedBonus.duration = BonusDuration::PERMANENT;
	scp.accumulatedBonus.turnsRemain = 0;
	scp.accumulatedBonus.source = BonusSource::COMMANDER;
	scp.accumulatedBonus.valType = BonusValueType::BASE_NUMBER;
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
				scp.accumulatedBonus.type = BonusType::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = static_cast<int>(PrimarySkill::ATTACK);
				break;
			case ECommander::DEFENSE:
				scp.accumulatedBonus.type = BonusType::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = static_cast<int>(PrimarySkill::DEFENSE);
				break;
			case ECommander::HEALTH:
				scp.accumulatedBonus.type = BonusType::STACK_HEALTH;
				scp.accumulatedBonus.valType = BonusValueType::PERCENT_TO_BASE;
				break;
			case ECommander::DAMAGE:
				scp.accumulatedBonus.type = BonusType::CREATURE_DAMAGE;
				scp.accumulatedBonus.subtype = 0;
				scp.accumulatedBonus.valType = BonusValueType::PERCENT_TO_BASE;
				break;
			case ECommander::SPEED:
				scp.accumulatedBonus.type = BonusType::STACKS_SPEED;
				break;
			case ECommander::SPELL_POWER:
				scp.accumulatedBonus.type = BonusType::MAGIC_RESISTANCE;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::RESISTANCE);
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CREATURE_SPELL_POWER;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::SPELL_POWER) * 100; //like hero with spellpower = ability level
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CASTS;
				scp.accumulatedBonus.val = difference (VLC->creh->skillLevels, c->secondarySkills, ECommander::CASTS);
				sendAndApply (&scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CREATURE_ENCHANT_POWER; //send normally
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
	if(hero)
	{
		clu.heroId = hero->id;
		clu.player = hero->tempOwner;
	}
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
	int skillAmount = static_cast<int>(clu.skills.size());

	if (!skillAmount)
	{
		sendAndApply(&clu);
		levelUpCommander(c);
	}
	else if (skillAmount == 1  ||  hero->tempOwner == PlayerColor::NEUTRAL) //choose skill automatically
	{
		sendAndApply(&clu);
		levelUpCommander(c, *RandomGeneratorUtil::nextItem(clu.skills, getRandomGenerator()));
	}
	else if (skillAmount > 1) //apply and ask for secondary skill
	{
		auto commanderLevelUp = std::make_shared<CCommanderLevelUpDialogQuery>(this, clu, hero);
		clu.queryID = commanderLevelUp->queryID;
		queries->addQuery(commanderLevelUp);
		sendAndApply(&clu);
	}
}

void CGameHandler::expGiven(const CGHeroInstance *hero)
{
	if (hero->gainsLevel())
		levelUpHero(hero);
	else if (hero->commander && hero->commander->gainsLevel())
		levelUpCommander(hero->commander);

	//if (hero->commander && hero->level > hero->commander->level && hero->commander->gainsLevel())
// 		levelUpCommander(hero->commander);
// 	else
// 		levelUpHero(hero);
}

void CGameHandler::changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, bool abs)
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
				iw.text.appendLocalString(EMetaText::GENERAL_TXT, 1); //can gain no more XP
				iw.text.replaceRawString(hero->getNameTranslated());
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
		if (hero->commander && hero->commander->alive)
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

void CGameHandler::changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs)
{
	if(!hero)
	{
		logGlobal->error("changeSecSkill provided no hero");
		return;
	}
	SetSecSkill sss;
	sss.id = hero->id;
	sss.which = which;
	sss.val = val;
	sss.abs = abs;
	sendAndApply(&sss);

	if (hero->visitedTown)
		giveSpells(hero->visitedTown, hero);
}

void CGameHandler::handleClientDisconnection(std::shared_ptr<CConnection> c)
{
	if(lobby->getState() == EServerState::SHUTDOWN || !gs || !gs->scenarioOps)
		return;
	
	for(auto & playerConnections : connections)
	{
		PlayerColor playerId = playerConnections.first;
		auto * playerSettings = gs->scenarioOps->getPlayersSettings(playerId.getNum());
		if(!playerSettings)
			continue;
		
		auto playerConnection = vstd::find(playerConnections.second, c);
		if(playerConnection != playerConnections.second.end())
		{
			std::string messageText = boost::str(boost::format("%s (cid %d) was disconnected") % playerSettings->name % c->connectionID);
			playerMessages->broadcastMessage(playerId, messageText);
		}
	}
}

void CGameHandler::handleReceivedPack(CPackForServer * pack)
{
	//prepare struct informing that action was applied
	auto sendPackageResponse = [&](bool succesfullyApplied)
	{
		PackageApplied applied;
		applied.player = pack->player;
		applied.result = succesfullyApplied;
		applied.packType = typeList.getTypeID(pack);
		applied.requestID = pack->requestID;
		pack->c->sendPack(&applied);
	};

	CBaseForGHApply * apply = applier->getApplier(typeList.getTypeID(pack)); //and appropriate applier object
	if(isBlockedByQueries(pack, pack->player))
	{
		sendPackageResponse(false);
	}
	else if(apply)
	{
		const bool result = apply->applyOnGH(this, this->gs, pack);
		if(result)
			logGlobal->trace("Message %s successfully applied!", typeid(*pack).name());
		else
			complain((boost::format("Got false in applying %s... that request must have been fishy!")
				% typeid(*pack).name()).str());

		sendPackageResponse(true);
	}
	else
	{
		logGlobal->error("Message cannot be applied, cannot find applier (unregistered type)!");
		sendPackageResponse(false);
	}

	vstd::clear_pointer(pack);
}


CGameHandler::CGameHandler()
	: turnTimerHandler(*this)
{}

CGameHandler::CGameHandler(CVCMIServer * lobby)
	: lobby(lobby)
	, heroPool(std::make_unique<HeroPoolProcessor>(this))
	, battles(std::make_unique<BattleProcessor>(this))
	, turnOrder(std::make_unique<TurnOrderProcessor>(this))
	, queries(std::make_unique<QueriesProcessor>())
	, playerMessages(std::make_unique<PlayerMessageProcessor>(this))
	, complainNoCreatures("No creatures to split")
	, complainNotEnoughCreatures("Cannot split that stack, not enough creatures!")
	, complainInvalidSlot("Invalid slot accessed!")
	, turnTimerHandler(*this)
{
	QID = 1;
	IObjectInterface::cb = this;
	applier = std::make_shared<CApplier<CBaseForGHApply>>();
	registerTypesServerPacks(*applier);

	spellEnv = new ServerSpellCastEnvironment(this);
}

CGameHandler::~CGameHandler()
{
	delete spellEnv;
	delete gs;
}

void CGameHandler::reinitScripting()
{
	serverEventBus = std::make_unique<events::EventBus>();
#if SCRIPTING_ENABLED
	serverScripts.reset(new scripting::PoolImpl(this, spellEnv));
#endif
}

void CGameHandler::init(StartInfo *si, Load::ProgressAccumulator & progressTracking)
{
	if (si->seedToBeUsed == 0)
	{
		si->seedToBeUsed = static_cast<ui32>(std::time(nullptr));
	}
	CMapService mapService;
	gs = new CGameState();
	gs->preInit(VLC);
	logGlobal->info("Gamestate created!");
	gs->init(&mapService, si, progressTracking);
	logGlobal->info("Gamestate initialized!");

	// reset seed, so that clients can't predict any following random values
	getRandomGenerator().resetSeed();

	for (auto & elem : gs->players)
		turnOrder->addPlayer(elem.first);

	reinitScripting();
}

static bool evntCmp(const CMapEvent &a, const CMapEvent &b)
{
	return a.earlierThan(b);
}

void CGameHandler::setPortalDwelling(const CGTownInstance * town, bool forced=false, bool clear = false)
{// bool forced = true - if creature should be replaced, if false - only if no creature was set

	const PlayerState * p = getPlayerState(town->tempOwner);
	if (!p)
	{
		assert(town->tempOwner == PlayerColor::NEUTRAL);
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

			auto dwelling = *RandomGeneratorUtil::nextItem(dwellings, getRandomGenerator());

			// for multi-creature dwellings like Golem Factory
			auto creatureId = RandomGeneratorUtil::nextItem(dwelling->creatures, getRandomGenerator())->second[0];

			if (clear)
			{
				ssi.creatures[GameConstants::CREATURES_PER_TOWN].first = std::max(1, (VLC->creh->objects.at(creatureId)->getGrowth())/2);
			}
			else
			{
				ssi.creatures[GameConstants::CREATURES_PER_TOWN].first = VLC->creh->objects.at(creatureId)->getGrowth();
			}
			ssi.creatures[GameConstants::CREATURES_PER_TOWN].second.push_back(creatureId);
			sendAndApply(&ssi);
		}
}

void CGameHandler::onPlayerTurnStarted(PlayerColor which)
{
	events::PlayerGotTurn::defaultExecute(serverEventBus.get(), which);
	turnTimerHandler.onPlayerGetTurn(which);
}

void CGameHandler::onPlayerTurnEnded(PlayerColor which)
{
	const auto * playerState = gs->getPlayerState(which);
	assert(playerState->status == EPlayerStatus::INGAME);

	if (playerState->towns.empty())
	{
		DaysWithoutTown pack;
		pack.player = which;
		pack.daysWithoutCastle = playerState->daysWithoutCastle.value_or(0) + 1;
		sendAndApply(&pack);
	}
	else
	{
		if (playerState->daysWithoutCastle.has_value())
		{
			DaysWithoutTown pack;
			pack.player = which;
			pack.daysWithoutCastle = std::nullopt;
			sendAndApply(&pack);
		}
	}

	// check for 7 days without castle
	checkVictoryLossConditionsForPlayer(which);

	bool newWeek = getDate(Date::DAY_OF_WEEK) == 7; // end of 7th day

	if (newWeek) //new heroes in tavern
		heroPool->onNewWeek(which);
}

void CGameHandler::onNewTurn()
{
	logGlobal->trace("Turn %d", gs->day+1);
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
		for (const CGTownInstance *t : gs->map->towns)
		{
			if (t->hasBuilt(BuildingID::GRAIL, ETownType::INFERNO))
			{
				deityOfFireBuilt = true;
				break;
			}
		}

		if (deityOfFireBuilt)
		{
			n.specialWeek = NewTurn::DEITYOFFIRE;
			n.creatureid = CreatureID::IMP;
		}
		else if(VLC->settings()->getBoolean(EGameSettings::CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS))
		{
			int monthType = getRandomGenerator().nextInt(99);
			if (newMonth) //new month
			{
				if (monthType < 40) //double growth
				{
					n.specialWeek = NewTurn::DOUBLE_GROWTH;
					if (VLC->settings()->getBoolean(EGameSettings::CREATURES_ALLOW_ALL_FOR_DOUBLE_MONTH))
					{
						n.creatureid = VLC->creh->pickRandomMonster(getRandomGenerator());
					}
					else if (VLC->creh->doubledCreatures.size())
					{
						n.creatureid = *RandomGeneratorUtil::nextItem(VLC->creh->doubledCreatures, getRandomGenerator());
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
					std::pair<int, CreatureID> newMonster(54, CreatureID());
					do
					{
						newMonster.second = VLC->creh->pickRandomMonster(getRandomGenerator());
					} while (VLC->creh->objects[newMonster.second] &&
						(*VLC->townh)[VLC->creatures()->getById(newMonster.second)->getFaction()]->town == nullptr); // find first non neutral creature
					n.creatureid = newMonster.second;
				}
			}
		}
	}

	for (auto & elem : gs->players)
	{
		if (elem.first == PlayerColor::NEUTRAL)
			continue;

		assert(elem.first.isValidPlayer());//illegal player number!

		std::pair<PlayerColor, si32> playerGold(elem.first, elem.second.resources[EGameResID::GOLD]);
		hadGold.insert(playerGold);

		if (firstTurn)
			heroPool->onNewWeek(elem.first);

		n.res[elem.first] = elem.second.resources;

		if(!firstTurn && newWeek) //weekly crystal generation if 1 or more crystal dragons in any hero army or town garrison
		{
			bool hasCrystalGenCreature = false;
			for(CGHeroInstance * hero : elem.second.heroes)
			{
				for(auto stack : hero->stacks)
				{
					if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
					{
						hasCrystalGenCreature = true;
						break;
					}
				}
			}
			if(!hasCrystalGenCreature) //not found in armies, check towns
			{
				for(CGTownInstance * town : elem.second.towns)
				{
					for(auto stack : town->stacks)
					{
						if(stack.second->hasBonusOfType(BonusType::SPECIAL_CRYSTAL_GENERATION))
						{
							hasCrystalGenCreature = true;
							break;
						}
					}
				}
			}
			if(hasCrystalGenCreature)
				n.res[elem.first][EGameResID::CRYSTAL] += 3;
		}

		for (CGHeroInstance *h : (elem).second.heroes)
		{
			if (h->visitedTown)
				giveSpells(h->visitedTown, h);

			NewTurn::Hero hth;
			hth.id = h->id;
			auto ti = std::make_unique<TurnInfo>(h, 1);
			// TODO: this code executed when bonuses of previous day not yet updated (this happen in NewTurn::applyGs). See issue 2356
			hth.move = h->movementPointsLimitCached(gs->map->getTile(h->visitablePos()).terType->isLand(), ti.get());
			hth.mana = h->getManaNewTurn();

			n.heroes.insert(hth);

			if (!firstTurn) //not first day
			{
				for (int k = 0; k < GameConstants::RESOURCE_QUANTITY; k++)
				{
					n.res[elem.first][k] += h->valOfBonuses(BonusType::GENERATE_RESOURCE, k);
				}
			}
		}
	}
	for (CGTownInstance *t : gs->map->towns)
	{
		PlayerColor player = t->tempOwner;
		handleTownEvents(t, n);
		if (newWeek) //first day of week
		{
			if (t->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
				setPortalDwelling(t, true, (n.specialWeek == NewTurn::PLAGUE ? true : false)); //set creatures for Portal of Summoning

			if (!firstTurn)
				if (t->hasBuilt(BuildingSubID::TREASURY) && player.isValidPlayer())
						n.res[player][EGameResID::GOLD] += hadGold.at(player)/10; //give 10% of starting gold

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
					const CCreature *cre = VLC->creh->objects.at(t->creatures.at(k).second.back());

					if (n.specialWeek == NewTurn::PLAGUE)
						availableCount = t->creatures.at(k).first / 2; //halve their number, no growth
					else
					{
						if (firstTurn) //first day of game: use only basic growths
							availableCount = cre->getGrowth();
						else
							availableCount += t->creatureGrowth(k);

						//Deity of fire week - upgrade both imps and upgrades
						if (n.specialWeek == NewTurn::DEITYOFFIRE && vstd::contains(t->creatures.at(k).second, n.creatureid))
							availableCount += 15;

						if (cre->getId() == n.creatureid) //bonus week, effect applies only to identical creatures
						{
							if (n.specialWeek == NewTurn::DOUBLE_GROWTH)
								availableCount *= 2;
							else if (n.specialWeek == NewTurn::BONUS_GROWTH)
								availableCount += 5;
						}
					}
				}
			}
		}
		if (!firstTurn  &&  player.isValidPlayer())//not the first day and town not neutral
		{
			n.res[player] = n.res[player] + t->dailyIncome();
		}
		if(t->hasBuilt(BuildingID::GRAIL)
			&& t->town->buildings.at(BuildingID::GRAIL)->height == CBuilding::HEIGHT_SKYSHIP)
		{
			// Skyship, probably easier to handle same as Veil of darkness
			//do it every new day after veils apply
			if (player != PlayerColor::NEUTRAL) //do not reveal fow for neutral player
			{
				FoWChange fw;
				fw.mode = 1;
				fw.player = player;
				// find all hidden tiles
				const auto fow = getPlayerTeam(player)->fogOfWarMap;

				auto shape = fow->shape();
				for(size_t z = 0; z < shape[0]; z++)
					for(size_t x = 0; x < shape[1]; x++)
						for(size_t y = 0; y < shape[2]; y++)
							if (!(*fow)[z][x][y])
								fw.tiles.insert(int3(x, y, z));

				sendAndApply (&fw);
			}
		}
		if (t->hasBonusOfType (BonusType::DARKNESS))
		{
			for (auto & player : gs->players)
			{
				if (getPlayerStatus(player.first) == EPlayerStatus::INGAME &&
					getPlayerRelations(player.first, t->tempOwner) == PlayerRelations::ENEMIES)
					changeFogOfWar(t->visitablePos(), t->getBonusLocalFirst(Selector::type()(BonusType::DARKNESS))->val, player.first, true);
			}
		}
	}

	if (newMonth)
	{
		SetAvailableArtifacts saa;
		saa.id = -1;
		pickAllowedArtsSet(saa.arts, getRandomGenerator());
		sendAndApply(&saa);
	}
	sendAndApply(&n);

	if (newWeek)
	{
		//spawn wandering monsters
		if (newMonth && (n.specialWeek == NewTurn::DOUBLE_GROWTH || n.specialWeek == NewTurn::DEITYOFFIRE))
		{
			spawnWanderingMonsters(n.creatureid);
		}

		//new week info popup
		if (!firstTurn)
		{
			InfoWindow iw;
			switch (n.specialWeek)
			{
				case NewTurn::DOUBLE_GROWTH:
					iw.text.appendLocalString(EMetaText::ARRAY_TXT, 131);
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, n.creatureid);
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, n.creatureid);
					break;
				case NewTurn::PLAGUE:
					iw.text.appendLocalString(EMetaText::ARRAY_TXT, 132);
					break;
				case NewTurn::BONUS_GROWTH:
					iw.text.appendLocalString(EMetaText::ARRAY_TXT, 134);
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, n.creatureid);
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, n.creatureid);
					break;
				case NewTurn::DEITYOFFIRE:
					iw.text.appendLocalString(EMetaText::ARRAY_TXT, 135);
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, 42); //%s imp
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, 42); //%s imp
					iw.text.replacePositiveNumber(15);							//%+d 15
					iw.text.replaceLocalString(EMetaText::CRE_SING_NAMES, 43); //%s familiar
					iw.text.replacePositiveNumber(15);							//%+d 15
					break;
				default:
					if (newMonth)
					{
						iw.text.appendLocalString(EMetaText::ARRAY_TXT, (130));
						iw.text.replaceLocalString(EMetaText::ARRAY_TXT, getRandomGenerator().nextInt(32, 41));
					}
					else
					{
						iw.text.appendLocalString(EMetaText::ARRAY_TXT, (133));
						iw.text.replaceLocalString(EMetaText::ARRAY_TXT, getRandomGenerator().nextInt(43, 57));
					}
			}
			for (auto & elem : gs->players)
			{
				iw.player = elem.first;
				sendAndApply(&iw);
			}
		}
	}

	if (!firstTurn)
		checkVictoryLossConditionsForAll(); // check for map turn limit

	logGlobal->trace("Info about turn %d has been sent!", n.day);
	handleTimeEvents();
	//call objects
	for (auto & elem : gs->map->objects)
	{
		if (elem)
			elem->newTurn(getRandomGenerator());
	}

	synchronizeArtifactHandlerLists(); //new day events may have changed them. TODO better of managing that
}

void CGameHandler::run(bool resume)
{
	LOG_TRACE_PARAMS(logGlobal, "resume=%d", resume);

	for (auto cc : lobby->connections)
	{
		auto players = lobby->getAllClientPlayers(cc->connectionID);
		std::stringstream sbuffer;
		sbuffer << "Connection " << cc->connectionID << " will handle " << players.size() << " player: ";
		for (PlayerColor color : players)
		{
			sbuffer << color << " ";
			{
				boost::unique_lock<boost::recursive_mutex> lock(gsm);
				connections[color].insert(cc);
			}
		}
		logGlobal->info(sbuffer.str());
	}

#if SCRIPTING_ENABLED
	services()->scripts()->run(serverScripts);
#endif

	if (!resume)
	{
		onNewTurn();
		events::TurnStarted::defaultExecute(serverEventBus.get());
		for(auto & player : gs->players)
			turnTimerHandler.onGameplayStart(player.first);
	}
	else
		events::GameResumed::defaultExecute(serverEventBus.get());

	turnOrder->onGameStarted();

	//wait till game is done
	auto clockLast = std::chrono::steady_clock::now();
	while(lobby->getState() == EServerState::GAMEPLAY)
	{
		const auto clockDuration = std::chrono::steady_clock::now() - clockLast;
		const int timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(clockDuration).count();
		clockLast += clockDuration;
		turnTimerHandler.update(timePassed);
		boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
	}
}

void CGameHandler::giveSpells(const CGTownInstance *t, const CGHeroInstance *h)
{
	if (!h->hasSpellbook())
		return; //hero hasn't spellbook
	ChangeSpells cs;
	cs.hid = h->id;
	cs.learn = true;
	if (t->hasBuilt(BuildingID::GRAIL, ETownType::CONFLUX) && t->hasBuilt(BuildingID::MAGES_GUILD_1))
	{
		// Aurora Borealis give spells of all levels even if only level 1 mages guild built
		for (int i = 0; i < h->maxSpellLevel(); i++)
		{
			std::vector<SpellID> spells;
			getAllowedSpells(spells, i+1);
			for (auto & spell : spells)
				cs.spells.insert(spell);
		}
	}
	else
	{
		for (int i = 0; i < std::min(t->mageGuildLevel(), h->maxSpellLevel()); i++)
		{
			for (int j = 0; j < t->spellsAtLevel(i+1, true) && j < t->spells.at(i).size(); j++)
			{
				if (!h->spellbookContainsSpell(t->spells.at(i).at(j)))
					cs.spells.insert(t->spells.at(i).at(j));
			}
		}
	}
	if (!cs.spells.empty())
		sendAndApply(&cs);
}

bool CGameHandler::removeObject(const CGObjectInstance * obj)
{
	if (!obj || !getObj(obj->id))
	{
		logGlobal->error("Something wrong, that object already has been removed or hasn't existed!");
		return false;
	}

	RemoveObject ro;
	ro.id = obj->id;
	sendAndApply(&ro);

	checkVictoryLossConditionsForAll(); //eg if monster escaped (removing objs after battle is done dircetly by endBattle, not this function)
	return true;
}

bool CGameHandler::moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit, PlayerColor asker)
{
	const CGHeroInstance *h = getHero(hid);
	// not turn of that hero or player can't simply teleport hero (at least not with this function)
	if(!h || (asker != PlayerColor::NEUTRAL && teleporting))
	{
		if(h && getStartInfo()->turnTimerInfo.isEnabled() && gs->players[h->getOwner()].turnTimer.turnTimer == 0)
			return true; //timer expired, no error
		
		logGlobal->error("Illegal call to move hero!");
		return false;
	}

	logGlobal->trace("Player %d (%s) wants to move hero %d from %s to %s", asker, asker.getStr(), hid.getNum(), h->pos.toString(), dst.toString());
	const int3 hmpos = h->convertToVisitablePos(dst);

	if (!gs->map->isInTheMap(hmpos))
	{
		logGlobal->error("Destination tile is outside the map!");
		return false;
	}

	const TerrainTile t = *getTile(hmpos);
	const int3 guardPos = gs->guardingCreaturePosition(hmpos);

	const bool embarking = !h->boat && !t.visitableObjects.empty() && t.visitableObjects.back()->ID == Obj::BOAT;
	const bool disembarking = h->boat
		&& t.terType->isLand()
		&& (dst == h->pos
			|| (h->boat->layer == EPathfindingLayer::SAIL && !t.blocked));

	//result structure for start - movement failed, no move points used
	TryMoveHero tmh;
	tmh.id = hid;
	tmh.start = h->pos;
	tmh.end = dst;
	tmh.result = TryMoveHero::FAILED;
	tmh.movePoints = h->movementPointsRemaining();

	//check if destination tile is available
	auto pathfinderHelper = std::make_unique<CPathfinderHelper>(gs, h, PathfinderOptions());
	auto ti = pathfinderHelper->getTurnInfo();

	const bool canFly = pathfinderHelper->hasBonusOfType(BonusType::FLYING_MOVEMENT) || (h->boat && h->boat->layer == EPathfindingLayer::AIR);
	const bool canWalkOnSea = pathfinderHelper->hasBonusOfType(BonusType::WATER_WALKING) || (h->boat && h->boat->layer == EPathfindingLayer::WATER);
	const int cost = pathfinderHelper->getMovementCost(h->visitablePos(), hmpos, nullptr, nullptr, h->movementPointsRemaining());

	//it's a rock or blocked and not visitable tile
	//OR hero is on land and dest is water and (there is not present only one object - boat)
	if (((!t.terType->isPassable()  ||  (t.blocked && !t.visitable && !canFly))
			&& complain("Cannot move hero, destination tile is blocked!"))
		|| ((!h->boat && !canWalkOnSea && !canFly && t.terType->isWater() && (t.visitableObjects.size() < 1 || !t.visitableObjects.back()->isCoastVisitable()))  //hero is not on boat/water walking and dst water tile doesn't contain boat/hero (objs visitable from land) -> we test back cause boat may be on top of another object (#276)
			&& complain("Cannot move hero, destination tile is on water!"))
		|| ((h->boat && h->boat->layer == EPathfindingLayer::SAIL && t.terType->isLand() && t.blocked)
			&& complain("Cannot disembark hero, tile is blocked!"))
		|| ((distance(h->pos, dst) >= 1.5 && !teleporting)
			&& complain("Tiles are not neighboring!"))
		|| ((h->inTownGarrison)
			&& complain("Can not move garrisoned hero!"))
		|| (((int)h->movementPointsRemaining() < cost  &&  dst != h->pos  &&  !teleporting)
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
		for (CGObjectInstance *obj : gs->map->getTile(int3(h->pos.x-1, h->pos.y, h->pos.z)).visitableObjects)
		{
			obj->onHeroLeave(h);
		}
		this->getTilesInRange(tmh.fowRevealed, h->getSightCenter()+(tmh.end-tmh.start), h->getSightRadius(), h->tempOwner, 1);
	};

	auto doMove = [&](TryMoveHero::EResult result, EGuardLook lookForGuards,
								EVisitDest visitDest, ELEaveTile leavingTile) -> bool
	{
		LOG_TRACE_PARAMS(logGlobal, "Hero %s starts movement from %s to %s", h->getNameTranslated() % tmh.start.toString() % tmh.end.toString());

		auto moveQuery = std::make_shared<CHeroMovementQuery>(this, tmh, h);
		queries->addQuery(moveQuery);

		if (leavingTile == LEAVING_TILE)
			leaveTile();

		if (isInTheMap(guardPos))
			tmh.attackedFrom = std::make_optional(guardPos);

		tmh.result = result;
		sendAndApply(&tmh);

		if (visitDest == VISIT_DEST && t.topVisitableObj() && t.topVisitableObj()->id == h->id)
		{ // Hero should be always able to visit any object he staying on even if there guards around
			visitObjectOnTile(t, h);
		}
		else if (lookForGuards == CHECK_FOR_GUARDS && isInTheMap(guardPos))
		{
			const TerrainTile &guardTile = *gs->getTile(guardPos);
			objectVisited(guardTile.visitableObjects.back(), h);

			moveQuery->visitDestAfterVictory = visitDest==VISIT_DEST;
		}
		else if (visitDest == VISIT_DEST)
		{
			visitObjectOnTile(t, h);
		}

		for(auto topQuery = queries->topQuery(h->tempOwner); true; topQuery = queries->topQuery(h->tempOwner))
		{
			moveQuery = std::dynamic_pointer_cast<CHeroMovementQuery>(topQuery);
			if(!moveQuery || (transit && result == TryMoveHero::SUCCESS))
				break;
			
			queries->popIfTop(moveQuery);
		}
		logGlobal->trace("Hero %s ends movement", h->getNameTranslated());
		return result != TryMoveHero::FAILED;
	};

	//interaction with blocking object (like resources)
	auto blockingVisit = [&]() -> bool
	{
		for (CGObjectInstance *obj : t.visitableObjects)
		{
			if(h->boat && !obj->isBlockedVisitable() && !h->boat->onboardVisitAllowed)
				return doMove(TryMoveHero::SUCCESS, this->IGNORE_GUARDS, DONT_VISIT_DEST, REMAINING_ON_TILE);
			
			if (obj != h && obj->isBlockedVisitable() && !obj->passableFor(h->tempOwner))
			{
				EVisitDest visitDest = VISIT_DEST;
				if(h->boat && !h->boat->onboardVisitAllowed)
					visitDest = DONT_VISIT_DEST;
				
				return doMove(TryMoveHero::BLOCKING_VISIT, this->IGNORE_GUARDS, visitDest, REMAINING_ON_TILE);
			}
		}
		return false;
	};


	if (!transit && embarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movementPointsRemaining(), cost, false, ti);
		return doMove(TryMoveHero::EMBARK, IGNORE_GUARDS, DONT_VISIT_DEST, LEAVING_TILE);
		// In H3 embark ignore guards
	}

	if (disembarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movementPointsRemaining(), cost, true, ti);
		return doMove(TryMoveHero::DISEMBARK, CHECK_FOR_GUARDS, VISIT_DEST, LEAVING_TILE);
	}

	if (teleporting)
	{
		if (blockingVisit()) // e.g. hero on the other side of teleporter
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
		tmh.movePoints = (int)h->movementPointsRemaining() >= cost
						? h->movementPointsRemaining() - cost
						: 0;

		EGuardLook lookForGuards = CHECK_FOR_GUARDS;
		EVisitDest visitDest = VISIT_DEST;
		if (transit)
		{
			if (CGTeleport::isTeleport(t.topVisitableObj()))
				visitDest = DONT_VISIT_DEST;

			if (canFly)
			{
				lookForGuards = IGNORE_GUARDS;
				visitDest = DONT_VISIT_DEST;
			}
		}
		else if (blockingVisit())
			return true;
		
		if(h->boat && !h->boat->onboardAssaultAllowed)
		   lookForGuards = IGNORE_GUARDS;

		doMove(TryMoveHero::SUCCESS, lookForGuards, visitDest, LEAVING_TILE);
		return true;
	}
}

bool CGameHandler::teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker)
{
	const CGHeroInstance *h = getHero(hid);
	const CGTownInstance *t = getTown(dstid);

	if (!h || !t)
		COMPLAIN_RET("Invalid call to teleportHero!");

	const CGTownInstance *from = h->visitedTown;
	if (((h->getOwner() != t->getOwner())
		&& complain("Cannot teleport hero to another player"))

	|| (from->town->faction->getId() != t->town->faction->getId()
		&& complain("Source town and destination town should belong to the same faction"))

	|| ((!from || !from->hasBuilt(BuildingSubID::CASTLE_GATE))
		&& complain("Hero must be in town with Castle gate for teleporting"))

	|| (!t->hasBuilt(BuildingSubID::CASTLE_GATE)
		&& complain("Cannot teleport hero to town without Castle gate in it")))
			return false;

	int3 pos = h->convertFromVisitablePos(t->visitablePos());
	moveHero(hid,pos,1);
	return true;
}

void CGameHandler::setOwner(const CGObjectInstance * obj, const PlayerColor owner)
{
	PlayerColor oldOwner = getOwner(obj->id);
	SetObjectProperty sop(obj->id, ObjProperty::OWNER, owner.getNum());
	sendAndApply(&sop);

	std::set<PlayerColor> playerColors = {owner, oldOwner};
	checkVictoryLossConditions(playerColors);

	const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(obj);
	if (town) //town captured
	{
		if (owner.isValidPlayer()) //new owner is real player
		{
			if (town->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
				setPortalDwelling(town, true, false);
		}

		if (oldOwner.isValidPlayer()) //old owner is real player
		{
			if (getPlayerState(oldOwner)->towns.empty() && getPlayerState(oldOwner)->status != EPlayerStatus::LOSER) //previous player lost last last town
			{
				InfoWindow iw;
				iw.player = oldOwner;
				iw.text.appendLocalString(EMetaText::GENERAL_TXT, 6); //%s, you have lost your last town. If you do not conquer another town in the next week, you will be eliminated.
				iw.text.replaceLocalString(EMetaText::COLOR, oldOwner.getNum());
				sendAndApply(&iw);
			}
		}
	}

	const PlayerState * p = getPlayerState(owner);

	if ((obj->ID == Obj::CREATURE_GENERATOR1 || obj->ID == Obj::CREATURE_GENERATOR4) && p && p->dwellings.size()==1)//first dwelling captured
	{
		for (const CGTownInstance * t : getPlayerState(owner)->towns)
		{
			if (t->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
				setPortalDwelling(t);//set initial creatures for all portals of summoning
		}
	}
}

void CGameHandler::showBlockingDialog(BlockingDialog *iw)
{
	auto dialogQuery = std::make_shared<CBlockingDialogQuery>(this, *iw);
	queries->addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(iw);
}

void CGameHandler::showTeleportDialog(TeleportDialog *iw)
{
	auto dialogQuery = std::make_shared<CTeleportDialogQuery>(this, *iw);
	queries->addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(iw);
}

void CGameHandler::giveResource(PlayerColor player, GameResID which, int val) //TODO: cap according to Bersy's suggestion
{
	if (!val) return; //don't waste time on empty call

	TResources resources;
	resources[which] = val;
	giveResources(player, resources);
}

void CGameHandler::giveResources(PlayerColor player, TResources resources)
{
	SetResources sr;
	sr.abs = false;
	sr.player = player;
	sr.res = resources;
	sendAndApply(&sr);
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

	for (CStackBasicDescriptor &sbd : cres)
	{
		TQuantity collected = 0;
		while(collected < sbd.count)
		{
			bool foundSth = false;
			for (auto i = obj->Slots().begin(); i != obj->Slots().end(); i++)
			{
				if (i->second->type == sbd.type)
				{
					TQuantity take = std::min(sbd.count - collected, i->second->count); //collect as much cres as we can
					changeStackCount(StackLocation(obj, i->first), -take, false);
					collected += take;
					foundSth = true;
					break;
				}
			}

			if (!foundSth) //we went through the whole loop and haven't found appropriate cres
			{
				complain("Unexpected failure during taking creatures!");
				return;
			}
		}
	}
}

void CGameHandler::heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)
{
	HeroVisitCastle vc;
	vc.hid = hero->id;
	vc.tid = obj->id;
	vc.flags |= 1;
	sendAndApply(&vc);
	visitCastleObjects(obj, hero);
	giveSpells (obj, hero);
	checkVictoryLossConditionsForPlayer(hero->tempOwner); //transported artifact?
}

void CGameHandler::visitCastleObjects(const CGTownInstance * t, const CGHeroInstance * h)
{
	for (auto building : t->bonusingBuildings)
		building->onHeroVisit(h);
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

void CGameHandler::changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells)
{
	ChangeSpells cs;
	cs.hid = hero->id;
	cs.spells = spells;
	cs.learn = give;
	sendAndApply(&cs);
}

void CGameHandler::giveHeroBonus(GiveBonus * bonus)
{
	sendAndApply(bonus);
}

void CGameHandler::setMovePoints(SetMovePoints * smp)
{
	sendAndApply(smp);
}

void CGameHandler::setManaPoints(ObjectInstanceID hid, int val)
{
	SetMana sm;
	sm.hid = hid;
	sm.val = val;
	sm.absolute = true;
	sendAndApply(&sm);
}

void CGameHandler::giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId)
{
	GiveHero gh;
	gh.id = id;
	gh.player = player;
	gh.boatId = boatId;
	sendAndApply(&gh);

	//Reveal fow around new hero, especially released from Prison
	auto h = getHero(id);
	changeFogOfWar(h->pos, h->getSightRadius(), player, false);
}

void CGameHandler::changeObjPos(ObjectInstanceID objid, int3 newPos)
{
	ChangeObjPos cop;
	cop.objid = objid;
	cop.nPos = newPos;
	sendAndApply(&cop);
}

void CGameHandler::useScholarSkill(ObjectInstanceID fromHero, ObjectInstanceID toHero)
{
	const CGHeroInstance * h1 = getHero(fromHero);
	const CGHeroInstance * h2 = getHero(toHero);
	int h1_scholarSpellLevel = h1->valOfBonuses(BonusType::LEARN_MEETING_SPELL_LIMIT, -1);
	int h2_scholarSpellLevel = h2->valOfBonuses(BonusType::LEARN_MEETING_SPELL_LIMIT, -1);

	if (h1_scholarSpellLevel < h2_scholarSpellLevel)
	{
		std::swap (h1,h2);//1st hero need to have higher scholar level for correct message
		std::swap(fromHero, toHero);
	}

	int ScholarSpellLevel = std::max(h1_scholarSpellLevel, h2_scholarSpellLevel);//heroes can trade up to this level
	if (!ScholarSpellLevel || !h1->hasSpellbook() || !h2->hasSpellbook())
		return;//no scholar skill or no spellbook

	int h1Lvl = std::min(ScholarSpellLevel, h1->maxSpellLevel()),
	    h2Lvl = std::min(ScholarSpellLevel, h2->maxSpellLevel());//heroes can receive this levels

	ChangeSpells cs1;
	cs1.learn = true;
	cs1.hid = toHero;//giving spells to first hero
	for (auto it : h1->getSpellsInSpellbook())
		if (h2Lvl >= it.toSpell()->getLevel() && !h2->spellbookContainsSpell(it))//hero can learn it and don't have it yet
			cs1.spells.insert(it);//spell to learn

	ChangeSpells cs2;
	cs2.learn = true;
	cs2.hid = fromHero;

	for (auto it : h2->getSpellsInSpellbook())
		if (h1Lvl >= it.toSpell()->getLevel() && !h1->spellbookContainsSpell(it))
			cs2.spells.insert(it);

	if (!cs1.spells.empty() || !cs2.spells.empty())//create a message
	{
		int ScholarSkillLevel = std::max(h1->getSecSkillLevel(SecondarySkill::SCHOLAR),
		                                 h2->getSecSkillLevel(SecondarySkill::SCHOLAR));
		InfoWindow iw;
		iw.player = h1->tempOwner;
		iw.components.emplace_back(Component::EComponentType::SEC_SKILL, 18, ScholarSkillLevel, 0);

		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 139);//"%s, who has studied magic extensively,
		iw.text.replaceRawString(h1->getNameTranslated());

		if (!cs2.spells.empty())//if found new spell - apply
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 140);//learns
			int size = static_cast<int>(cs2.spells.size());
			for (auto it : cs2.spells)
			{
				iw.components.emplace_back(Component::EComponentType::SPELL, it, 1, 0);
				iw.text.appendLocalString(EMetaText::SPELL_NAME, it.toEnum());
				switch (size--)
				{
					case 2:
						iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);
					case 1:
						break;
					default:
						iw.text.appendRawString(", ");
				}
			}
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 142);//from %s
			iw.text.replaceRawString(h2->getNameTranslated());
			sendAndApply(&cs2);
		}

		if (!cs1.spells.empty() && !cs2.spells.empty())
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);//and
		}

		if (!cs1.spells.empty())
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 147);//teaches
			int size = static_cast<int>(cs1.spells.size());
			for (auto it : cs1.spells)
			{
				iw.components.emplace_back(Component::EComponentType::SPELL, it, 1, 0);
				iw.text.appendLocalString(EMetaText::SPELL_NAME, it.toEnum());
				switch (size--)
				{
					case 2:
						iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);
					case 1:
						break;
					default:
						iw.text.appendRawString(", ");
				}
			}
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 148);//from %s
			iw.text.replaceRawString(h2->getNameTranslated());
			sendAndApply(&cs1);
		}
		sendAndApply(&iw);
	}
}

void CGameHandler::heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)
{
	auto h1 = getHero(hero1), h2 = getHero(hero2);

	if (getPlayerRelations(h1->getOwner(), h2->getOwner()) != PlayerRelations::ENEMIES)
	{
		auto exchange = std::make_shared<CGarrisonDialogQuery>(this, h1, h2);
		ExchangeDialog hex;
		hex.queryID = exchange->queryID;
		hex.player = h1->getOwner();
		hex.hero1 = hero1;
		hex.hero2 = hero2;
		sendAndApply(&hex);

		useScholarSkill(hero1,hero2);
		queries->addQuery(exchange);
	}
}

void CGameHandler::sendToAllClients(CPackForClient * pack)
{
	logNetwork->trace("\tSending to all clients: %s", typeid(*pack).name());
	for (auto c : lobby->connections)
	{
		if(!c->isOpen())
			continue;

		c->sendPack(pack);
	}
}

void CGameHandler::sendAndApply(CPackForClient * pack)
{
	sendToAllClients(pack);
	gs->apply(pack);
	logNetwork->trace("\tApplied on gs: %s", typeid(*pack).name());
}

void CGameHandler::sendAndApply(CGarrisonOperationPack * pack)
{
	sendAndApply(static_cast<CPackForClient *>(pack));
	checkVictoryLossConditionsForAll();
}

void CGameHandler::sendAndApply(SetResources * pack)
{
	sendAndApply(static_cast<CPackForClient *>(pack));
	checkVictoryLossConditionsForPlayer(pack->player);
}

void CGameHandler::sendAndApply(NewStructures * pack)
{
	sendAndApply(static_cast<CPackForClient *>(pack));
	checkVictoryLossConditionsForPlayer(getTown(pack->tid)->tempOwner);
}

bool CGameHandler::isPlayerOwns(CPackForServer * pack, ObjectInstanceID id)
{
	return pack->player == getOwner(id) && hasPlayerAt(getOwner(id), pack->c);
}

void CGameHandler::throwNotAllowedAction(CPackForServer * pack)
{
	if(pack->c)
		playerMessages->sendSystemMessage(pack->c, "You are not allowed to perform this action!");

	logNetwork->error("Player is not allowed to perform this action!");
	throw ExceptionNotAllowedAction();
}

void CGameHandler::wrongPlayerMessage(CPackForServer * pack, PlayerColor expectedplayer)
{
	std::ostringstream oss;
	oss << "You were identified as player " << pack->player << " while expecting " << expectedplayer;
	logNetwork->error(oss.str());

	if(pack->c)
		playerMessages->sendSystemMessage(pack->c, oss.str());
}

void CGameHandler::throwIfWrongOwner(CPackForServer * pack, ObjectInstanceID id)
{
	if(!isPlayerOwns(pack, id))
	{
		wrongPlayerMessage(pack, getOwner(id));
		throwNotAllowedAction(pack);
	}
}

void CGameHandler::throwIfWrongPlayer(CPackForServer * pack)
{
	throwIfWrongPlayer(pack, pack->player);
}

void CGameHandler::throwIfWrongPlayer(CPackForServer * pack, PlayerColor player)
{
	if(!hasPlayerAt(player, pack->c) || pack->player != player)
	{
		wrongPlayerMessage(pack, player);
		throwNotAllowedAction(pack);
	}
}

void CGameHandler::throwAndComplain(CPackForServer * pack, std::string txt)
{
	complain(txt);
	throwNotAllowedAction(pack);
}

void CGameHandler::save(const std::string & filename)
{
	logGlobal->info("Saving to %s", filename);
	const auto stem	= FileInfo::GetPathStem(filename);
	const auto savefname = stem.to_string() + ".vsgm1";
	CResourceHandler::get("local")->createResource(savefname);

	try
	{
		{
			CSaveFile save(*CResourceHandler::get("local")->getResourceName(ResourceID(stem.to_string(), EResType::SAVEGAME)));
			saveCommonState(save);
			logGlobal->info("Saving server state");
			save << *this;
		}
		logGlobal->info("Game has been successfully saved!");
	}
	catch(std::exception &e)
	{
		logGlobal->error("Failed to save game: %s", e.what());
	}
}

bool CGameHandler::load(const std::string & filename)
{
	logGlobal->info("Loading from %s", filename);
	const auto stem	= FileInfo::GetPathStem(filename);

	reinitScripting();

	try
	{
		{
			CLoadFile lf(*CResourceHandler::get()->getResourceName(ResourceID(stem.to_string(), EResType::SAVEGAME)), MINIMAL_SERIALIZATION_VERSION);
			loadCommonState(lf);
			logGlobal->info("Loading server state");
			lf >> *this;
		}
		logGlobal->info("Game has been successfully loaded!");
	}
	catch(const ModIncompatibility & e)
	{
		logGlobal->error("Failed to load game: %s", e.what());
		auto errorMsg = VLC->generaltexth->translate("vcmi.server.errors.modsIncompatibility") + '\n';
		errorMsg += e.what();
		lobby->announceMessage(errorMsg);
		return false;
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Failed to load game: %s", e.what());
		return false;
	}
	gs->preInit(VLC);
	gs->updateOnLoad(lobby->si.get());
	return true;
}

bool CGameHandler::bulkSplitStack(SlotID slotSrc, ObjectInstanceID srcOwner, si32 howMany)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if((!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		|| (howMany < 1 && complain("Invalid split parameter!")))
	{
		return false;
	}
	auto actualAmount = army->getStackCount(slotSrc);

	if(actualAmount <= howMany && complain(complainNotEnoughCreatures)) // '<=' because it's not intended just for moving a stack
		return false;

	auto freeSlots = creatureSet.getFreeSlots();

	if(freeSlots.empty() && complain("No empty stacks"))
		return false;

	BulkRebalanceStacks bulkRS;

	for(auto slot : freeSlots)
	{
		RebalanceStacks rs;
		rs.srcArmy = army->id;
		rs.dstArmy = army->id;
		rs.srcSlot = slotSrc;
		rs.dstSlot = slot;
		rs.count = howMany;

		bulkRS.moves.push_back(rs);
		actualAmount -= howMany;

		if(actualAmount <= howMany)
			break;
	}
	sendAndApply(&bulkRS);
	return true;
}

bool CGameHandler::bulkMergeStacks(SlotID slotSrc, ObjectInstanceID srcOwner)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if(!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		return false;

	auto actualAmount = creatureSet.getStackCount(slotSrc);

	if(actualAmount < 1 && complain(complainNoCreatures))
		return false;

	auto currentCreature = creatureSet.getCreature(slotSrc);

	if(!currentCreature && complain(complainNoCreatures))
		return false;

	auto creatureSlots = creatureSet.getCreatureSlots(currentCreature, slotSrc);

	if(!creatureSlots.size())
		return false;

	BulkRebalanceStacks bulkRS;

	for(auto slot : creatureSlots)
	{
		RebalanceStacks rs;
		rs.srcArmy = army->id;
		rs.dstArmy = army->id;
		rs.srcSlot = slot;
		rs.dstSlot = slotSrc;
		rs.count = creatureSet.getStackCount(slot);
		bulkRS.moves.push_back(rs);
	}
	sendAndApply(&bulkRS);
	return true;
}

bool CGameHandler::bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot)
{
	if(!srcSlot.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * armySrc = static_cast<const CArmedInstance*>(getObjInstance(srcArmy));
	const CCreatureSet & setSrc = *armySrc;

	if(!vstd::contains(setSrc.stacks, srcSlot) && complain(complainNoCreatures))
		return false;

	const CArmedInstance * armyDest = static_cast<const CArmedInstance*>(getObjInstance(destArmy));
	const CCreatureSet & setDest = *armyDest;
	auto freeSlots = setDest.getFreeSlotsQueue();

	typedef std::map<SlotID, std::pair<SlotID, TQuantity>> TRebalanceMap;
	TRebalanceMap moves;

	auto srcQueue = setSrc.getCreatureQueue(srcSlot); // Exclude srcSlot, it should be moved last
	auto slotsLeft = setSrc.stacksCount();
	auto destMap = setDest.getCreatureMap();
	TMapCreatureSlot::key_compare keyComp = destMap.key_comp();

	while(!srcQueue.empty())
	{
		auto pair = srcQueue.top();
		srcQueue.pop();

		auto currCreature = pair.first;
		auto currSlot = pair.second;
		const auto quantity = setSrc.getStackCount(currSlot);

		TMapCreatureSlot::iterator lb = destMap.lower_bound(currCreature);
		const bool alreadyExists = (lb != destMap.end() && !(keyComp(currCreature, lb->first)));

		if(!alreadyExists)
		{
			if(freeSlots.empty())
				continue;

			auto currFreeSlot = freeSlots.front();
			freeSlots.pop();
			destMap.insert(lb, TMapCreatureSlot::value_type(currCreature, currFreeSlot));
		}
		moves.insert(std::make_pair(currSlot, std::make_pair(destMap[currCreature], quantity)));
		slotsLeft--;
	}
	if(slotsLeft == 1)
	{
		auto lastCreature = setSrc.getCreature(srcSlot);
		auto slotToMove = SlotID();
		// Try to find a slot for last creature
		if(destMap.find(lastCreature) == destMap.end())
		{
			if(!freeSlots.empty())
				slotToMove = freeSlots.front();
		}
		else
		{
			slotToMove = destMap[lastCreature];
		}

		if(slotToMove != SlotID())
		{
			const bool needsLastStack = armySrc->needsLastStack();
			const auto quantity = setSrc.getStackCount(srcSlot) - (needsLastStack ? 1 : 0);
			moves.insert(std::make_pair(srcSlot, std::make_pair(slotToMove, quantity)));
		}
	}
	BulkRebalanceStacks bulkRS;

	for(auto & move : moves)
	{
		RebalanceStacks rs;
		rs.srcArmy = armySrc->id;
		rs.dstArmy = armyDest->id;
		rs.srcSlot = move.first;
		rs.dstSlot = move.second.first;
		rs.count = move.second.second;
		bulkRS.moves.push_back(rs);
	}
	sendAndApply(&bulkRS);
	return true;
}

bool CGameHandler::bulkSmartSplitStack(SlotID slotSrc, ObjectInstanceID srcOwner)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if(!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		return false;

	auto actualAmount = creatureSet.getStackCount(slotSrc);

	if(actualAmount <= 1 && complain(complainNoCreatures))
		return false;

	auto freeSlot = creatureSet.getFreeSlot();
	auto currentCreature = creatureSet.getCreature(slotSrc);

	if(freeSlot == SlotID() && creatureSet.isCreatureBalanced(currentCreature))
		return true;

	auto creatureSlots = creatureSet.getCreatureSlots(currentCreature, SlotID(-1), 1); // Ignore slots where's only 1 creature, don't ignore slotSrc
	TQuantity totalCreatures = 0;

	for(auto slot : creatureSlots)
		totalCreatures += creatureSet.getStackCount(slot);

	if(totalCreatures <= 1 && complain("Total creatures number is invalid"))
		return false;

	if(freeSlot != SlotID())
		creatureSlots.push_back(freeSlot);

	if(creatureSlots.empty() && complain("No available slots for smart rebalancing"))
		return false;

	const auto totalCreatureSlots = creatureSlots.size();
	const auto rem = totalCreatures % totalCreatureSlots;
	const auto quotient = totalCreatures / totalCreatureSlots;

	// totalCreatures == rem * (quotient + 1) + (totalCreatureSlots - rem) * quotient;
	// Proof: r(q+1)+(s-r)q = rq+r+qs-rq = r+qs = total, where total/s = q+r/s

	BulkSmartRebalanceStacks bulkSRS;

	if(freeSlot != SlotID())
	{
		RebalanceStacks rs;
		rs.srcArmy = rs.dstArmy = army->id;
		rs.srcSlot = slotSrc;
		rs.dstSlot = freeSlot;
		rs.count = 1;
		bulkSRS.moves.push_back(rs);
	}
	auto currSlot = 0;
	auto check = 0;

	for(auto slot : creatureSlots)
	{
		ChangeStackCount csc;

		csc.army = army->id;
		csc.slot = slot;
		csc.count = (currSlot < rem)
			? quotient + 1
			: quotient;
		csc.absoluteValue = true;
		bulkSRS.changes.push_back(csc);
		currSlot++;
		check += csc.count;
	}

	if(check != totalCreatures)
	{
		complain((boost::format("Failure: totalCreatures=%d but check=%d") % totalCreatures % check).str());
		return false;
	}
	sendAndApply(&bulkSRS);
	return true;
}

bool CGameHandler::arrangeStacks(ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player)
{
	const CArmedInstance * s1 = static_cast<const CArmedInstance *>(getObjInstance(id1)),
		* s2 = static_cast<const CArmedInstance *>(getObjInstance(id2));
	const CCreatureSet &S1 = *s1, &S2 = *s2;
	StackLocation sl1(s1, p1), sl2(s2, p2);
	if (!sl1.slot.validSlot()  ||  !sl2.slot.validSlot())
	{
		complain(complainInvalidSlot);
		return false;
	}

	if (!isAllowedExchange(id1,id2))
	{
		complain("Cannot exchange stacks between these two objects!\n");
		return false;
	}

	// We can always put stacks into locked garrison, but not take them out of it
	auto notRemovable = [&](const CArmedInstance * army)
	{
		if (id1 != id2) // Stack arrangement inside locked garrison is allowed
		{
			auto g = dynamic_cast<const CGGarrison *>(army);
			if (g && !g->removableUnits)
			{
				complain("Stacks in this garrison are not removable!\n");
				return true;
			}
		}
		return false;
	};

	if (what==1) //swap
	{
		if (((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s1->getStackCount(p1))
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

		if (!s1->slotEmpty(p1) && !s2->slotEmpty(p2))
		{
			if (notRemovable(sl1.army) || notRemovable(sl2.army))
				return false;
		}
		if (s1->slotEmpty(p1) && notRemovable(sl2.army))
			return false;
		else if (s2->slotEmpty(p2) && notRemovable(sl1.army))
			return false;

		swapStacks(sl1, sl2);
	}
	else if (what==2)//merge
	{
		if ((s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot merge different creatures stacks!"))
		|| (((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s2->getStackCount(p2)) && complain("Can't take troops from another player!")))
			return false;

		if (s1->slotEmpty(p1) || s2->slotEmpty(p2))
		{
			complain("Cannot merge empty stack!");
			return false;
		}
		else if (notRemovable(sl1.army))
			return false;

		moveStack(sl1, sl2);
	}
	else if (what==3) //split
	{
		const int countToMove = val - s2->getStackCount(p2);
		const int countLeftOnSrc = s1->getStackCount(p1) - countToMove;

		if (  (s1->tempOwner != player && countLeftOnSrc < s1->getStackCount(p1))
			|| (s2->tempOwner != player && val < s2->getStackCount(p2)))
		{
			complain("Can't move troops of another player!");
			return false;
		}

		//general conditions checking
		if ((!vstd::contains(S1.stacks,p1) && complain(complainNoCreatures))
			|| (val<1  && complain(complainNoCreatures)) )
		{
			return false;
		}


		if (vstd::contains(S2.stacks,p2))	 //dest. slot not free - it must be "rebalancing"...
		{
			int total = s1->getStackCount(p1) + s2->getStackCount(p2);
			if ((total < val   &&   complain("Cannot split that stack, not enough creatures!"))
				|| (s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot rebalance different creatures stacks!"))
			)
			{
				return false;
			}

			if (notRemovable(sl1.army))
			{
				if (s1->getStackCount(p1) > countLeftOnSrc)
					return false;
			}
			else if (notRemovable(sl2.army))
			{
				if (s2->getStackCount(p1) < countLeftOnSrc)
					return false;
			}

			moveStack(sl1, sl2, countToMove);
			//S2.slots[p2]->count = val;
			//S1.slots[p1]->count = total - val;
		}
		else //split one stack to the two
		{
			if (s1->getStackCount(p1) < val)//not enough creatures
			{
				complain(complainNotEnoughCreatures);
				return false;
			}

			if (notRemovable(sl1.army))
				return false;

			moveStack(sl1, sl2, val);
		}

	}
	return true;
}

bool CGameHandler::hasPlayerAt(PlayerColor player, std::shared_ptr<CConnection> c) const
{
	return connections.at(player).count(c);
}

bool CGameHandler::disbandCreature(ObjectInstanceID id, SlotID pos)
{
	const CArmedInstance * s1 = static_cast<const CArmedInstance *>(getObjInstance(id));
	if (!vstd::contains(s1->stacks,pos))
	{
		complain("Illegal call to disbandCreature - no such stack in army!");
		return false;
	}

	eraseStack(StackLocation(s1, pos));
	return true;
}

bool CGameHandler::buildStructure(ObjectInstanceID tid, BuildingID requestedID, bool force)
{
	const CGTownInstance * t = getTown(tid);
	if(!t)
		COMPLAIN_RETF("No such town (ID=%s)!", tid);
	if(!t->town->buildings.count(requestedID))
		COMPLAIN_RETF("Town of faction %s does not have info about building ID=%s!", t->town->faction->getNameTranslated() % requestedID);
	if(t->hasBuilt(requestedID))
		COMPLAIN_RETF("Building %s is already built in %s", t->town->buildings.at(requestedID)->getNameTranslated() % t->getNameTranslated());

	const CBuilding * requestedBuilding = t->town->buildings.at(requestedID);

	//Vector with future list of built building and buildings in auto-mode that are not yet built.
	std::vector<const CBuilding*> remainingAutoBuildings;
	std::set<BuildingID> buildingsThatWillBe;

	//Check validity of request
	if(!force)
	{
		switch(requestedBuilding->mode)
		{
		case CBuilding::BUILD_NORMAL :
			if (canBuildStructure(t, requestedID) != EBuildingState::ALLOWED)
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

			if(upgradeNumber >= t->town->creatures.at(level).size())
			{
				complain(boost::str(boost::format("Error ecountered when building dwelling (bid=%s):"
													"no creature found (upgrade number %d, level %d!")
												% buildingID % upgradeNumber % level));
				return;
			}

			CCreature * crea = VLC->creh->objects.at(t->town->creatures.at(level).at(upgradeNumber));

			SetAvailableCreatures ssi;
			ssi.tid = t->id;
			ssi.creatures = t->creatures;
			if (ssi.creatures[level].second.empty()) // first creature in a dwelling
				ssi.creatures[level].first = crea->getGrowth();
			ssi.creatures[level].second.push_back(crea->getId());
			sendAndApply(&ssi);
		}
		if(t->town->buildings.at(buildingID)->subId == BuildingSubID::PORTAL_OF_SUMMONING)
		{
			setPortalDwelling(t);
		}
	};

	//Performs stuff that has to be done after new building is built
	auto processAfterBuiltStructure = [t, this](const BuildingID buildingID)
	{
		auto isMageGuild = (buildingID <= BuildingID::MAGES_GUILD_5 && buildingID >= BuildingID::MAGES_GUILD_1);
		auto isLibrary = isMageGuild ? false
			: t->town->buildings.at(buildingID)->subId == BuildingSubID::EBuildingSubID::LIBRARY;

		if(isMageGuild || isLibrary || (t->subID == ETownType::CONFLUX && buildingID == BuildingID::GRAIL))
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
		{
			buildingsThatWillBe.insert(build.first);
		}
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
			auto actualRequirements = t->genBuildingRequirements(autoBuilding->bid);

			if(actualRequirements.test(areRequirementsFullfilled))
				buildingsToAdd.push(autoBuilding);
		}
	}

	// FIXME: it's done before NewStructures applied because otherwise town window wont be properly updated on client. That should be actually fixed on client and not on server.
	for(auto builtID : ns.bid)
		processBeforeBuiltStructure(builtID);

	//Take cost
	if(!force)
		giveResources(t->tempOwner, -requestedBuilding->resources);

	//We know what has been built, apply changes. Do this as final step to properly update town window
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
		visitCastleObjects(t, t->visitingHero);
	if(t->garrisonHero)
		visitCastleObjects(t, t->garrisonHero);

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
// 		rb.source = BonusSource::TOWN_STRUCTURE;
// 		rb.id = 17;
// 		sendAndApply(&rb);
// 	}
	return true;
}

bool CGameHandler::recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dstid, CreatureID crid, ui32 cram, si32 fromLvl)
{
	const CGDwelling * dw = static_cast<const CGDwelling *>(getObj(objid));
	const CArmedInstance *dst = nullptr;
	const CCreature *c = VLC->creh->objects.at(crid);
	const bool warMachine = c->warMachine != ArtifactID::NONE;

	//TODO: test for owning
	//TODO: check if dst can recruit objects (e.g. hero is actually visiting object, town and source are same, etc)
	dst = dynamic_cast<const CArmedInstance*>(getObj(dstid));

	assert(dw && dst);

	//verify
	bool found = false;
	int level = 0;

	for (; level < dw->creatures.size(); level++) //iterate through all levels
	{
		if ((fromLvl != -1) && (level !=fromLvl))
			continue;
		const auto &cur = dw->creatures.at(level); //current level info <amount, list of cr. ids>
		int i = 0;
		for (; i < cur.second.size(); i++) //look for crid among available creatures list on current level
			if (cur.second.at(i) == crid)
				break;

		if (i < cur.second.size())
		{
			found = true;
			cram = std::min(cram, cur.first); //reduce recruited amount up to available amount
			break;
		}
	}
	SlotID slot = dst->getSlotFor(crid);

	if ((!found && complain("Cannot recruit: no such creatures!"))
		|| ((si32)cram  >  VLC->creh->objects.at(crid)->maxAmount(getPlayerState(dst->tempOwner)->resources) && complain("Cannot recruit: lack of resources!"))
		|| (cram<=0  &&  complain("Cannot recruit: cram <= 0!"))
		|| (!slot.validSlot()  && !warMachine && complain("Cannot recruit: no available slot!")))
	{
		return false;
	}

	//recruit
	giveResources(dst->tempOwner, -(c->getFullRecruitCost() * cram));

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = dw->creatures;
	sac.creatures[level].first -= cram;
	sendAndApply(&sac);

	if (warMachine)
	{
		const CGHeroInstance *h = dynamic_cast<const CGHeroInstance*>(dst);

		COMPLAIN_RET_FALSE_IF(!h, "Only hero can buy war machines");

		ArtifactID artId = c->warMachine;

		COMPLAIN_RET_FALSE_IF(artId == ArtifactID::CATAPULT, "Catapult cannot be recruited!");

		const CArtifact * art = artId.toArtifact();

		COMPLAIN_RET_FALSE_IF(nullptr == art, "Invalid war machine artifact");

		return giveHeroNewArtifact(h, art);
	}
	else
	{
		addToSlot(StackLocation(dst, slot), c, cram);
	}
	return true;
}

bool CGameHandler::upgradeCreature(ObjectInstanceID objid, SlotID pos, CreatureID upgID)
{
	const CArmedInstance * obj = static_cast<const CArmedInstance *>(getObjInstance(objid));
	if (!obj->hasStackAtSlot(pos))
	{
		COMPLAIN_RET("Cannot upgrade, no stack at slot " + std::to_string(pos));
	}
	UpgradeInfo ui;
	fillUpgradeInfo(obj, pos, ui);
	PlayerColor player = obj->tempOwner;
	const PlayerState *p = getPlayerState(player);
	int crQuantity = obj->stacks.at(pos)->count;
	int newIDpos= vstd::find_pos(ui.newID, upgID);//get position of new id in UpgradeInfo

	//check if upgrade is possible
	if ((ui.oldID == CreatureID::NONE || newIDpos == -1) && complain("That upgrade is not possible!"))
	{
		return false;
	}
	TResources totalCost = ui.cost.at(newIDpos) * crQuantity;

	//check if player has enough resources
	if (!p->resources.canAfford(totalCost))
		COMPLAIN_RET("Cannot upgrade, not enough resources!");

	//take resources
	giveResources(player, -totalCost);

	//upgrade creature
	changeStackType(StackLocation(obj, pos), VLC->creh->objects.at(upgID));
	return true;
}

bool CGameHandler::changeStackType(const StackLocation &sl, const CCreature *c)
{
	if (!sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to change type");

	SetStackType sst;
	sst.army = sl.army->id;
	sst.slot = sl.slot;
	sst.type = c->getId();
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
		if (!pos.validSlot())
		{
			//try to merge two other stacks to make place
			std::pair<SlotID, SlotID> toMerge;
			if (dst->mergableStacks(toMerge, i->first) && allowMerging)
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

bool CGameHandler::swapGarrisonOnSiege(ObjectInstanceID tid)
{
	const CGTownInstance * town = getTown(tid);

	if(!town->garrisonHero == !town->visitingHero)
		return false;

	SetHeroesInTown intown;
	intown.tid = tid;

	if(town->garrisonHero) //garrison -> vising
	{
		intown.garrison = ObjectInstanceID();
		intown.visiting = town->garrisonHero->id;
	}
	else //visiting -> garrison
	{
		if(town->armedGarrison())
			town->mergeGarrisonOnSiege();

		intown.visiting = ObjectInstanceID();
		intown.garrison = town->visitingHero->id;
	}
	sendAndApply(&intown);
	return true;
}

bool CGameHandler::garrisonSwap(ObjectInstanceID tid)
{
	const CGTownInstance * town = getTown(tid);
	if (!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{

		if (!town->visitingHero->canBeMergedWith(*town))
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
		if (getHeroCount(town->garrisonHero->tempOwner,false) >= 8)
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
	else if (!!town->garrisonHero && town->visitingHero) //swap visiting and garrison hero
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
	const bool isDstSlotBackpack = ArtifactUtils::isSlotBackpack(dst.slot);

	if(srcArtifact == nullptr)
		COMPLAIN_RET("No artifact to move!");
	if(destArtifact && srcPlayer != dstPlayer && !isDstSlotBackpack)
		COMPLAIN_RET("Can't touch artifact on hero of another player!");

	// Check if src/dest slots are appropriate for the artifacts exchanged.
	// Moving to the backpack is always allowed.
	if((!srcArtifact || !isDstSlotBackpack)
		&& srcArtifact && !srcArtifact->canBePutAt(dst, true))
		COMPLAIN_RET("Cannot move artifact!");

	auto srcSlot = src.getSlot();
	auto dstSlot = dst.getSlot();

	if((srcSlot && srcSlot->locked) || (dstSlot && dstSlot->locked))
		COMPLAIN_RET("Cannot move artifact locks.");

	if(isDstSlotBackpack && srcArtifact->artType->isBig())
		COMPLAIN_RET("Cannot put big artifacts in backpack!");
	if(src.slot == ArtifactPosition::MACH4 || dst.slot == ArtifactPosition::MACH4)
		COMPLAIN_RET("Cannot move catapult!");

	if(isDstSlotBackpack)
	{
		if(!ArtifactUtils::isBackpackFreeSlots(dst.getHolderArtSet()))
			COMPLAIN_RET("Backpack is full!");
		vstd::amin(dst.slot, ArtifactPosition::BACKPACK_START + dst.getHolderArtSet()->artifactsInBackpack.size());
	}

	if(!(src.slot == ArtifactPosition::TRANSITION_POS && dst.slot == ArtifactPosition::TRANSITION_POS))
	{
		if(src.slot == dst.slot && src.artHolder == dst.artHolder)
			COMPLAIN_RET("Won't move artifact: Dest same as source!");

		// Check if dst slot is occupied
		if(!isDstSlotBackpack && destArtifact)
		{
			// Previous artifact must be removed first
			moveArtifact(dst, ArtifactLocation(dst.artHolder, ArtifactPosition::TRANSITION_POS));
		}

		try
		{
			auto hero = std::get<ConstTransitivePtr<CGHeroInstance>>(dst.artHolder);
			if(ArtifactUtils::checkSpellbookIsNeeded(hero, srcArtifact->artType->getId(), dst.slot))
				giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);
		}
		catch(const std::bad_variant_access &)
		{
			// object other than hero received an art - ignore
		}

		MoveArtifact ma(&src, &dst);
		if(dst.slot == ArtifactPosition::TRANSITION_POS)
			ma.askAssemble = false;
		sendAndApply(&ma);
	}
	return true;
}

bool CGameHandler::bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap)
{
	// Make sure exchange is even possible between the two heroes.
	if(!isAllowedExchange(srcHero, dstHero))
		COMPLAIN_RET("That heroes cannot make any exchange!");

	auto psrcHero = getHero(srcHero);
	auto pdstHero = getHero(dstHero);
	if((!psrcHero) || (!pdstHero))
		COMPLAIN_RET("bulkMoveArtifacts: wrong hero's ID");

	BulkMoveArtifacts ma(static_cast<ConstTransitivePtr<CGHeroInstance>>(psrcHero),
		static_cast<ConstTransitivePtr<CGHeroInstance>>(pdstHero), swap);
	auto & slotsSrcDst = ma.artsPack0;
	auto & slotsDstSrc = ma.artsPack1;

	// Temporary fitting set for artifacts. Used to select available slots before sending data.
	CArtifactFittingSet artFittingSet(pdstHero->bearerType());

	auto moveArtifact = [this, &artFittingSet](const CArtifactInstance * artifact,
		ArtifactPosition srcSlot, const CGHeroInstance * dstHero,
		std::vector<BulkMoveArtifacts::LinkedSlots> & slots) -> void
	{
		assert(artifact);
		auto dstSlot = ArtifactUtils::getArtAnyPosition(&artFittingSet, artifact->getTypeId());
		if(dstSlot != ArtifactPosition::PRE_FIRST)
		{
			artFittingSet.putArtifact(dstSlot, static_cast<ConstTransitivePtr<CArtifactInstance>>(artifact));
			slots.push_back(BulkMoveArtifacts::LinkedSlots(srcSlot, dstSlot));

			if(ArtifactUtils::checkSpellbookIsNeeded(dstHero, artifact->getTypeId(), dstSlot))
				giveHeroNewArtifact(dstHero, VLC->arth->objects[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);
		}
	};

	if(swap)
	{
		auto moveArtsWorn = [moveArtifact](const CGHeroInstance * srcHero, const CGHeroInstance * dstHero,
			std::vector<BulkMoveArtifacts::LinkedSlots> & slots) -> void
		{
			for(auto & artifact : srcHero->artifactsWorn)
			{
				if(ArtifactUtils::isArtRemovable(artifact))
					moveArtifact(artifact.second.getArt(), artifact.first, dstHero, slots);
			}
		};
		auto moveArtsInBackpack = [](const CArtifactSet * artSet,
			std::vector<BulkMoveArtifacts::LinkedSlots> & slots) -> void
		{
			for(auto & slotInfo : artSet->artifactsInBackpack)
			{
				auto slot = artSet->getArtPos(slotInfo.artifact);
				slots.push_back(BulkMoveArtifacts::LinkedSlots(slot, slot));
			}
		};
		// Move over artifacts that are worn srcHero -> dstHero
		moveArtsWorn(psrcHero, pdstHero, slotsSrcDst);
		artFittingSet.artifactsWorn.clear();
		// Move over artifacts that are worn dstHero -> srcHero
		moveArtsWorn(pdstHero, psrcHero, slotsDstSrc);
		// Move over artifacts that are in backpack srcHero -> dstHero
		moveArtsInBackpack(psrcHero, slotsSrcDst);
		// Move over artifacts that are in backpack dstHero -> srcHero
		moveArtsInBackpack(pdstHero, slotsDstSrc);
	}
	else
	{
		artFittingSet.artifactsInBackpack = pdstHero->artifactsInBackpack;
		artFittingSet.artifactsWorn = pdstHero->artifactsWorn;

		// Move over artifacts that are worn
		for(auto & artInfo : psrcHero->artifactsWorn)
		{
			if(ArtifactUtils::isArtRemovable(artInfo))
			{
				moveArtifact(psrcHero->getArt(artInfo.first), artInfo.first, pdstHero, slotsSrcDst);
			}
		}
		// Move over artifacts that are in backpack
		for(auto & slotInfo : psrcHero->artifactsInBackpack)
		{
			moveArtifact(psrcHero->getArt(psrcHero->getArtPos(slotInfo.artifact)),
				psrcHero->getArtPos(slotInfo.artifact), pdstHero, slotsSrcDst);
		}
	}
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
	const CGHeroInstance * hero = getHero(heroID);
	const CArtifactInstance * destArtifact = hero->getArt(artifactSlot);

	if(!destArtifact)
		COMPLAIN_RET("assembleArtifacts: there is no such artifact instance!");

	if(assemble)
	{
		CArtifact * combinedArt = VLC->arth->objects[assembleTo];
		if(!combinedArt->isCombined())
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to assemble is not a combined artifacts!");
		if (!vstd::contains(ArtifactUtils::assemblyPossibilities(hero, destArtifact->getTypeId(),
			ArtifactUtils::isSlotEquipment(artifactSlot)), combinedArt))
		{
			COMPLAIN_RET("assembleArtifacts: It's impossible to assemble requested artifact!");
		}

		
		if(ArtifactUtils::checkSpellbookIsNeeded(hero, assembleTo, artifactSlot))
			giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);

		AssembledArtifact aa;
		aa.al = ArtifactLocation(hero, artifactSlot);
		aa.builtArt = combinedArt;
		sendAndApply(&aa);
	}
	else
	{
		if(!destArtifact->isCombined())
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble is not a combined artifact!");

		if(ArtifactUtils::isSlotBackpack(artifactSlot)
			&& !ArtifactUtils::isBackpackFreeSlots(hero, destArtifact->artType->getConstituents().size() - 1))
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble but backpack is full!");

		DisassembledArtifact da;
		da.al = ArtifactLocation(hero, artifactSlot);
		sendAndApply(&da);
	}

	return true;
}

bool CGameHandler::eraseArtifactByClient(const ArtifactLocation & al)
{
	const auto * hero = getHero(al.relatedObj()->id);
	if(hero == nullptr)
		COMPLAIN_RET("eraseArtifactByClient: wrong hero's ID");

	const auto * art = al.getArt();
	if(art == nullptr)
		COMPLAIN_RET("Cannot remove artifact!");

	if(al.getArt()->artType->canBePutAt(hero) || al.slot != ArtifactPosition::TRANSITION_POS)
		COMPLAIN_RET("Illegal artifact removal request");

	removeArtifact(al);
	return true;
}

bool CGameHandler::buyArtifact(ObjectInstanceID hid, ArtifactID aid)
{
	const CGHeroInstance * hero = getHero(hid);
	COMPLAIN_RET_FALSE_IF(nullptr == hero, "Invalid hero index");
	const CGTownInstance * town = hero->visitedTown;
	COMPLAIN_RET_FALSE_IF(nullptr == town, "Hero not in town");

	if (aid==ArtifactID::SPELLBOOK)
	{
		if ((!town->hasBuilt(BuildingID::MAGES_GUILD_1) && complain("Cannot buy a spellbook, no mage guild in the town!"))
		    || (getResource(hero->getOwner(), EGameResID::GOLD) < GameConstants::SPELLBOOK_GOLD_COST && complain("Cannot buy a spellbook, not enough gold!"))
		    || (hero->getArt(ArtifactPosition::SPELLBOOK) && complain("Cannot buy a spellbook, hero already has a one!"))
		   )
			return false;

		giveResource(hero->getOwner(),EGameResID::GOLD,-GameConstants::SPELLBOOK_GOLD_COST);
		giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);
		assert(hero->getArt(ArtifactPosition::SPELLBOOK));
		giveSpells(town,hero);
		return true;
	}
	else
	{
		const CArtifact * art = aid.toArtifact();
		COMPLAIN_RET_FALSE_IF(nullptr == art, "Invalid artifact index to buy");
		COMPLAIN_RET_FALSE_IF(art->getWarMachine() == CreatureID::NONE, "War machine artifact required");
		COMPLAIN_RET_FALSE_IF(hero->hasArt(aid),"Hero already has this machine!");
		const int price = art->getPrice();
		COMPLAIN_RET_FALSE_IF(getPlayerState(hero->getOwner())->resources[EGameResID::GOLD] < price, "Not enough gold!");

		if ((town->hasBuilt(BuildingID::BLACKSMITH) && town->town->warMachine == aid)
		 || (town->hasBuilt(BuildingSubID::BALLISTA_YARD) && aid == ArtifactID::BALLISTA))
		{
			giveResource(hero->getOwner(),EGameResID::GOLD,-price);
			return giveHeroNewArtifact(hero, art);
		}
		else
			COMPLAIN_RET("This machine is unavailable here!");
	}
}

bool CGameHandler::buyArtifact(const IMarket *m, const CGHeroInstance *h, GameResID rid, ArtifactID aid)
{
	if(!h)
		COMPLAIN_RET("Only hero can buy artifacts!");

	if (!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), aid))
		COMPLAIN_RET("That artifact is unavailable!");

	int b1, b2;
	m->getOffer(rid, aid, b1, b2, EMarketMode::RESOURCE_ARTIFACT);

	if (getResource(h->tempOwner, rid) < b1)
		COMPLAIN_RET("You can't afford to buy this artifact!");

	giveResource(h->tempOwner, rid, -b1);

	SetAvailableArtifacts saa;
	if(dynamic_cast<const CGTownInstance *>(m))
	{
		saa.id = -1;
		saa.arts = CGTownInstance::merchantArtifacts;
	}
	else if(const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(m)) //black market
	{
		saa.id = bm->id.getNum();
		saa.arts = bm->artifacts;
	}
	else
		COMPLAIN_RET("Wrong marktet...");

	bool found = false;
	for (const CArtifact *&art : saa.arts)
	{
		if (art && art->getId() == aid)
		{
			art = nullptr;
			found = true;
			break;
		}
	}

	if (!found)
		COMPLAIN_RET("Cannot find selected artifact on the list");

	sendAndApply(&saa);
	giveHeroNewArtifact(h, VLC->arth->objects[aid], ArtifactPosition::FIRST_AVAILABLE);
	return true;
}

bool CGameHandler::sellArtifact(const IMarket *m, const CGHeroInstance *h, ArtifactInstanceID aid, GameResID rid)
{
	COMPLAIN_RET_FALSE_IF((!h), "Only hero can sell artifacts!");
	const CArtifactInstance *art = h->getArtByInstanceId(aid);
	COMPLAIN_RET_FALSE_IF((!art), "There is no artifact to sell!");
	COMPLAIN_RET_FALSE_IF((!art->artType->isTradable()), "Cannot sell a war machine or spellbook!");

	int resVal = 0, dump = 1;
	m->getOffer(art->artType->getId(), rid, dump, resVal, EMarketMode::ARTIFACT_RESOURCE);

	removeArtifact(ArtifactLocation(h, h->getArtPos(art)));
	giveResource(h->tempOwner, rid, resVal);
	return true;
}

bool CGameHandler::buySecSkill(const IMarket *m, const CGHeroInstance *h, SecondarySkill skill)
{
	if (!h)
		COMPLAIN_RET("You need hero to buy a skill!");

	if (h->getSecSkillLevel(SecondarySkill(skill)))
		COMPLAIN_RET("Hero already know this skill");

	if (!h->canLearnSkill())
		COMPLAIN_RET("Hero can't learn any more skills");

	if (!h->canLearnSkill(skill))
		COMPLAIN_RET("The hero can't learn this skill!");

	if (!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_SKILL), skill))
		COMPLAIN_RET("That skill is unavailable!");

	if (getResource(h->tempOwner, EGameResID::GOLD) < GameConstants::SKILL_GOLD_COST)//TODO: remove hardcoded resource\summ?
		COMPLAIN_RET("You can't afford to buy this skill");

	giveResource(h->tempOwner, EGameResID::GOLD, -GameConstants::SKILL_GOLD_COST);

	changeSecSkill(h, skill, 1, true);
	return true;
}

bool CGameHandler::tradeResources(const IMarket *market, ui32 val, PlayerColor player, ui32 id1, ui32 id2)
{
	TResourceCap r1 = getPlayerState(player)->resources[id1];

	vstd::amin(val, r1); //can't trade more resources than have

	int b1, b2; //base quantities for trade
	market->getOffer(id1, id2, b1, b2, EMarketMode::RESOURCE_RESOURCE);
	int units = val / b1; //how many base quantities we trade

	if (val%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		COMPLAIN_RET("Invalid deal, not all offered units of resource were used.");
	}

	giveResource(player, GameResID(id1), - b1 * units);
	giveResource(player, GameResID(id2), b2 * units);

	return true;
}

bool CGameHandler::sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, SlotID slot, GameResID resourceID)
{
	if(!hero)
		COMPLAIN_RET("Only hero can sell creatures!");
	if (!vstd::contains(hero->Slots(), slot))
		COMPLAIN_RET("Hero doesn't have any creature in that slot!");

	const CStackInstance &s = hero->getStack(slot);

	if (s.count < (TQuantity)count //can't sell more creatures than have
		|| (hero->stacksCount() == 1 && hero->needsLastStack() && s.count == count)) //can't sell last stack
	{
		COMPLAIN_RET("Not enough creatures in army!");
	}

	int b1, b2; //base quantities for trade
	market->getOffer(s.type->getId(), resourceID, b1, b2, EMarketMode::CREATURE_RESOURCE);
	int units = count / b1; //how many base quantities we trade

	if (count%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		//TODO: complain?
		assert(0);
	}

	changeStackCount(StackLocation(hero, slot), -(TQuantity)count);

	giveResource(hero->tempOwner, resourceID, b2 * units);

	return true;
}

bool CGameHandler::transformInUndead(const IMarket *market, const CGHeroInstance * hero, SlotID slot)
{
	const CArmedInstance *army = nullptr;
	if (hero)
		army = hero;
	else
		army = dynamic_cast<const CGTownInstance *>(market);

	if (!army)
		COMPLAIN_RET("Incorrect call to transform in undead!");
	if (!army->hasStackAtSlot(slot))
		COMPLAIN_RET("Army doesn't have any creature in that slot!");


	const CStackInstance &s = army->getStack(slot);

	//resulting creature - bone dragons or skeletons
	CreatureID resCreature = CreatureID::SKELETON;

	if ((s.hasBonusOfType(BonusType::DRAGON_NATURE)
			&& !(s.hasBonusOfType(BonusType::UNDEAD)))
			|| (s.getCreatureID() == CreatureID::HYDRA)
			|| (s.getCreatureID() == CreatureID::CHAOS_HYDRA))
		resCreature = CreatureID::BONE_DRAGON;
	changeStackType(StackLocation(army, slot), resCreature.toCreature());
	return true;
}

bool CGameHandler::sendResources(ui32 val, PlayerColor player, GameResID r1, PlayerColor r2)
{
	const PlayerState *p2 = getPlayerState(r2, false);
	if (!p2  ||  p2->status != EPlayerStatus::INGAME)
	{
		complain("Dest player must be in game!");
		return false;
	}

	TResourceCap curRes1 = getPlayerState(player)->resources[r1];

	vstd::amin(val, curRes1);

	giveResource(player, r1, -(int)val);
	giveResource(r2, r1, val);

	return true;
}

bool CGameHandler::setFormation(ObjectInstanceID hid, ui8 formation)
{
	const CGHeroInstance *h = getHero(hid);
	if (!h)
	{
		logGlobal->error("Hero doesn't exist!");
		return false;
	}

	ChangeFormation cf;
	cf.hid = hid;
	cf.formation = formation;
	sendAndApply(&cf);

	return true;
}

bool CGameHandler::queryReply(QueryID qid, const JsonNode & answer, PlayerColor player)
{
	boost::unique_lock<boost::recursive_mutex> lock(gsm);

	logGlobal->trace("Player %s attempts answering query %d with answer:", player, qid);
	logGlobal->trace(answer.toJson());

	auto topQuery = queries->topQuery(player);

	COMPLAIN_RET_FALSE_IF(!topQuery, "This player doesn't have any queries!");

	if(topQuery->queryID != qid)
	{
		auto currentQuery = queries->getQuery(qid);

		if(currentQuery != nullptr && currentQuery->endsByPlayerAnswer())
			currentQuery->setReply(answer);

		COMPLAIN_RET("This player top query has different ID!"); //topQuery->queryID != qid
	}
	COMPLAIN_RET_FALSE_IF(!topQuery->endsByPlayerAnswer(), "This query cannot be ended by player's answer!");

	topQuery->setReply(answer);
	queries->popQuery(topQuery);
	return true;
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

			const PlayerState * pinfo = getPlayerState(color, false); //do not output error if player does not exist

			if (pinfo  //player exists
				&& (ev.players & 1<<player) //event is enabled to this player
				&& ((ev.computerAffected && !pinfo->human)
					|| (ev.humanAffected && pinfo->human)
				)
			)
			{
				//give resources
				giveResources(color, ev.resources);

				//prepare dialog
				InfoWindow iw;
				iw.player = color;
				iw.text.appendRawString(ev.message);

				for (int i=0; i<ev.resources.size(); i++)
				{
					if (ev.resources[i]) //if resource is changed, we add it to the dialog
						iw.components.emplace_back(Component::EComponentType::RESOURCE,i,ev.resources[i],0);
				}

				sendAndApply(&iw); //show dialog
			}
		} //PLAYERS LOOP

		if (ev.nextOccurence)
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
		const PlayerState * pinfo = getPlayerState(player, false);

		if (pinfo  //player exists
			&& (ev.players & 1<<player.getNum()) //event is enabled to this player
			&& ((ev.computerAffected && !pinfo->human)
				|| (ev.humanAffected && pinfo->human)))
		{
			// dialog
			InfoWindow iw;
			iw.player = player;
			iw.text.appendRawString(ev.message);

			if (ev.resources.nonZero())
			{
				TResources was = n.res[player];
				n.res[player] += ev.resources;
				n.res[player].amax(0);

				for (int i=0; i<ev.resources.size(); i++)
					if (ev.resources[i] && pinfo->resources[i] != n.res.at(player)[i]) //if resource had changed, we add it to the dialog
						iw.components.emplace_back(Component::EComponentType::RESOURCE,i,n.res.at(player)[i]-was[i],0);

			}

			for (auto & i : ev.buildings)
			{
				if (!town->hasBuilt(i))
				{
					buildStructure(town->id, i, true);
					iw.components.emplace_back(Component::EComponentType::BUILDING, town->subID, i, 0);
				}
			}

			if (!ev.creatures.empty() && !vstd::contains(n.cres, town->id))
			{
				n.cres[town->id].tid = town->id;
				n.cres[town->id].creatures = town->creatures;
			}
			auto & sac = n.cres[town->id];

			for (si32 i=0;i<ev.creatures.size();i++) //creature growths
			{
				if (!town->creatures.at(i).second.empty() && ev.creatures.at(i) > 0)//there is dwelling
				{
					sac.creatures[i].first += ev.creatures.at(i);
					iw.components.emplace_back(Component::EComponentType::CREATURE,
							town->creatures.at(i).second.back(), ev.creatures.at(i), 0);
				}
			}
			sendAndApply(&iw); //show dialog
		}

		if (ev.nextOccurence)
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

bool CGameHandler::complain(const std::string &problem)
{
	playerMessages->broadcastSystemMessage("Server encountered a problem: " + problem);
	logGlobal->error(problem);
	return true;
}

void CGameHandler::showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits)
{
	//PlayerColor player = getOwner(hid);
	auto upperArmy = dynamic_cast<const CArmedInstance*>(getObj(upobj));
	auto lowerArmy = dynamic_cast<const CArmedInstance*>(getObj(hid));

	assert(lowerArmy);
	assert(upperArmy);

	auto garrisonQuery = std::make_shared<CGarrisonDialogQuery>(this, upperArmy, lowerArmy);
	queries->addQuery(garrisonQuery);

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
	ow.window = EOpenWindowMode::THIEVES_GUILD;
	ow.id1 = player.getNum();
	ow.id2 = requestingObjId.getNum();
	sendAndApply(&ow);
}

bool CGameHandler::isAllowedExchange(ObjectInstanceID id1, ObjectInstanceID id2)
{
	if (id1 == id2)
		return true;

	const CGObjectInstance *o1 = getObj(id1), *o2 = getObj(id2);
	if (!o1 || !o2)
		return true; //arranging stacks within an object should be always allowed

	if (o1 && o2)
	{
		if (o1->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o1);
			if (t->visitingHero == o2  ||  t->garrisonHero == o2)
				return true;
		}
		if (o2->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o2);
			if (t->visitingHero == o1  ||  t->garrisonHero == o1)
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

		//Ongoing garrison exchange - usually picking from top garison (from o1 to o2), but who knows
		auto dialog = std::dynamic_pointer_cast<CGarrisonDialogQuery>(queries->topQuery(o1->tempOwner));
		if (!dialog)
		{
			dialog = std::dynamic_pointer_cast<CGarrisonDialogQuery>(queries->topQuery(o2->tempOwner));
		}
		if (dialog)
		{
			auto topArmy = dialog->exchangingArmies.at(0);
			auto bottomArmy = dialog->exchangingArmies.at(1);

			if ((topArmy == o1 && bottomArmy == o2) || (bottomArmy == o1 && topArmy == o2))
				return true;
		}
	}

	return false;
}

void CGameHandler::objectVisited(const CGObjectInstance * obj, const CGHeroInstance * h)
{
	using events::ObjectVisitStarted;

	logGlobal->debug("%s visits %s (%d:%d)", h->nodeName(), obj->getObjectName(), obj->ID, obj->subID);

	std::shared_ptr<CObjectVisitQuery> visitQuery;

	auto startVisit = [&](ObjectVisitStarted & event)
	{
		auto visitedObject = obj;

		if(obj->ID == Obj::HERO)
		{
			auto visitedHero = static_cast<const CGHeroInstance *>(obj);
			const auto visitedTown = visitedHero->visitedTown;

			if(visitedTown)
			{
				const bool isEnemy = visitedHero->getOwner() != h->getOwner();

				if(isEnemy && !visitedTown->isBattleOutsideTown(visitedHero))
					visitedObject = visitedTown;
			}
		}
		visitQuery = std::make_shared<CObjectVisitQuery>(this, visitedObject, h, visitedObject->visitablePos());
		queries->addQuery(visitQuery); //TODO real visit pos

		HeroVisit hv;
		hv.objId = obj->id;
		hv.heroId = h->id;
		hv.player = h->tempOwner;
		hv.starting = true;
		sendAndApply(&hv);

		obj->onHeroVisit(h);
	};

	ObjectVisitStarted::defaultExecute(serverEventBus.get(), startVisit, h->tempOwner, h->id, obj->id);

	if(visitQuery)
		queries->popIfTop(visitQuery); //visit ends here if no queries were created
}

void CGameHandler::objectVisitEnded(const CObjectVisitQuery & query)
{
	using events::ObjectVisitEnded;

	logGlobal->debug("%s visit ends.\n", query.visitingHero->nodeName());

	auto endVisit = [&](ObjectVisitEnded & event)
	{
		HeroVisit hv;
		hv.player = event.getPlayer();
		hv.heroId = event.getHero();
		hv.starting = false;
		sendAndApply(&hv);
	};

	//TODO: ObjectVisitEnded should also have id of visited object,
	//but this requires object being deleted only by `removeAfterVisit()` but not `removeObject()`
	ObjectVisitEnded::defaultExecute(serverEventBus.get(), endVisit, query.players.front(), query.visitingHero->id);
}

bool CGameHandler::buildBoat(ObjectInstanceID objid, PlayerColor playerID)
{
	const IShipyard *obj = IShipyard::castFrom(getObj(objid));

	if (obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		complain("Cannot build boat in this shipyard!");
		return false;
	}

	TResources boatCost;
	obj->getBoatCost(boatCost);
	TResources aviable = getPlayerState(playerID)->resources;

	if (!aviable.canAfford(boatCost))
	{
		complain("Not enough resources to build a boat!");
		return false;
	}

	int3 tile = obj->bestLocation();
	if (!gs->map->isInTheMap(tile))
	{
		complain("Cannot find appropriate tile for a boat!");
		return false;
	}

	giveResources(playerID, -boatCost);
	createObject(tile, Obj::BOAT, obj->getBoatType().getNum());
	return true;
}

void CGameHandler::checkVictoryLossConditions(const std::set<PlayerColor> & playerColors)
{
	for (auto playerColor : playerColors)
	{
		if (getPlayerState(playerColor, false))
			checkVictoryLossConditionsForPlayer(playerColor);
	}
}

void CGameHandler::checkVictoryLossConditionsForAll()
{
	std::set<PlayerColor> playerColors;
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		playerColors.insert(PlayerColor(i));
	}
	checkVictoryLossConditions(playerColors);
}

void CGameHandler::checkVictoryLossConditionsForPlayer(PlayerColor player)
{
	const PlayerState * p = getPlayerState(player);

	if(!p || p->status != EPlayerStatus::INGAME) return;

	auto victoryLossCheckResult = gs->checkForVictoryAndLoss(player);

	if (victoryLossCheckResult.victory() || victoryLossCheckResult.loss())
	{
		InfoWindow iw;
		getVictoryLossMessage(player, victoryLossCheckResult, iw);
		sendAndApply(&iw);

		PlayerEndsGame peg;
		peg.player = player;
		peg.victoryLossCheckResult = victoryLossCheckResult;
		sendAndApply(&peg);

		turnOrder->onPlayerEndsGame(player);

		if (victoryLossCheckResult.victory())
		{
			//one player won -> all enemies lost
			for (auto i = gs->players.cbegin(); i!=gs->players.cend(); i++)
			{
				if (i->first != player && getPlayerState(i->first)->status == EPlayerStatus::INGAME)
				{
					peg.player = i->first;
					peg.victoryLossCheckResult = getPlayerRelations(player, i->first) == PlayerRelations::ALLIES ?
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
				lobby->setState(EServerState::GAMEPLAY_ENDED);
			}
		}
		else
		{
			//copy heroes vector to avoid iterator invalidation as removal change PlayerState
			auto hlp = p->heroes;
			for (auto h : hlp) //eliminate heroes
			{
				if (h.get())
					removeObject(h);
			}

			//player lost -> all his objects become unflagged (neutral)
			for (auto obj : gs->map->objects) //unflag objs
			{
				if (obj.get() && obj->tempOwner == player)
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
				if (getPlayerState(pc)->status == EPlayerStatus::INGAME)
				{
					InfoWindow iw;
					getVictoryLossMessage(player, victoryLossCheckResult.invert(), iw);
					iw.player = pc;
					sendAndApply(&iw);
				}
			}
			checkVictoryLossConditions(playerColors);
		}
	}
}

void CGameHandler::getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const
{
	out.player = player;
	out.text = victoryLossCheckResult.messageToSelf;
	out.text.replaceLocalString(EMetaText::COLOR, player.getNum());
	out.components.emplace_back(Component::EComponentType::FLAG, player.getNum(), 0, 0);
}

bool CGameHandler::dig(const CGHeroInstance *h)
{
	if (h->diggingStatus() != EDiggingStatus::CAN_DIG) //checks for terrain and movement
		COMPLAIN_RETF("Hero cannot dig (error code %d)!", static_cast<int>(h->diggingStatus()));

	createObject(h->visitablePos(), Obj::HOLE, 0 );

	//take MPs
	SetMovePoints smp;
	smp.hid = h->id;
	smp.val = 0;
	sendAndApply(&smp);

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->tempOwner;
	if (gs->map->grailPos == h->visitablePos())
	{
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 58); //"Congratulations! After spending many hours digging here, your hero has uncovered the "
		iw.text.appendLocalString(EMetaText::ART_NAMES, ArtifactID::GRAIL);
		iw.soundID = soundBase::ULTIMATEARTIFACT;
		giveHeroNewArtifact(h, VLC->arth->objects[ArtifactID::GRAIL], ArtifactPosition::FIRST_AVAILABLE); //give grail
		sendAndApply(&iw);

		iw.soundID = soundBase::invalid;
		iw.components.emplace_back(Component::EComponentType::ARTIFACT, ArtifactID::GRAIL, 0, 0);
		iw.text.clear();
		iw.text.appendLocalString(EMetaText::ART_DESCR, ArtifactID::GRAIL);
		sendAndApply(&iw);
	}
	else
	{
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 59); //"Nothing here. \n Where could it be?"
		iw.soundID = soundBase::Dig;
		sendAndApply(&iw);
	}

	return true;
}

void CGameHandler::visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h)
{
	if (!t.visitableObjects.empty())
	{
		//to prevent self-visiting heroes on space press
		if (t.visitableObjects.back() != h)
			objectVisited(t.visitableObjects.back(), h);
		else if (t.visitableObjects.size() > 1)
			objectVisited(*(t.visitableObjects.end()-2),h);
	}
}

bool CGameHandler::sacrificeCreatures(const IMarket * market, const CGHeroInstance * hero, const std::vector<SlotID> & slot, const std::vector<ui32> & count)
{
	if (!hero)
		COMPLAIN_RET("You need hero to sacrifice creature!");

	int expSum = 0;
	auto finish = [this, &hero, &expSum]()
	{
		changePrimSkill(hero, PrimarySkill::EXPERIENCE, hero->calculateXp(expSum));
	};

	for(int i = 0; i < slot.size(); ++i)
	{
		int oldCount = hero->getStackCount(slot[i]);

		if(oldCount < (int)count[i])
		{
			finish();
			COMPLAIN_RET("Not enough creatures to sacrifice!")
		}
		else if(oldCount == count[i] && hero->stacksCount() == 1 && hero->needsLastStack())
		{
			finish();
			COMPLAIN_RET("Cannot sacrifice last creature!");
		}

		int crid = hero->getStack(slot[i]).type->getId();

		changeStackCount(StackLocation(hero, slot[i]), -(TQuantity)count[i]);

		int dump, exp;
		market->getOffer(crid, 0, dump, exp, EMarketMode::CREATURE_EXP);
		exp *= count[i];
		expSum += exp;
	}

	finish();

	return true;
}

bool CGameHandler::sacrificeArtifact(const IMarket * m, const CGHeroInstance * hero, const std::vector<ArtifactPosition> & slot)
{
	if (!hero)
		COMPLAIN_RET("You need hero to sacrifice artifact!");

	int expSum = 0;
	auto finish = [this, &hero, &expSum]()
	{
		changePrimSkill(hero, PrimarySkill::EXPERIENCE, hero->calculateXp(expSum));
	};

	for(int i = 0; i < slot.size(); ++i)
	{
		ArtifactLocation al(hero, slot[i]);
		const CArtifactInstance * a = al.getArt();

		if(!a)
		{
			finish();
			COMPLAIN_RET("Cannot find artifact to sacrifice!");
		}

		const CArtifactInstance * art = hero->getArt(slot[i]);

		if(!art)
		{
			finish();
			COMPLAIN_RET("No artifact at position to sacrifice!");
		}

		si32 typId = art->artType->getId();
		int dmp, expToGive;

		m->getOffer(typId, 0, dmp, expToGive, EMarketMode::ARTIFACT_EXP);

		expSum += expToGive;

		removeArtifact(al);
	}

	finish();

	return true;
}

bool CGameHandler::insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	if (sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Slot is already taken!");

	if (!sl.slot.validSlot())
		COMPLAIN_RET("Cannot insert stack to that slot!");

	InsertNewStack ins;
	ins.army = sl.army->id;
	ins.slot = sl.slot;
	ins.type = c->getId();
	ins.count = count;
	sendAndApply(&ins);
	return true;
}

bool CGameHandler::eraseStack(const StackLocation &sl, bool forceRemoval)
{
	if (!sl.army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to erase");

	if (sl.army->stacksCount() == 1 //from the last stack
		&& sl.army->needsLastStack() //that must be left
		&& !forceRemoval) //ignore above conditions if we are forcing removal
	{
		COMPLAIN_RET("Cannot erase the last stack!");
	}

	EraseStack es;
	es.army = sl.army->id;
	es.slot = sl.slot;
	sendAndApply(&es);
	return true;
}

bool CGameHandler::changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue)
{
	TQuantity currentCount = sl.army->getStackCount(sl.slot);
	if ((absoluteValue && count < 0)
		|| (!absoluteValue && -count > currentCount))
	{
		COMPLAIN_RET("Cannot take more stacks than present!");
	}

	if ((currentCount == -count  &&  !absoluteValue)
	   || (!count && absoluteValue))
	{
		eraseStack(sl);
	}
	else
	{
		ChangeStackCount csc;
		csc.army = sl.army->id;
		csc.slot = sl.slot;
		csc.count = count;
		csc.absoluteValue = absoluteValue;
		sendAndApply(&csc);
	}
	return true;
}

bool CGameHandler::addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	const CCreature *slotC = sl.army->getCreature(sl.slot);
	if (!slotC) //slot is empty
		insertNewStack(sl, c, count);
	else if (c == slotC)
		changeStackCount(sl, count);
	else
	{
		COMPLAIN_RET("Cannot add " + c->getNamePluralTranslated() + " to slot " + boost::lexical_cast<std::string>(sl.slot) + "!");
	}
	return true;
}

void CGameHandler::tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging)
{
	if (removeObjWhenFinished)
		removeAfterVisit(src);

	if (!src->canBeMergedWith(*dst, allowMerging))
	{
		if (allowMerging) //do that, add all matching creatures.
		{
			bool cont = true;
			while (cont)
			{
				for (auto i = src->stacks.begin(); i != src->stacks.end(); i++)//while there are unmoved creatures
				{
					SlotID pos = dst->getSlotFor(i->second->type);
					if (pos.validSlot())
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
	if (!src.army->hasStackAtSlot(src.slot))
		COMPLAIN_RET("No stack to move!");

	if (dst.army->hasStackAtSlot(dst.slot) && dst.army->getCreature(dst.slot) != src.army->getCreature(src.slot))
		COMPLAIN_RET("Cannot move: stack of different type at destination pos!");

	if (!dst.slot.validSlot())
		COMPLAIN_RET("Cannot move stack to that slot!");

	if (count == -1)
	{
		count = src.army->getStackCount(src.slot);
	}

	if (src.army != dst.army  //moving away
		&&  count == src.army->getStackCount(src.slot) //all creatures
		&& src.army->stacksCount() == 1 //from the last stack
		&& src.army->needsLastStack()) //that must be left
	{
		COMPLAIN_RET("Cannot move away the last creature!");
	}

	RebalanceStacks rs;
	rs.srcArmy = src.army->id;
	rs.dstArmy = dst.army->id;
	rs.srcSlot = src.slot;
	rs.dstSlot = dst.slot;
	rs.count = count;
	sendAndApply(&rs);
	return true;
}

void CGameHandler::castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos)
{
	const CSpell * s = spellID.toSpell();
	if(!s)
		return;

	AdventureSpellCastParameters p;
	p.caster = caster;
	p.pos = pos;

	s->adventureCast(spellEnv, p);
}

bool CGameHandler::swapStacks(const StackLocation & sl1, const StackLocation & sl2)
{
	if(!sl1.army->hasStackAtSlot(sl1.slot))
	{
		return moveStack(sl2, sl1);
	}
	else if(!sl2.army->hasStackAtSlot(sl2.slot))
	{
		return moveStack(sl1, sl2);
	}
	else
	{
		SwapStacks ss;
		ss.srcArmy = sl1.army->id;
		ss.dstArmy = sl2.army->id;
		ss.srcSlot = sl1.slot;
		ss.dstSlot = sl2.slot;
		sendAndApply(&ss);
		return true;
	}
}

bool CGameHandler::giveHeroArtifact(const CGHeroInstance * h, const CArtifactInstance * a, ArtifactPosition pos)
{
	assert(a->artType);
	ArtifactLocation al(h, ArtifactPosition::PRE_FIRST);

	if(pos == ArtifactPosition::FIRST_AVAILABLE)
	{
		al.slot = ArtifactUtils::getArtAnyPosition(h, a->getTypeId());
	}
	else if(ArtifactUtils::isSlotBackpack(pos))
	{
		al.slot = ArtifactUtils::getArtBackpackPosition(h, a->getTypeId());
	}
	else
	{
		al.slot = pos;
	}

	if(a->canBePutAt(al))
		putArtifact(al, a);
	else
		return false;

	return true;
}

void CGameHandler::putArtifact(const ArtifactLocation &al, const CArtifactInstance *a)
{
	PutArtifact pa;
	pa.art = a;
	pa.al = al;
	sendAndApply(&pa);
}

bool CGameHandler::giveHeroNewArtifact(const CGHeroInstance * h, const CArtifact * artType, ArtifactPosition pos)
{
	assert(artType);

	if(pos == ArtifactPosition::FIRST_AVAILABLE)
	{
		if(!artType->canBePutAt(h, ArtifactUtils::getArtAnyPosition(h, artType->getId())))
			COMPLAIN_RET("Cannot put artifact in that slot!");
	}
	else if(ArtifactUtils::isSlotBackpack(pos))
	{
		if(!artType->canBePutAt(h, ArtifactUtils::getArtBackpackPosition(h, artType->getId())))
			COMPLAIN_RET("Cannot put artifact in that slot!");
	}
	else
	{
		COMPLAIN_RET_FALSE_IF(!artType->canBePutAt(h, pos, false), "Cannot put artifact in that slot!");
	}

	auto * newArtInst = new CArtifactInstance();
	newArtInst->artType = artType; // *NOT* via settype -> all bonus-related stuff must be done by NewArtifact apply

	NewArtifact na;
	na.art = newArtInst;
	sendAndApply(&na); // -> updates newArtInst!!!

	if(giveHeroArtifact(h, newArtInst, pos))
		return true;
	else
		return false;
}

void CGameHandler::spawnWanderingMonsters(CreatureID creatureID)
{
	std::vector<int3>::iterator tile;
	std::vector<int3> tiles;
	getFreeTiles(tiles);
	ui32 amount = (ui32)tiles.size() / 200; //Chance is 0.5% for each tile

	RandomGeneratorUtil::randomShuffle(tiles, getRandomGenerator());
	logGlobal->trace("Spawning wandering monsters. Found %d free tiles. Creature type: %d", tiles.size(), creatureID.num);
	const CCreature *cre = VLC->creh->objects.at(creatureID);
	for (int i = 0; i < (int)amount; ++i)
	{
		tile = tiles.begin();
		logGlobal->trace("\tSpawning monster at %s", tile->toString());
		{
			auto count = cre->getRandomAmount(std::rand);

			createObject(*tile, Obj::MONSTER, creatureID);
			auto monsterId = getTopObj(*tile)->id;

			setObjProperty(monsterId, ObjProperty::MONSTER_COUNT, count);
			setObjProperty(monsterId, ObjProperty::MONSTER_POWER, (si64)1000*count);
		}
		tiles.erase(tile); //not use it again
	}
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
	if (!strcmp(typeid(*pack).name(), typeid(PlayerMessage).name()))
		return false;

	auto query = queries->topQuery(player);
	if (query && query->blocksPack(pack))
	{
		complain(boost::str(boost::format(
			"\r\n| Player \"%s\" has to answer queries before attempting any further actions.\r\n| Top Query: \"%s\"\r\n")
			% boost::to_upper_copy<std::string>(player.getStr())
			% query->toString()
		));
		return true;
	}

	return false;
}

void CGameHandler::removeAfterVisit(const CGObjectInstance *object)
{
	//If the object is being visited, there must be a matching query
	for (const auto &query : queries->allQueries())
	{
		if (auto someVistQuery = std::dynamic_pointer_cast<CObjectVisitQuery>(query))
		{
			if (someVistQuery->visitedObject == object)
			{
				someVistQuery->removeObjectAfterVisit = true;
				return;
			}
		}
	}

	//If we haven't returned so far, there is no query and no visit, call was wrong
	assert("This function needs to be called during the object visit!");
}

void CGameHandler::changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide)
{
	std::unordered_set<int3> tiles;
	getTilesInRange(tiles, center, radius, player, hide? -1 : 1);
	if (hide)
	{
		std::unordered_set<int3> observedTiles; //do not hide tiles observed by heroes. May lead to disastrous AI problems
		auto p = getPlayerState(player);
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

void CGameHandler::changeFogOfWar(std::unordered_set<int3> &tiles, PlayerColor player, bool hide)
{
	FoWChange fow;
	fow.tiles = tiles;
	fow.player = player;
	fow.mode = hide? 0 : 1;
	sendAndApply(&fow);
}

bool CGameHandler::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	if (auto topQuery = queries->topQuery(hero->getOwner()))
		if (auto visit = std::dynamic_pointer_cast<const CObjectVisitQuery>(topQuery))
			return !(visit->visitedObject == obj && visit->visitingHero == hero);

	return true;
}

void CGameHandler::setObjProperty(ObjectInstanceID objid, int prop, si64 val)
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.val = static_cast<ui32>(val);
	sendAndApply(&sob);
}

void CGameHandler::showInfoDialog(InfoWindow * iw)
{
	sendAndApply(iw);
}

void CGameHandler::showInfoDialog(const std::string & msg, PlayerColor player)
{
	InfoWindow iw;
	iw.player = player;
	iw.text.appendRawString(msg);
	showInfoDialog(&iw);
}

CRandomGenerator & CGameHandler::getRandomGenerator()
{
	return CRandomGenerator::getDefault();
}

#if SCRIPTING_ENABLED
scripting::Pool * CGameHandler::getGlobalContextPool() const
{
	return serverScripts.get();
}

scripting::Pool * CGameHandler::getContextPool() const
{
	return serverScripts.get();
}
#endif

void CGameHandler::createObject(const int3 & visitablePosition, Obj type, int32_t subtype)
{
	NewObject no;
	no.ID = type;
	no.subID= subtype;
	no.targetPos = visitablePosition;
	sendAndApply(&no);
}

void CGameHandler::startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank, const CGTownInstance *town)
{
	battles->startBattlePrimary(army1, army2, tile, hero1, hero2, creatureBank, town);
}

void CGameHandler::startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank )
{
	battles->startBattleI(army1, army2, tile, creatureBank);
}

void CGameHandler::startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank )
{
	battles->startBattleI(army1, army2, creatureBank);
}
