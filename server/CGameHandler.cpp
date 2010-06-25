#include "../hch/CCampaignHandler.h"
#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CTownHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/map.h"
#include "../lib/VCMIDirs.h"
#include "CGameHandler.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp> //no i/o just types
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/assign/list_of.hpp>
#include <fstream>
#include <boost/system/system_error.hpp>

/*
 * CGameHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#undef DLL_EXPORT
#define DLL_EXPORT 
#include "../lib/RegisterTypes.cpp"
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

#define COMPLAIN_RET(txt) {complain(txt); return false;}
#define NEW_ROUND 		BattleNextRound bnr;\
		bnr.round = gs->curB->round + 1;\
		sendAndApply(&bnr);

CondSh<bool> battleMadeAction;
CondSh<BattleResult *> battleResult(NULL);


class CBaseForGHApply
{
public:
	virtual bool applyOnGH(CGameHandler *gh, CConnection *c, void *pack) const =0; 
};
template <typename T> class CApplyOnGH : public CBaseForGHApply
{
public:
	bool applyOnGH(CGameHandler *gh, CConnection *c, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->c = c;
		return ptr->applyGh(gh);
	}
};

class CGHApplier
{
public:
	std::map<ui16,CBaseForGHApply*> apps; 

	CGHApplier()
	{
		registerTypes3(*this);
	}
	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		apps[ID] = new CApplyOnGH<T>;
	}

} *applier = NULL;

CMP_stack cmpst ;

static inline double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}
static void giveExp(BattleResult &r)
{
	r.exp[0] = 0;
	r.exp[1] = 0;
	for(std::map<ui32,si32>::iterator i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
	{
		r.exp[r.winner] += VLC->creh->creatures[i->first]->valOfBonuses(Bonus::STACK_HEALTH) * i->second;
	}
}

PlayerStatus PlayerStatuses::operator[](ui8 player)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		return players[player];
	}
	else
	{
		throw std::string("No such player!");
	}
}
void PlayerStatuses::addPlayer(ui8 player)
{
	boost::unique_lock<boost::mutex> l(mx);
	players[player];
}
bool PlayerStatuses::hasQueries(ui8 player)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		return players[player].queries.size();
	}
	else
	{
		throw std::string("No such player!");
	}
}
bool PlayerStatuses::checkFlag(ui8 player, bool PlayerStatus::*flag)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		return players[player].*flag;
	}
	else
	{
		throw std::string("No such player!");
	}
}
void PlayerStatuses::setFlag(ui8 player, bool PlayerStatus::*flag, bool val)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		players[player].*flag = val;
	}
	else
	{
		throw std::string("No such player!");
	}
	cv.notify_all();
}
void PlayerStatuses::addQuery(ui8 player, ui32 id)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		players[player].queries.insert(id);
	}
	else
	{
		throw std::string("No such player!");
	}
	cv.notify_all();
}
void PlayerStatuses::removeQuery(ui8 player, ui32 id)
{
	boost::unique_lock<boost::mutex> l(mx);
	if(players.find(player) != players.end())
	{
		players[player].queries.erase(id);
	}
	else
	{
		throw std::string("No such player!");
	}
	cv.notify_all();
}

template <typename T>
void callWith(std::vector<T> args, boost::function<void(T)> fun, ui32 which)
{
	fun(args[which]);
}

void CGameHandler::changeSecSkill( int ID, int which, int val, bool abs/*=false*/ )
{
	SetSecSkill sss;
	sss.id = ID;
	sss.which = which;
	sss.val = val;
	sss.abs = abs;
	sendAndApply(&sss);

	if(which == 7) //Wisdom
	{
		const CGHeroInstance *h = getHero(ID);
		if(h && h->visitedTown)
			giveSpells(h->visitedTown, h);
	}
}

void CGameHandler::changePrimSkill(int ID, int which, si64 val, bool abs)
{
	SetPrimSkill sps;
	sps.id = ID;
	sps.which = which;
	sps.abs = abs;
	sps.val = val;
	sendAndApply(&sps);
	if(which==4) //only for exp - hero may level up
	{
		CGHeroInstance *hero = static_cast<CGHeroInstance *>(gs->map->objects[ID]);
		while (hero->exp >= VLC->heroh->reqExp(hero->level+1)) //new level
		{
			//give prim skill
			tlog5 << hero->name <<" got level "<<hero->level<<std::endl;
			int r = rand()%100, pom=0, x=0;
			int std::pair<int,int>::*g  =  (hero->level>9) ? (&std::pair<int,int>::second) : (&std::pair<int,int>::first);
			for(;x<PRIMARY_SKILLS;x++)
			{
				pom += hero->type->heroClass->primChance[x].*g;
				if(r<pom)
					break;
			}
			tlog5 << "Bohater dostaje umiejetnosc pierwszorzedna " << x << " (wynik losowania "<<r<<")"<<std::endl; 
			SetPrimSkill sps;
			sps.id = ID;
			sps.which = x;
			sps.abs = false;
			sps.val = 1;
			sendAndApply(&sps);

			HeroLevelUp hlu;
			hlu.heroid = ID;
			hlu.primskill = x;
			hlu.level = hero->level+1;

			//picking sec. skills for choice
			std::set<int> basicAndAdv, expert, none;
			for(int i=0;i<SKILL_QUANTITY;i++) none.insert(i);
			for(unsigned i=0;i<hero->secSkills.size();i++)
			{
				if(hero->secSkills[i].second < 3)
					basicAndAdv.insert(hero->secSkills[i].first);
				else
					expert.insert(hero->secSkills[i].first);
				none.erase(hero->secSkills[i].first);
			}

			//first offered skill
			if(hero->secSkills.size() < hero->type->heroClass->skillLimit) //free skill slot
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(none)); //new skill
				none.erase(hlu.skills.back());
			}
			else if(basicAndAdv.size())
			{
				int s = hero->type->heroClass->chooseSecSkill(basicAndAdv);
				hlu.skills.push_back(s);
				basicAndAdv.erase(s);
			}

			//second offered skill
			if(basicAndAdv.size())
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(basicAndAdv)); //new skill
			}
			else if(hero->secSkills.size() < hero->type->heroClass->skillLimit)
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(none)); //new skill
			}

			if(hlu.skills.size() > 1) //apply and ask for secondary skill
			{
				boost::function<void(ui32)> callback = boost::function<void(ui32)>(boost::bind(callWith<ui16>,hlu.skills,boost::function<void(ui16)>(boost::bind(&CGameHandler::changeSecSkill,this,ID,_1,1,0)),_1));
				applyAndAsk(&hlu,hero->tempOwner,callback); //call changeSecSkill with appropriate args when client responds
			}
			else if(hlu.skills.size() == 1) //apply, give only possible skill  and send info
			{
				sendAndApply(&hlu);
				changeSecSkill(ID,hlu.skills.back(),1,false);
			}
			else //apply and send info
			{
				sendAndApply(&hlu);
			}
		}
	}
}

static CCreatureSet takeCasualties(int color, const CCreatureSet &set, BattleInfo *bat)
{
	if(color == 254)
		color = 255;

	CCreatureSet ret(set);
	for(int i=0; i<bat->stacks.size();i++)
	{
		CStack *st = bat->stacks[i];
		if(vstd::contains(st->state, SUMMONED)) //don't take into account sumoned stacks
			continue;

		if(st->owner==color && !set.slotEmpty(st->slot) && st->count < set.getAmount(st->slot))
		{
			if(st->alive())
				ret.setStackCount(st->slot, st->count);
			else
				ret.eraseStack(st->slot);
		}
	}
	return ret;
}

void CGameHandler::startBattle(const CArmedInstance *army1, const CArmedInstance * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank, boost::function<void(BattleResult*)> cb, const CGTownInstance *town)
{
	battleEndCallback = new boost::function<void(BattleResult*)>(cb);
	bEndArmy1 = army1;
	bEndArmy2 = army2;
	{
		BattleInfo *curB = new BattleInfo;
		curB->side1 = army1->tempOwner;
		curB->side2 = army2->tempOwner;
		if(curB->side2 == 254) 
			curB->side2 = 255;
		setupBattle(curB, tile, army1, army2, hero1, hero2, creatureBank, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces
	}

	NEW_ROUND;
	//TODO: pre-tactic stuff, call scripts etc.

	//tactic round
	{
		NEW_ROUND;
		if( (hero1 && hero1->getSecSkillLevel(19)>0) || 
			( hero2 && hero2->getSecSkillLevel(19)>0)  )//someone has tactics
		{
			//TODO: tactic round (round -1)
		}
	}

	//spells opening battle
	if (hero1 && hero1->hasBonusOfType(Bonus::OPENING_BATTLE_SPELL))
	{
		BonusList bl;
		hero1->getBonuses(bl, Selector::type(Bonus::OPENING_BATTLE_SPELL));
		BOOST_FOREACH (Bonus b, bl)
		{
			handleSpellCasting(b.subtype, 3, -1, 0, hero1->tempOwner, NULL, hero2, b.val);
		}
	}
	if (hero2 && hero2->hasBonusOfType(Bonus::OPENING_BATTLE_SPELL))
	{
		BonusList bl;
		hero2->getBonuses(bl, Selector::type(Bonus::OPENING_BATTLE_SPELL));
		BOOST_FOREACH (Bonus b, bl)
		{
			handleSpellCasting(b.subtype, 3, -1, 1, hero2->tempOwner, NULL, hero1, b.val);
		}
	}

	//main loop
	while(!battleResult.get()) //till the end of the battle ;]
	{
		NEW_ROUND;
		std::vector<CStack*> & stacks = (gs->curB->stacks);
		const BattleInfo & curB = *gs->curB;

		//stack loop
		const CStack *next;
		while(!battleResult.get() && (next = curB.getNextStack()) && next->willMove())
		{

			//check for bad morale => freeze
			int nextStackMorale = next->MoraleVal();
			if( nextStackMorale < 0 &&
				!(NBonus::hasOfType(hero1, Bonus::BLOCK_MORALE) || NBonus::hasOfType(hero2, Bonus::BLOCK_MORALE)) //checking if heroes have (or don't have) morale blocking bonuses)
				)
			{
				if( rand()%24   <   -2 * nextStackMorale)
				{
					//unit loses its turn - empty freeze action
					BattleAction ba;
					ba.actionType = 11;
					ba.additionalInfo = 1;
					ba.side = !next->attackerOwned;
					ba.stackNumber = next->ID;
					sendAndApply(&StartAction(ba));
					sendAndApply(&EndAction());
					checkForBattleEnd(stacks); //check if this "action" ended the battle (not likely but who knows...)
					continue;
				}
			}

			if(next->hasBonusOfType(Bonus::ATTACKS_NEAREST_CREATURE)) //while in berserk
			{
				std::pair<const CStack *, int> attackInfo = curB.getNearestStack(next, boost::logic::indeterminate);
				if(attackInfo.first != NULL)
				{
					BattleAction attack;
					attack.actionType = 6;
					attack.side = !next->attackerOwned;
					attack.stackNumber = next->ID;

					attack.additionalInfo = attackInfo.first->position;
					attack.destinationTile = attackInfo.second;

					makeBattleAction(attack);

					checkForBattleEnd(stacks);
				}
				continue;
			}

			const CGHeroInstance * curOwner = gs->battleGetOwner(next->ID);

			if( (next->position < 0 && (!curOwner || curOwner->getSecSkillLevel(10) == 0)) //arrow turret, hero has no ballistics
				|| (next->type->idNumber == 146 && (!curOwner || curOwner->getSecSkillLevel(20) == 0))) //ballista, hero has no artillery
			{
				BattleAction attack;
				attack.actionType = 7;
				attack.side = !next->attackerOwned;
				attack.stackNumber = next->ID;

				for(int g=0; g<gs->curB->stacks.size(); ++g)
				{
					if(gs->curB->stacks[g]->owner != next->owner && gs->curB->stacks[g]->alive())
					{
						attack.destinationTile = gs->curB->stacks[g]->position;
						break;
					}
				}

				makeBattleAction(attack);

				checkForBattleEnd(stacks);
				continue;
			}

			if(next->type->idNumber == 145 && (!curOwner || curOwner->getSecSkillLevel(10) == 0)) //catapult, hero has no ballistics
			{
				BattleAction attack;
				static const int wallHexes[] = {50, 183, 182, 130, 62, 29, 12, 95};

				attack.destinationTile = wallHexes[ rand()%ARRAY_COUNT(wallHexes) ];
				attack.actionType = 9;
				attack.additionalInfo = 0;
				attack.side = !next->attackerOwned;
				attack.stackNumber = next->ID;

				makeBattleAction(attack);
				continue;
			}

			if(next->type->idNumber == 147 && (!curOwner || curOwner->getSecSkillLevel(27) == 0)) //first aid tent, hero has no first aid
			{
				BattleAction heal;

				std::vector< const CStack * > possibleStacks;
				for (int v=0; v<gs->curB->stacks.size(); ++v)
				{
					const CStack * cstack = gs->curB->stacks[v];
					if (cstack->owner == next->owner && cstack->firstHPleft < cstack->MaxHealth() && cstack->alive()) //it's friendly and not fully healthy
					{
						possibleStacks.push_back(cstack);
					}
				}

				if(possibleStacks.size() == 0)
				{
					//nothing to heal
					BattleAction doNothing;
					doNothing.actionType = 0;
					doNothing.additionalInfo = 0;
					doNothing.destinationTile = -1;
					doNothing.side = !next->attackerOwned;
					doNothing.stackNumber = next->ID;
					sendAndApply(&StartAction(doNothing));
					sendAndApply(&EndAction());
					continue;
				}
				else
				{
					//heal random creature
					const CStack * toBeHealed = possibleStacks[ rand()%possibleStacks.size() ];
					heal.actionType = 12;
					heal.additionalInfo = 0;
					heal.destinationTile = toBeHealed->position;
					heal.side = !next->attackerOwned;
					heal.stackNumber = next->ID;

					makeBattleAction(heal);

				}
				continue;
			}

askInterfaceForMove:
			//ask interface and wait for answer
			if(!battleResult.get())
			{
				BattleSetActiveStack sas;
				sas.stack = next->ID;
				sendAndApply(&sas);
				boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
				while(next->alive() && (!battleMadeAction.data  &&  !battleResult.get())) //active stack hasn't made its action and battle is still going
					battleMadeAction.cond.wait(lock);
				battleMadeAction.data = false;
			}
			else
			{
				break;
			}

			//we're after action, all results applied
			checkForBattleEnd(stacks); //check if this action ended the battle

			//check for good morale
			nextStackMorale = next->MoraleVal();
			if(!vstd::contains(next->state,HAD_MORALE)  //only one extra move per turn possible
				&& !vstd::contains(next->state,DEFENDING)
				&& !vstd::contains(next->state,WAITING)
				&&  next->alive()
				&&  nextStackMorale > 0
				&& !(NBonus::hasOfType(hero1, Bonus::BLOCK_MORALE) || NBonus::hasOfType(hero2, Bonus::BLOCK_MORALE)) //checking if heroes have (or don't have) morale blocking bonuses
			)
				if(rand()%24 < nextStackMorale) //this stack hasn't got morale this turn
					goto askInterfaceForMove; //move this stack once more
		}
	}

	endBattle(tile, hero1, hero2);

}

void CGameHandler::endBattle(int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	BattleResultsApplied resultsApplied;
	resultsApplied.player1 = bEndArmy1->tempOwner;
	resultsApplied.player2 = bEndArmy2->tempOwner;

	//unblock engaged players
	if(bEndArmy1->tempOwner<PLAYER_LIMIT)
		states.setFlag(bEndArmy1->tempOwner, &PlayerStatus::engagedIntoBattle, false);
	if(bEndArmy2 && bEndArmy2->tempOwner<PLAYER_LIMIT)
		states.setFlag(bEndArmy2->tempOwner, &PlayerStatus::engagedIntoBattle, false);	

	//casualties among heroes armies
	SetGarrisons sg;
	sg.garrs[bEndArmy1->id] = takeCasualties(bEndArmy1->tempOwner, *bEndArmy1, gs->curB);
	sg.garrs[bEndArmy2->id] = takeCasualties(bEndArmy2->tempOwner, *bEndArmy2, gs->curB);
	sendAndApply(&sg);

	ui8 sides[2];
	sides[0] = gs->curB->side1;
	sides[1] = gs->curB->side2;

	//end battle, remove all info, free memory
	giveExp(*battleResult.data);
	sendAndApply(battleResult.data);

	//if one hero has lost we will erase him
	if(battleResult.data->winner!=0 && hero1)
	{
		RemoveObject ro(hero1->id);
		sendAndApply(&ro);
	}
	if(battleResult.data->winner!=1 && hero2)
	{
		RemoveObject ro(hero2->id);
		sendAndApply(&ro);
	}

	//give exp
	if(battleResult.data->exp[0] && hero1)
		changePrimSkill(hero1->id,4,battleResult.data->exp[0]);
	if(battleResult.data->exp[1] && hero2)
		changePrimSkill(hero2->id,4,battleResult.data->exp[1]);

	if(battleEndCallback && *battleEndCallback)
	{
		(*battleEndCallback)(battleResult.data);
		delete battleEndCallback;
		battleEndCallback = 0;
	}

	sendAndApply(&resultsApplied);

	// Necromancy if applicable.
	const CGHeroInstance *winnerHero = battleResult.data->winner != 0 ? hero2 : hero1;
	if (winnerHero) 
	{
		CStackInstance raisedStack = winnerHero->calculateNecromancy(*battleResult.data);

		// Give raised units to winner and show dialog, if any were raised.
		if (raisedStack.type) 
		{
			int slot = winnerHero->getSlotFor(raisedStack.type->idNumber);

			if (slot != -1) 
			{
				SetGarrisons sg;
				sg.garrs[winnerHero->id] = winnerHero->getArmy();
				sg.garrs[winnerHero->id].addToSlot(slot, raisedStack);

// 				if (vstd::contains(winnerHero->slots, slot)) // Add to existing stack.
// 					sg.garrs[winnerHero->id].slots[slot].count += raisedStack.count;
// 				else // Create a new stack.
// 					sg.garrs[winnerHero->id].slots[slot] = raisedStack;
				winnerHero->showNecromancyDialog(raisedStack);
				sendAndApply(&sg);
			}
		}
	}

	if(visitObjectAfterVictory && winnerHero == hero1)
	{
		visitObjectOnTile(*getTile(winnerHero->getPosition()), winnerHero);
	}
	visitObjectAfterVictory = false; 

	winLoseHandle(1<<sides[0] | 1<<sides[1]); //handle victory/loss of engaged players
	delete battleResult.data;
}

void CGameHandler::prepareAttacked(BattleStackAttacked &bsa, const CStack *def)
{	
	bsa.killedAmount = bsa.damageAmount / def->MaxHealth();
	unsigned damageFirst = bsa.damageAmount % def->MaxHealth();

	if( def->firstHPleft <= damageFirst )
	{
		bsa.killedAmount++;
		bsa.newHP = def->firstHPleft + def->MaxHealth() - damageFirst;
	}
	else
	{
		bsa.newHP = def->firstHPleft - damageFirst;
	}

	if(def->count <= bsa.killedAmount) //stack killed
	{
		bsa.newAmount = 0;
		bsa.flags |= 1;
		bsa.killedAmount = def->count; //we cannot kill more creatures than we have
	}
	else
	{
		bsa.newAmount = def->count - bsa.killedAmount;
	}
}

void CGameHandler::prepareAttack(BattleAttack &bat, const CStack *att, const CStack *def, int distance)
{
	bat.bsa.clear();
	bat.stackAttacking = att->ID;
	bat.bsa.push_back(BattleStackAttacked());
	BattleStackAttacked *bsa = &bat.bsa.back();
	bsa->stackAttacked = def->ID;
	bsa->attackerID = att->ID;
	int attackerLuck = att->LuckVal();
	const CGHeroInstance * h0 = gs->curB->heroes[0],
		* h1 = gs->curB->heroes[1];
	bool noLuck = false;
	if(h0 && NBonus::hasOfType(h0, Bonus::BLOCK_LUCK) ||
		h1 && NBonus::hasOfType(h1, Bonus::BLOCK_LUCK))
	{
		noLuck = true;
	}

	if(!noLuck && attackerLuck > 0  &&  rand()%24 < attackerLuck) //TODO?: negative luck option?
	{
		bsa->damageAmount *= 2;
		bat.flags |= 4;
	}

	bsa->damageAmount = gs->curB->calculateDmg(att, def, gs->battleGetOwner(att->ID), gs->battleGetOwner(def->ID), bat.shot(), distance, bat.lucky());//counting dealt damage
	
	
	int dmg = bsa->damageAmount;
	prepareAttacked(*bsa, def);

	//life drain handling
	if (att->hasBonusOfType(Bonus::LIFE_DRAIN))
	{
		StacksHealedOrResurrected shi;
		shi.lifeDrain = true;
		shi.drainedFrom = def->ID;

		StacksHealedOrResurrected::HealInfo hi;
		hi.stackID = att->ID;
		hi.healedHP = std::min<int>(dmg, att->MaxHealth() - att->firstHPleft + att->MaxHealth() * (att->baseAmount - att->count) );
		hi.lowLevelResurrection = false;
		shi.healedStacks.push_back(hi);

		if (hi.healedHP > 0)
		{
			bsa->healedStacks.push_back(shi);
		}
	} 
	else
	{
	}

	//fire shield handling
	if ( !bat.shot() && def->hasBonusOfType(Bonus::FIRE_SHIELD) && !bsa->killed() )
	{
		bat.bsa.push_back(BattleStackAttacked());
		BattleStackAttacked *bsa = &bat.bsa.back();
		bsa->stackAttacked = att->ID;
		bsa->attackerID = def->ID;
		bsa->flags |= 2;
		bsa->effect = 11;

		bsa->damageAmount = (dmg * def->valOfBonuses(Bonus::FIRE_SHIELD)) / 100;
		prepareAttacked(*bsa, att);
	}

}
void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	srand(time(NULL));
	CPack *pack = NULL;
	try
	{
		while(1)//server should never shut connection first //was: while(!end2)
		{
			{
				boost::unique_lock<boost::mutex> lock(*c.rmx);
				c >> pack; //get the package
				tlog5 << "Received client message of type " << typeid(*pack).name() << std::endl;
			}

			int packType = typeList.getTypeID(pack); //get the id of type
			CBaseForGHApply *apply = applier->apps[packType]; //and appropriae applier object

			if(apply)
			{
				bool result = apply->applyOnGH(this,&c,pack);
				tlog5 << "Message successfully applied (result=" << result << ")!\n";

				//send confirmation that we've applied the package
				if(pack->type != 6000) //WORKAROUND - not confirm query replies TODO: reconsider
				{
					PackageApplied applied;
					applied.result = result;
					applied.packType = packType;
					{
						boost::unique_lock<boost::mutex> lock(*c.wmx);
						c << &applied;
					}
				}
			}
			else
			{
				tlog1 << "Message cannot be applied, cannot find applier (unregistered type)!\n";
			}
			delete pack;
			pack = NULL;
		}
	}
	catch(boost::system::system_error &e) //for boost errors just log, not crash - probably client shut down connection
	{
		assert(!c.connected); //make sure that connection has been marked as broken
		tlog1 << e.what() << std::endl;
		end2 = true;
	}
	HANDLE_EXCEPTION(end2 = true);

	tlog1 << "Ended handling connection\n";
}

int CGameHandler::moveStack(int stack, int dest)
{
	int ret = 0;

	CStack *curStack = gs->curB->getStack(stack),
		*stackAtEnd = gs->curB->getStackT(dest);

	assert(curStack);
	assert(dest < BFIELD_SIZE);

	//initing necessary tables
	bool accessibility[BFIELD_SIZE];
	std::vector<int> accessible = gs->curB->getAccessibility(curStack->ID, false);
	for(int b=0; b<BFIELD_SIZE; ++b)
	{
		accessibility[b] = false;
	}
	for(int g=0; g<accessible.size(); ++g)
	{
		accessibility[accessible[g]] = true;
	}

	//shifting destination (if we have double wide stack and we can occupy dest but not be exactly there)
	if(!stackAtEnd && curStack->doubleWide() && !accessibility[dest])
	{
		if(curStack->attackerOwned)
		{
			if(accessibility[dest+1])
				dest+=1;
		}
		else
		{
			if(accessibility[dest-1])
				dest-=1;
		}
	}

	if((stackAtEnd && stackAtEnd!=curStack && stackAtEnd->alive()) || !accessibility[dest])
		return 0;

	bool accessibilityWithOccupyable[BFIELD_SIZE];
	std::vector<int> accOc = gs->curB->getAccessibility(curStack->ID, true);
	for(int b=0; b<BFIELD_SIZE; ++b)
	{
		accessibilityWithOccupyable[b] = false;
	}
	for(int g=0; g<accOc.size(); ++g)
	{
		accessibilityWithOccupyable[accOc[g]] = true;
	}

	//if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
	//	return false;

	std::pair< std::vector<int>, int > path = gs->curB->getPath(curStack->position, dest, accessibilityWithOccupyable, curStack->hasBonusOfType(Bonus::FLYING), curStack->doubleWide(), curStack->attackerOwned);

	ret = path.second;

	if(curStack->hasBonusOfType(Bonus::FLYING))
	{
		if(path.second <= curStack->Speed() && path.first.size() > 0)
		{
			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			sm.tile = path.first[0];
			sm.distance = path.second;
			sm.ending = true;
			sm.teleporting = false;
			sendAndApply(&sm);
		}
	}
	else //for non-flying creatures
	{
		int tilesToMove = std::max((int)(path.first.size() - curStack->Speed()), 0);
		for(int v=path.first.size()-1; v>=tilesToMove; --v)
		{
			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			sm.tile = path.first[v];
			sm.distance = path.second;
			sm.ending = v==tilesToMove;
			sm.teleporting = false;
			sendAndApply(&sm);
		}
	}

	return ret;
}
CGameHandler::CGameHandler(void)
{
	QID = 1;
	gs = NULL;
	IObjectInterface::cb = this;
	applier = new CGHApplier;
	visitObjectAfterVictory = false; 
}

CGameHandler::~CGameHandler(void)
{
	delete applier;
	applier = NULL;
	delete gs;
}

void CGameHandler::init(StartInfo *si, int Seed)
{
	gs = new CGameState();
	tlog0 << "Gamestate created!" << std::endl;
	gs->init(si, 0, Seed);
	tlog0 << "Gamestate initialized!" << std::endl;

	for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		states.addPlayer(i->first);
}

static bool evntCmp(const CMapEvent *a, const CMapEvent *b)
{
	return *a < *b;
}

void CGameHandler::newTurn()
{
	tlog5 << "Turn " << gs->day+1 << std::endl;
	NewTurn n;
	n.day = gs->day + 1;
	n.resetBuilded = true;
	
	std::map<ui8, si32> hadGold;//starting gold - for buildings like dwarven treasury
	srand(time(NULL));

	std::map<ui32,CGHeroInstance *> pool = gs->hpool.heroesPool;

	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->first == 255) continue;
		else if(i->first > PLAYER_LIMIT) assert(0); //illegal player number!

		std::pair<ui8,si32> playerGold(i->first,i->second.resources[6]);
		hadGold.insert(playerGold); 

		if(gs->getDate(1)==7) //first day of week - new heroes in tavern
		{
			SetAvailableHeroes sah;
			sah.player = i->first;
			CGHeroInstance *h = gs->hpool.pickHeroFor(true,i->first,&VLC->townh->towns[gs->scenarioOps->getIthPlayersSettings(i->first).castle], pool);
			if(h)
				sah.hid1 = h->subID;
			else
				sah.hid1 = -1;
			h = gs->hpool.pickHeroFor(false,i->first,&VLC->townh->towns[gs->scenarioOps->getIthPlayersSettings(i->first).castle], pool);
			if(h)
				sah.hid2 = h->subID;
			else
				sah.hid2 = -1;
			sendAndApply(&sah);
		}
		if(i->first>=PLAYER_LIMIT) continue;

		n.res[i->first] = i->second.resources;
// 		SetResources r;
// 		r.player = i->first;
// 		for(int j=0;j<RESOURCE_QUANTITY;j++)
// 			r.res[j] = i->second.resources[j];
		
		BOOST_FOREACH(CGHeroInstance *h, (*i).second.heroes)
		{
			if(h->visitedTown)
				giveSpells(h->visitedTown, h);

			NewTurn::Hero hth;
			hth.id = h->id;
			hth.move = h->maxMovePoints(gs->map->getTile(h->getPosition(false)).tertype != TerrainTile::water);

			if(h->visitedTown && vstd::contains(h->visitedTown->builtBuildings,0)) //if hero starts turn in town with mage guild
				hth.mana = std::max(h->mana, h->manaLimit()); //restore all mana
			else
				hth.mana = std::max(si32(0), std::max(h->mana, std::min(h->mana + h->manaRegain(), h->manaLimit())) ); 

			n.heroes.insert(hth);
			
			if(gs->day) //not first day
			{
				switch(h->getSecSkillLevel(13)) //handle estates - give gold
				{
				case 1: //basic
					n.res[i->first][6] += 125;
					break;
				case 2: //advanced
					n.res[i->first][6] += 250;
					break;
				case 3: //expert
					n.res[i->first][6] += 500;
					break;
				}

				for(std::list<Bonus>::iterator  j = h->bonuses.begin(); j != h->bonuses.end(); j++)
					if(j->type == Bonus::GENERATE_RESOURCE)
						n.res[i->first][j->subtype] += j->val;

				//TODO player bonuses

			}
		}
		//n.res.push_back(r);
	}
	for(std::vector<CGTownInstance *>::iterator j = gs->map->towns.begin(); j!=gs->map->towns.end(); j++)//handle towns
	{
		ui8 player = (*j)->tempOwner;
		if(gs->getDate(1)==7) //first day of week
		{
			if  ( (**j).subID == 1 && gs->getDate(0) && player < PLAYER_LIMIT
				&& vstd::contains((**j).builtBuildings,22) )//dwarven treasury
					n.res[player][6] += hadGold[player]/10; //give 10% of starting gold
			SetAvailableCreatures sac;
			sac.tid = (**j).id;
			sac.creatures = (**j).creatures;
			for(int k=0;k<CREATURES_PER_TOWN;k++) //creature growths
			{
				if((**j).creatureDwelling(k))//there is dwelling (k-level)
				{
					sac.creatures[k].first += (**j).creatureGrowth(k);
					if(!gs->getDate(0)) //first day of game: use only basic growths
						amin(sac.creatures[k].first, VLC->creh->creatures[(*j)->town->basicCreatures[k]]->growth);
				}
			}
			n.cres.push_back(sac);
		}
		if(gs->day  &&  player < PLAYER_LIMIT)//not the first day and town not neutral
		{
			////SetResources r;
			//r.player = (**j).tempOwner;
			if(vstd::contains((**j).builtBuildings,15)) //there is resource silo
			{
				if((**j).town->primaryRes == 127) //we'll give wood and ore
				{
					n.res[player][0] += 1;
					n.res[player][2] += 1;
				}
				else
				{
					n.res[player][(**j).town->primaryRes] += 1;
				}
			}
			n.res[player][6] += (**j).dailyIncome();
		}
		if ((**j).subID == 2 && vstd::contains((**j).builtBuildings, 26)) // Skyship, probably easier to handle same as Veil of darkness
		{ //do it every new day after veils apply
			FoWChange fw;
			fw.mode = 1;
			fw.player = player;
			getAllTiles(fw.tiles, player, -1, 0);
			sendAndApply (&fw);
		}
		if ((**j).hasBonusOfType (Bonus::DARKNESS))
		{
			(**j).hideTiles((**j).getOwner(), (**j).getBonus(Selector::type(Bonus::DARKNESS))->val);
		}
		//unhiding what shouldn't be hidden? //that's handled in netpacks client
	}

	sendAndApply(&n);
	tlog5 << "Info about turn " << n.day << "has been sent!" << std::endl;
	handleTimeEvents();
	//call objects
	for(size_t i = 0; i<gs->map->objects.size(); i++)
		if(gs->map->objects[i])
			gs->map->objects[i]->newTurn();

	winLoseHandle(0xff);

	//warn players without town
	if(gs->day)
	{
		for (std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
		{
			if(i->second.status || i->second.towns.size() || i->second.color >= PLAYER_LIMIT)
				continue;

			InfoWindow iw;
			iw.player = i->first;
			iw.components.push_back(Component(Component::FLAG,i->first,0,0));

			if(!i->second.daysWithoutCastle)
			{
				iw.text.addTxt(MetaString::GENERAL_TXT,6); //%s, you have lost your last town.  If you do not conquer another town in the next week, you will be eliminated.
				iw.text.addReplacement(MetaString::COLOR, i->first);
			}
			else if(i->second.daysWithoutCastle == 6)
			{
				iw.text.addTxt(MetaString::ARRAY_TXT,129); //%s, this is your last day to capture a town or you will be banished from this land.
				iw.text.addReplacement(MetaString::COLOR, i->first);
			}
			else
			{
				iw.text.addTxt(MetaString::ARRAY_TXT,128); //%s, you only have %d days left to capture a town or you will be banished from this land.
				iw.text.addReplacement(MetaString::COLOR, i->first);
				iw.text.addReplacement(7 - i->second.daysWithoutCastle);
			}
			sendAndApply(&iw);
		}
	}
}
void CGameHandler::run(bool resume, const StartInfo *si /*= NULL*/)
{
	using namespace boost::posix_time;
	BOOST_FOREACH(CConnection *cc, conns)
	{//init conn.
		ui8 quantity, pom;
		//ui32 seed;
		if(!resume)
			(*cc) << si << gs->map->checksum << gs->seed; // gs->scenarioOps

		(*cc) >> quantity; //how many players will be handled at that client
		for(int i=0;i<quantity;i++)
		{
			(*cc) >> pom; //read player color
			{
				boost::unique_lock<boost::recursive_mutex> lock(gsm);
				connections[pom] = cc;
			}
		}	
	}
	
	for(std::set<CConnection*>::iterator i = conns.begin(); i!=conns.end();i++)
	{
		std::set<int> pom;
		for(std::map<int,CConnection*>::iterator j = connections.begin(); j!=connections.end();j++)
			if(j->second == *i)
				pom.insert(j->first);

		boost::thread(boost::bind(&CGameHandler::handleConnection,this,pom,boost::ref(**i)));
	}

	while (!end2)
	{
		if(!resume)
			newTurn();

		std::map<ui8,PlayerState>::iterator i;
		if(!resume)
			i = gs->players.begin();
		else
			i = gs->players.find(gs->currentPlayer);

		resume = false;
		for(; i != gs->players.end(); i++)
		{
			if(i->second.towns.size()==0 && i->second.heroes.size()==0
				|| i->second.color<0 
				|| i->first>=PLAYER_LIMIT  
				|| i->second.status) 
			{
				continue; 
			}
			states.setFlag(i->first,&PlayerStatus::makingTurn,true);

			{
				YourTurn yt;
				yt.player = i->first;
				sendAndApply(&yt);
			}

			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(states.mx);
			while(states.players[i->first].makingTurn && !end2)
			{
				static time_duration p = milliseconds(200);
				states.cv.timed_wait(lock,p); 
			}
		}
	}

	while(conns.size() && (*conns.begin())->isOpen())
		boost::this_thread::sleep(boost::posix_time::milliseconds(5)); //give time client to close socket
}

namespace CGH
{
	using namespace std;
	static void readItTo(ifstream & input, vector< vector<int> > & dest) //reads 7 lines, i-th one containing i integers, and puts it to dest
	{
		for(int j=0; j<7; ++j)
		{
			std::vector<int> pom;
			for(int g=0; g<j+1; ++g)
			{
				int hlp; input>>hlp;
				pom.push_back(hlp);
			}
			dest.push_back(pom);
		}
	}
}

void CGameHandler::setupBattle(BattleInfo * curB, int3 tile, const CArmedInstance *army1, const CArmedInstance *army2, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool creatureBank, const CGTownInstance *town)
{
	battleResult.set(NULL);
	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->belligerents[0] = const_cast<CArmedInstance*>(army1);
	curB->belligerents[1] = const_cast<CArmedInstance*>(army2);
	curB->heroes[0] = const_cast<CGHeroInstance*>(hero1);
	curB->heroes[1] = const_cast<CGHeroInstance*>(hero2);
	curB->round = -2;
	curB->activeStack = -1;

	if(town)
	{
		curB->tid = town->id;
		curB->siege = town->fortLevel();
	}
	else
	{
		curB->tid = -1;
		curB->siege = 0;
	}

	//reading battleStartpos
	std::ifstream positions;
	positions.open(DATA_DIR "/config/battleStartpos.txt", std::ios_base::in|std::ios_base::binary);
	if(!positions.is_open())
	{
		tlog1<<"Unable to open battleStartpos.txt!"<<std::endl;
	}
	std::string dump;
	positions>>dump; positions>>dump;
	std::vector< std::vector<int> > attackerLoose, defenderLoose, attackerTight, defenderTight, attackerCreBank, defenderCreBank;
	CGH::readItTo(positions, attackerLoose);
	positions>>dump;
	CGH::readItTo(positions, defenderLoose);
	positions>>dump;
	positions>>dump;
	CGH::readItTo(positions, attackerTight);
	positions>>dump;
	CGH::readItTo(positions, defenderTight);
	positions>>dump;
	positions>>dump;
	CGH::readItTo(positions, attackerCreBank);
	positions>>dump;
	CGH::readItTo(positions, defenderCreBank);
	positions.close();
	//battleStartpos read

	int k = 0; //stack serial 
	for(TSlots::const_iterator i = army1->Slots().begin(); i!=army1->Slots().end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = attackerCreBank[army1->stacksCount()-1][k];
		else if(army1->formation)
			pos = attackerTight[army1->stacksCount()-1][k];
		else
			pos = attackerLoose[army1->stacksCount()-1][k];

		CStack * stack = curB->generateNewStack(i->second, stacks.size(), true, i->first, gs->map->terrain[tile.x][tile.y][tile.z].tertype, pos);
		stacks.push_back(stack);
	}
	
	k = 0;
	for(TSlots::const_iterator i = army2->Slots().begin(); i!=army2->Slots().end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = defenderCreBank[army2->stacksCount()-1][k];
		else if(army2->formation)
			pos = defenderTight[army2->stacksCount()-1][k];
		else
			pos = defenderLoose[army2->stacksCount()-1][k];

		CStack * stack = curB->generateNewStack(i->second, stacks.size(), false, i->first, gs->map->terrain[tile.x][tile.y][tile.z].tertype, pos);
		stacks.push_back(stack);
	}

	for(unsigned g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		if((stacks[g]->position%17)==1 && stacks[g]->doubleWide() && stacks[g]->attackerOwned)
		{
			stacks[g]->position += 1;
		}
		else if((stacks[g]->position%17)==15 && stacks[g]->doubleWide() && !stacks[g]->attackerOwned)
		{
			stacks[g]->position -= 1;
		}
	}

	//adding war machines
	if(hero1)
	{
		if(hero1->getArt(13)) //ballista
		{
			CStack * stack = curB->generateNewStack(CStackInstance(146, 1, hero1), stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 52);
			stacks.push_back(stack);
		}
		if(hero1->getArt(14)) //ammo cart
		{
			CStack * stack = curB->generateNewStack(CStackInstance(148, 1, hero1), stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 18);
			stacks.push_back(stack);
		}
		if(hero1->getArt(15)) //first aid tent
		{
			CStack * stack = curB->generateNewStack(CStackInstance(147, 1, hero1), stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 154);
			stacks.push_back(stack);
		}
	}
	if(hero2)
	{
		if(hero2->getArt(13)) //ballista
		{
			CStack * stack = curB->generateNewStack(CStackInstance(146, 1, hero2),  stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 66);
			stacks.push_back(stack);
		}
		if(hero2->getArt(14)) //ammo cart
		{
			CStack * stack = curB->generateNewStack(CStackInstance(148, 1, hero1), stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 32);
			stacks.push_back(stack);
		}
		if(hero2->getArt(15)) //first aid tent
		{
			CStack * stack = curB->generateNewStack(CStackInstance(147, 1, hero2), stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 168);
			stacks.push_back(stack);
		}
	}
	if(town && hero1 && town->hasFort()) //catapult
	{
		CStack * stack = curB->generateNewStack(CStackInstance(145, 1, hero1), stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 120);
		stacks.push_back(stack);
	}
	//war machines added

	switch(curB->siege) //adding towers
	{
		
	case 3: //castle
		{//lower tower / upper tower
			CStack * stack = curB->generateNewStack(CStackInstance(149, 1, hero2), stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -4);
			stacks.push_back(stack);
			stack = curB->generateNewStack(CStackInstance(149, 1, hero2), stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -3);
			stacks.push_back(stack);
		}
	case 2: //citadel
		{//main tower
			CStack * stack = curB->generateNewStack(CStackInstance(149, 1, hero2), stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -2);
			stacks.push_back(stack);
		}
	}

	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//seting up siege
	if(town && town->hasFort())
	{
		for(int b=0; b<ARRAY_COUNT(curB->si.wallState); ++b)
		{
			curB->si.wallState[b] = 1;
		}
	}
	
	int terType = gs->battleGetBattlefieldType(tile);

	//randomize obstacles
	if(town == NULL && !creatureBank) //do it only when it's not siege and not creature bank
	{
		bool obAv[BFIELD_SIZE]; //availability of hexes for obstacles;
		std::vector<int> possibleObstacles;

		for(int i=0; i<BFIELD_SIZE; ++i)
		{
			if(i%17 < 4 || i%17 > 12)
			{
				obAv[i] = false;
			}
			else
			{
				obAv[i] = true;
			}
		}

		for(std::map<int, CObstacleInfo>::const_iterator g=VLC->heroh->obstacles.begin(); g!=VLC->heroh->obstacles.end(); ++g)
		{
			if(g->second.allowedTerrains[terType-1] == '1') //we need to take terType with -1 because terrain ids start from 1 and allowedTerrains array is indexed from 0
			{
				possibleObstacles.push_back(g->first);
			}
		}

		srand(time(NULL));
		if(possibleObstacles.size() > 0) //we cannot place any obstacles when we don't have them
		{
			int toBlock = rand()%6 + 6; //how many hexes should be blocked by obstacles
			while(toBlock>0)
			{
				CObstacleInstance coi;
				coi.uniqueID = curB->obstacles.size();
				coi.ID = possibleObstacles[rand()%possibleObstacles.size()];
				coi.pos = rand()%BFIELD_SIZE;
				std::vector<int> block = VLC->heroh->obstacles[coi.ID].getBlocked(coi.pos);
				bool badObstacle = false;
				for(int b=0; b<block.size(); ++b)
				{
					if(block[b] < 0 || block[b] >= BFIELD_SIZE || !obAv[block[b]])
					{
						badObstacle = true;
						break;
					}
				}
				if(badObstacle) continue;
				//obstacle can be placed
				curB->obstacles.push_back(coi);
				for(int b=0; b<block.size(); ++b)
				{
					if(block[b] >= 0 && block[b] < BFIELD_SIZE)
						obAv[block[b]] = false;
				}
				toBlock -= block.size();
			}
		}
	}

	//giving building bonuses, if siege and we have harrisoned hero
	if (town)
	{
		if (hero2)
		{
			for (int i=0; i<4; i++)
			{
				int val = town->defenceBonus(i);
				if (val)
				{
					GiveBonus gs;
					gs.bonus = Bonus(Bonus::ONE_BATTLE, Bonus::PRIMARY_SKILL, Bonus::OBJECT, val, -1, "", i);
					gs.id = hero2->id;
					sendAndApply(&gs);
				}
			}
		}
		else//if we don't have hero - apply separately, if hero present - will be taken from hero bonuses
		{
			if(town->subID == 0  &&  vstd::contains(town->builtBuildings,22)) //castle, brotherhood of sword built
				for(int g=0; g<stacks.size(); ++g)
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, 2, Bonus::TOWN_STRUCTURE));

			else if(vstd::contains(town->builtBuildings,5)) //tavern is built
				for(int g=0; g<stacks.size(); ++g)
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, 1, Bonus::TOWN_STRUCTURE));

			if(town->subID == 1  &&  vstd::contains(town->builtBuildings,21)) //rampart, fountain of fortune is present
				for(int g=0; g<stacks.size(); ++g)
					stacks[g]->bonuses.push_back(makeFeature(Bonus::LUCK, Bonus::ONE_BATTLE, 0, 2, Bonus::TOWN_STRUCTURE));
		}
	}

	//giving terrain premies for heroes & stacks

	int bonusSubtype = -1;
	switch(terType)
	{
	case 9: //magic plains
		{
			bonusSubtype = 0;
		}
	case 14: //fiery fields
		{
			if(bonusSubtype == -1) bonusSubtype = 1;
		}
	case 15: //rock lands
		{
			if(bonusSubtype == -1) bonusSubtype = 8;
		}
	case 16: //magic clouds
		{
			if(bonusSubtype == -1) bonusSubtype = 2;
		}
	case 17: //lucid pools
		{
			if(bonusSubtype == -1) bonusSubtype = 4;
		}

		{ //common part for cases 9, 14, 15, 16, 17
			const CGHeroInstance * cHero = NULL;
			for(int i=0; i<2; ++i)
			{
				if(i == 0) cHero = hero1;
				else cHero = hero2;

				if(cHero == NULL) continue;

				GiveBonus gs;
				gs.bonus = Bonus(Bonus::ONE_BATTLE, Bonus::MAGIC_SCHOOL_SKILL, Bonus::OBJECT, 3, -1, "", bonusSubtype);
				gs.id = cHero->id;

				sendAndApply(&gs);
			}

			break;
		}

	case 18: //holy ground
		{
			for(int g=0; g<stacks.size(); ++g) //+1 morale bonus for good creatures, -1 morale bonus for evil creatures
			{
				if (stacks[g]->type->isGood())
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, 1, Bonus::TERRAIN_OVERLAY));
				else if (stacks[g]->type->isEvil())
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY));
			}
			break;
		}
	case 19: //clover field
		{
			for(int g=0; g<stacks.size(); ++g)
			{
				if(stacks[g]->type->faction == -1) //+2 luck bonus for neutral creatures
				{
					stacks[g]->bonuses.push_back(makeFeature(Bonus::LUCK, Bonus::ONE_BATTLE, 0, 2, Bonus::TERRAIN_OVERLAY));
				}
			}
			break;
		}
	case 20: //evil fog
		{
			for(int g=0; g<stacks.size(); ++g) //-1 morale bonus for good creatures, +1 morale bonus for evil creatures
			{
				if (stacks[g]->type->isGood())
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY));
				else if (stacks[g]->type->isEvil())
					stacks[g]->bonuses.push_back(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, 1, Bonus::TERRAIN_OVERLAY));
			}
			break;
		}
	case 22: //cursed ground
		{
			for(int g=0; g<stacks.size(); ++g) //no luck nor morale
			{
				stacks[g]->bonuses.push_back(makeFeature(Bonus::NO_MORALE, Bonus::ONE_BATTLE, 0, 0, Bonus::TERRAIN_OVERLAY));
				stacks[g]->bonuses.push_back(makeFeature(Bonus::NO_LUCK, Bonus::ONE_BATTLE, 0, 0, Bonus::TERRAIN_OVERLAY));
			}

			const CGHeroInstance * cHero = NULL;
			for(int i=0; i<2; ++i) //blocking spells above level 1
			{
				if(i == 0) cHero = hero1;
				else cHero = hero2;

				if(cHero == NULL) continue;

				GiveBonus gs;
				gs.bonus = Bonus(Bonus::ONE_BATTLE, Bonus::BLOCK_SPELLS_ABOVE_LEVEL, Bonus::OBJECT, 1, -1, "", bonusSubtype);
				gs.id = cHero->id;

				sendAndApply(&gs);
			}

			break;
		}
	}

	//premies given


	//send info about battles
	BattleStart bs;
	bs.info = curB;
	sendAndApply(&bs);
}

void CGameHandler::checkForBattleEnd( std::vector<CStack*> &stacks )
{
	//checking winning condition
	bool hasStack[2]; //hasStack[0] - true if attacker has a living stack; defender similarily
	hasStack[0] = hasStack[1] = false;
	for(int b = 0; b<stacks.size(); ++b)
	{
		if(stacks[b]->alive() && !stacks[b]->hasBonusOfType(Bonus::SIEGE_WEAPON))
		{
			hasStack[1-stacks[b]->attackerOwned] = true;
		}
	}
	if(!hasStack[0] || !hasStack[1]) //somebody has won
	{
		BattleResult *br = new BattleResult; //will be deleted at the end of startBattle(...)
		br->result = 0;
		br->winner = hasStack[1]; //fleeing side loses
		gs->curB->calculateCasualties(br->casualties);
		battleResult.set(br);
	}
}

void CGameHandler::giveSpells( const CGTownInstance *t, const CGHeroInstance *h )
{
	if(!vstd::contains(h->artifWorn,17))
		return; //hero hasn't spellbok
	ChangeSpells cs;
	cs.hid = h->id;
	cs.learn = true;
	for(int i=0; i<std::min(t->mageGuildLevel(),h->getSecSkillLevel(7)+2);i++)
	{
		if (t->subID == 8 && vstd::contains(t->builtBuildings, 26)) //Aurora Borealis
		{
			std::vector<ui16> spells;
			getAllowedSpells(spells, i);
			for (int j = 0; j < spells.size(); ++j)
				cs.spells.insert(spells[j]);
		}
		else
		{
			for(int j=0; j<t->spellsAtLevel(i+1,true) && j<t->spells[i].size(); j++)
			{
				if(!vstd::contains(h->spells,t->spells[i][j]))
					cs.spells.insert(t->spells[i][j]);
			}
		}
	}
	if(cs.spells.size())
		sendAndApply(&cs);
}

void CGameHandler::setBlockVis(int objid, bool bv)
{
	SetObjectProperty sop(objid,2,bv);
	sendAndApply(&sop);
}

bool CGameHandler::removeObject( int objid )
{
	if(!getObj(objid))
	{
		tlog1 << "Something wrong, that object already has been removed or hasn't existed!\n";
		return false;
	}

	RemoveObject ro;
	ro.id = objid;
	sendAndApply(&ro);

	winLoseHandle(255); //eg if monster escaped (removing objs after battle is done dircetly by endBattle, not this function)
	return true;
}

void CGameHandler::setAmount(int objid, ui32 val)
{
	SetObjectProperty sop(objid,3,val);
	sendAndApply(&sop);
}

bool CGameHandler::moveHero( si32 hid, int3 dst, ui8 instant, ui8 asker /*= 255*/ )
{
	bool blockvis = false;
	const CGHeroInstance *h = getHero(hid);

	if(!h  ||  asker != 255 && (instant  ||   h->getOwner() != gs->currentPlayer) //not turn of that hero or player can't simply teleport hero (at least not with this function)
	  )
	{
		tlog1 << "Illegal call to move hero!\n";
		return false;
	}


	tlog5 << "Player " <<int(asker) << " wants to move hero "<< hid << " from "<< h->pos << " to " << dst << std::endl;
	int3 hmpos = dst + int3(-1,0,0);

	if(!gs->map->isInTheMap(hmpos))
	{
		tlog1 << "Destination tile is outside the map!\n";
		return false;
	}

	TerrainTile t = gs->map->terrain[hmpos.x][hmpos.y][hmpos.z];
	int cost = gs->getMovementCost(h, h->getPosition(false), CGHeroInstance::convertPosition(dst,false),h->movement);
	int3 guardPos = gs->guardingCreaturePosition(hmpos);

	//result structure for start - movement failed, no move points used
	TryMoveHero tmh;
	tmh.id = hid;
	tmh.start = h->pos;
	tmh.end = dst;
	tmh.result = TryMoveHero::FAILED;
	tmh.movePoints = h->movement;

	//check if destination tile is available

	//it's a rock or blocked and not visitable tile 
	//OR hero is on land and dest is water and (there is not present only one object - boat)
	if((t.tertype == TerrainTile::rock  ||  (t.blocked && !t.visitable && !h->hasBonusOfType(Bonus::FLYING_MOVEMENT) )) 
			&& complain("Cannot move hero, destination tile is blocked!") 
		|| (!h->boat && !h->canWalkOnSea() && t.tertype == TerrainTile::water && (t.visitableObjects.size() != 1 ||  (t.visitableObjects.front()->ID != 8 && t.visitableObjects.front()->ID != HEROI_TYPE)))  //hero is not on boat/water walking and dst water tile doesn't contain boat/hero (objs visitable from land)
			&& complain("Cannot move hero, destination tile is on water!")
		|| (h->boat && t.tertype != TerrainTile::water && t.blocked)
			&& complain("Cannot disembark hero, tile is blocked!")
		|| (h->movement < cost  &&  dst != h->pos  &&  !instant)
			&& complain("Hero doesn't have any movement points left!")
		|| states.checkFlag(h->tempOwner, &PlayerStatus::engagedIntoBattle)
			&& complain("Cannot move hero during the battle"))
	{
		//send info about movement failure
		sendAndApply(&tmh);
		return false;
	}

	//hero enters the boat
	if(!h->boat && t.visitableObjects.size() && t.visitableObjects.front()->ID == 8)
	{
		tmh.result = TryMoveHero::EMBARK;
		tmh.movePoints = 0; //embarking takes all move points
		//TODO: check for bonus that removes that penalty

		getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);
		sendAndApply(&tmh);
		return true;
	}
	//hero leaves the boat
	else if(h->boat && t.tertype != TerrainTile::water && !t.blocked)
	{
		tmh.result = TryMoveHero::DISEMBARK;
		tmh.movePoints = 0; //disembarking takes all move points
		//TODO: check for bonus that removes that penalty

		getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);
		sendAndApply(&tmh);
		tryAttackingGuard(guardPos, h);
		return true;
	}

	//checks for standard movement
	if(!instant)
	{
		if( distance(h->pos,dst) >= 1.5  &&  complain("Tiles are not neighboring!")
			|| h->movement < cost  &&  h->movement < 100  &&  complain("Not enough move points!")) 
		{
			sendAndApply(&tmh);
			return false;
		}

		//check if there is blocking visitable object
		blockvis = false;
		tmh.movePoints = std::max(si32(0),h->movement-cost); //take move points
		BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
		{
			if(obj != h  &&  obj->blockVisit  &&  !(obj->getPassableness() & 1<<h->tempOwner))
			{
				blockvis = true;
				break;
			}
		}
		//we start moving
		if(blockvis)//interaction with blocking object (like resources)
		{
			tmh.result = TryMoveHero::BLOCKING_VISIT;
			sendAndApply(&tmh); 
			//failed to move to that tile but we visit object
			if(t.visitableObjects.size())
				objectVisited(t.visitableObjects.back(), h);


// 			BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
// 			{
// 				if (obj->blockVisit)
// 				{
// 					objectVisited(obj, h);
// 				}
// 			}
			tlog5 << "Blocking visit at " << hmpos << std::endl;
			return true;
		}
		else //normal move
		{
			BOOST_FOREACH(CGObjectInstance *obj, gs->map->terrain[h->pos.x-1][h->pos.y][h->pos.z].visitableObjects)
			{
				obj->onHeroLeave(h);
			}
			getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);

			tmh.result = TryMoveHero::SUCCESS;
			tmh.attackedFrom = guardPos;
			sendAndApply(&tmh);
			tlog5 << "Moved to " <<tmh.end<<std::endl;

			// If a creature guards the tile, block visit.
			const bool fightingGuard = tryAttackingGuard(guardPos, h);

			if(!fightingGuard && t.visitableObjects.size()) //call objects if they are visited
			{
				visitObjectOnTile(t, h);
			}
// 			BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
// 			{
// 				objectVisited(obj, h);
// 			}

			tlog5 << "Movement end!\n";
			return true;
		}
	}
	else //instant move - teleportation
	{
		BOOST_FOREACH(CGObjectInstance* obj, t.blockingObjects)
		{
			if(obj->ID==HEROI_TYPE)
			{
				CGHeroInstance *dh = static_cast<CGHeroInstance *>(obj);

				if(obj->tempOwner==h->tempOwner) 
				{
					heroExchange(dh->id, h->id);
					return true;
				}
				//TODO: check for ally
				startBattleI(h, dh);
				return true;
			}
		}
		tmh.result = TryMoveHero::TELEPORTATION;
		getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);
		sendAndApply(&tmh);
		return true;
	}
}
void CGameHandler::setOwner(int objid, ui8 owner)
{
	ui8 oldOwner = getOwner(objid);
	SetObjectProperty sop(objid,1,owner);
	sendAndApply(&sop);

	winLoseHandle(1<<owner | 1<<oldOwner);
	if(owner < PLAYER_LIMIT && getTown(objid) && !gs->getPlayer(owner)->towns.size()) //player lost last town
	{
		InfoWindow iw;
		iw.player = oldOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 6); //%s, you have lost your last town.  If you do not conquer another town in the next week, you will be eliminated.
		sendAndApply(&iw);
	}
}
void CGameHandler::setHoverName(int objid, MetaString* name)
{
	SetHoverName shn(objid, *name);
	sendAndApply(&shn);
}
void CGameHandler::showInfoDialog(InfoWindow *iw)
{
	sendToAllClients(iw);
}
void CGameHandler::showBlockingDialog( BlockingDialog *iw, const CFunctionList<void(ui32)> &callback )
{
	ask(iw,iw->player,callback);
}

ui32 CGameHandler::showBlockingDialog( BlockingDialog *iw )
{
	//TODO

	//gsm.lock();
	//int query = QID++;
	//states.addQuery(player,query);
	//sendToAllClients(iw);
	//gsm.unlock();
	//ui32 ret = getQueryResult(iw->player, query);
	//gsm.lock();
	//states.removeQuery(player, query);
	//gsm.unlock();
	return 0;
}

int CGameHandler::getCurrentPlayer()
{
	return gs->currentPlayer;
}

void CGameHandler::giveResource(int player, int which, int val)
{
	if(!val) return; //don't waste time on empty call
	SetResource sr;
	sr.player = player;
	sr.resid = which;
	sr.val = gs->players.find(player)->second.resources[which]+val;
	sendAndApply(&sr);
}
void CGameHandler::giveCreatures (int objid, const CGHeroInstance * h, CCreatureSet creatures)
{
	if (creatures.stacksCount() <= 0)
		return;
	CCreatureSet heroArmy = h->getArmy();
	while (creatures.stacksCount() > 0)
	{
		int slot = heroArmy.getSlotFor(creatures.Slots().begin()->second.type->idNumber);
		if (slot < 0)
			break;
		heroArmy.addToSlot(slot, creatures.slots.begin()->second);
		creatures.slots.erase (creatures.slots.begin());
	}

	if (creatures.stacksCount() == 0) //all creatures can be moved to hero army - do that
	{
		SetGarrisons sg;
		sg.garrs[h->id] = heroArmy;
		sendAndApply(&sg);
	}
	else //show garrison window and let player pick creatures
	{
		SetGarrisons sg;
		sg.garrs[objid] = creatures;
		sendAndApply (&sg);
		showGarrisonDialog (objid, h->id, true, 0);
		return;
	}
}
void CGameHandler::takeCreatures (int objid, TSlots creatures) //probably we could use ArmedInstance as well
{
	if (creatures.size() <= 0)
		return;
	const CArmedInstance* obj = static_cast<const CArmedInstance*>(getObj(objid));
	CCreatureSet newArmy = obj->getArmy();
	while (creatures.size())
	{
		int slot = newArmy.getSlotFor(creatures.begin()->second.type->idNumber);
		if (slot < 0)
			break;
		newArmy.slots[slot].type = creatures.begin()->second.type;
		newArmy.slots[slot].count -= creatures.begin()->second.count;
		if (newArmy.getStack(slot).count < 1)
			newArmy.eraseStack(slot);
		creatures.erase (creatures.begin());
	}
	SetGarrisons sg;
	sg.garrs[objid] = newArmy;
	sendAndApply(&sg);
}
void CGameHandler::showCompInfo(ShowInInfobox * comp)
{
	sendToAllClients(comp);
}
void CGameHandler::heroVisitCastle(int obj, int heroID)
{
	HeroVisitCastle vc;
	vc.hid = heroID;
	vc.tid = obj;
	vc.flags |= 1;
	sendAndApply(&vc);
	const CGHeroInstance *h = getHero(heroID);
	vistiCastleObjects (getTown(obj), h);
	giveSpells (getTown(obj), getHero(heroID));

	if(gs->map->victoryCondition.condition == transportItem)
		checkLossVictory(h->tempOwner); //transported artifact?
}

void CGameHandler::vistiCastleObjects (const CGTownInstance *t, const CGHeroInstance *h)
{
	std::vector<CGTownBuilding*>::const_iterator i;
	for (i = t->bonusingBuildings.begin(); i != t->bonusingBuildings.end(); i++)
		(*i)->onHeroVisit (h);
}

void CGameHandler::stopHeroVisitCastle(int obj, int heroID)
{
	HeroVisitCastle vc;
	vc.hid = heroID;
	vc.tid = obj;
	sendAndApply(&vc);
}
void CGameHandler::giveHeroArtifact(int artid, int hid, int position) //pos==-1 - first free slot in backpack
{
	const CGHeroInstance* h = getHero(hid);
	const CArtifact &art = VLC->arth->artifacts[artid];

	SetHeroArtifacts sha;
	sha.hid = hid;
	sha.artifacts = h->artifacts;
	sha.artifWorn = h->artifWorn;

	if(position<0)
	{
		if(position == -2)
		{
			int i;
			for(i=0; i<art.possibleSlots.size(); i++) //try to put artifact into first available slot
			{
				if( !vstd::contains(sha.artifWorn,art.possibleSlots[i]) )
				{
					//we've found a free suitable slot
					VLC->arth->equipArtifact(sha.artifWorn, art.possibleSlots[i], artid);
					break;
				}
			}
			if(i == art.possibleSlots.size() && !art.isBig()) //if haven't find proper slot, use backpack or discard big artifact
				sha.artifacts.push_back(artid);
		}
		else if (!art.isBig()) //should be -1 => put artifact into backpack
		{
			sha.artifacts.push_back(artid);
		}
	}
	else
	{
		if(!vstd::contains(sha.artifWorn,ui16(position)))
		{
			VLC->arth->equipArtifact(sha.artifWorn, position, artid);
		}
		else if (!art.isBig())
		{
			sha.artifacts.push_back(artid);
		}
	}

	sendAndApply(&sha);
}
void CGameHandler::removeArtifact(int artid, int hid)
{
	const CGHeroInstance* h = getHero(hid);

	SetHeroArtifacts sha;
	sha.hid = hid;
	sha.artifacts = h->artifacts;
	sha.artifWorn = h->artifWorn;
	
	std::vector<ui32>::iterator it;
	if 	((it = std::find(sha.artifacts.begin(), sha.artifacts.end(), artid)) != sha.artifacts.end()) //it is in backpack
		sha.artifacts.erase(it);
	else //worn
	{
		for (std::map<ui16,ui32>::iterator itr = sha.artifWorn.begin(); itr != sha.artifWorn.end(); ++itr)
		{
			if (itr->second == artid)
			{
				VLC->arth->unequipArtifact(sha.artifWorn, itr->first);
				break;
			}
		}
	}
	sendAndApply(&sha);
}

void CGameHandler::startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank, boost::function<void(BattleResult*)> cb, const CGTownInstance *town) //use hero=NULL for no hero
{
	engageIntoBattle(army1->tempOwner);
	engageIntoBattle(army2->tempOwner);
	//block engaged players
	if(army2->tempOwner < PLAYER_LIMIT)
		states.setFlag(army2->tempOwner,&PlayerStatus::engagedIntoBattle,true);

	boost::thread(boost::bind(&CGameHandler::startBattle, this, army1, army2, tile, hero1, hero2, creatureBank, cb, town));
}

void CGameHandler::startBattleI( const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, boost::function<void(BattleResult*)> cb, bool creatureBank )
{
	startBattleI(army1, army2, tile,
		army1->ID == HEROI_TYPE ? static_cast<const CGHeroInstance*>(army1) : NULL, 
		army2->ID == HEROI_TYPE ? static_cast<const CGHeroInstance*>(army2) : NULL, 
		creatureBank, cb);
}

void CGameHandler::startBattleI( const CArmedInstance *army1, const CArmedInstance *army2, boost::function<void(BattleResult*)> cb, bool creatureBank)
{
	startBattleI(army1, army2, army2->visitablePos(), cb, creatureBank);
}

//void CGameHandler::startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb) //for hero<=>neutral army
//{
//	CGHeroInstance* h = const_cast<CGHeroInstance*>(getHero(heroID));
//	startBattleI(&h->army,&army,tile,h,NULL,cb);
//	//battle(&h->army,army,tile,h,NULL);
//}

void CGameHandler::changeSpells( int hid, bool give, const std::set<ui32> &spells )
{
	ChangeSpells cs;
	cs.hid = hid;
	cs.spells = spells;
	cs.learn = give;
	sendAndApply(&cs);
}

int CGameHandler::getSelectedHero() 
{
	return IGameCallback::getSelectedHero(getCurrentPlayer())->id;
}

void CGameHandler::setObjProperty( int objid, int prop, si64 val )
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.val = val;
	sendAndApply(&sob);
}

void CGameHandler::sendMessageTo( CConnection &c, const std::string &message )
{
	SystemMessage sm;
	sm.text = message;
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

void CGameHandler::setManaPoints( int hid, int val )
{
	SetMana sm;
	sm.hid = hid;
	sm.val = val;
	sendAndApply(&sm);
}

void CGameHandler::giveHero( int id, int player )
{
	GiveHero gh;
	gh.id = id;
	gh.player = player;
	sendAndApply(&gh);
}

void CGameHandler::changeObjPos( int objid, int3 newPos, ui8 flags )
{
	ChangeObjPos cop;
	cop.objid = objid;
	cop.nPos = newPos;
	cop.flags = flags;
	sendAndApply(&cop);
}

void CGameHandler::useScholarSkill(si32 fromHero, si32 toHero)
{
	const CGHeroInstance * h1 = getHero(fromHero);
	const CGHeroInstance * h2 = getHero(toHero);

	if ( h1->getSecSkillLevel(18) < h2->getSecSkillLevel(18) )
	{
		std::swap (h1,h2);//1st hero need to have higher scholar level for correct message
		std::swap(fromHero, toHero);
	}

	int ScholarLevel = h1->getSecSkillLevel(18);//heroes can trade up to this level
	if (!ScholarLevel || !vstd::contains(h1->artifWorn,17) || !vstd::contains(h2->artifWorn,17) )
		return;//no scholar skill or no spellbook

	int h1Lvl = std::min(ScholarLevel+1, h1->getSecSkillLevel(7)+2),
	    h2Lvl = std::min(ScholarLevel+1, h2->getSecSkillLevel(7)+2);//heroes can receive this levels

	ChangeSpells cs1;
	cs1.learn = true;
	cs1.hid = toHero;//giving spells to first hero
		for(std::set<ui32>::const_iterator it=h1->spells.begin(); it!=h1->spells.end();it++)
			if ( h2Lvl >= VLC->spellh->spells[*it].level && !vstd::contains(h2->spells, *it))//hero can learn it and don't have it yet
				cs1.spells.insert(*it);//spell to learn

	ChangeSpells cs2;
	cs2.learn = true;
	cs2.hid = fromHero;

	for(std::set<ui32>::const_iterator it=h2->spells.begin(); it!=h2->spells.end();it++)
		if ( h1Lvl >= VLC->spellh->spells[*it].level && !vstd::contains(h1->spells, *it))
			cs2.spells.insert(*it);
			
	if (cs1.spells.size() || cs2.spells.size())//create a message
	{		
		InfoWindow iw;
		iw.player = h1->tempOwner;
		iw.components.push_back(Component(Component::SEC_SKILL, 18, ScholarLevel, 0));

		iw.text.addTxt(MetaString::GENERAL_TXT, 139);//"%s, who has studied magic extensively,
		iw.text.addReplacement(h1->name);
		
		if (cs2.spells.size())//if found new spell - apply
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 140);//learns
			int size = cs2.spells.size();
			for(std::set<ui32>::const_iterator it=cs2.spells.begin(); it!=cs2.spells.end();it++)
			{
				iw.components.push_back(Component(Component::SPELL, (*it), 1, 0));
				iw.text.addTxt(MetaString::SPELL_NAME, (*it));
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

		if (cs1.spells.size() && cs2.spells.size() )
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 141);//and
		}

		if (cs1.spells.size())
		{
			iw.text.addTxt(MetaString::GENERAL_TXT, 147);//teaches
			int size = cs1.spells.size();
			for(std::set<ui32>::const_iterator it=cs1.spells.begin(); it!=cs1.spells.end();it++)
			{
				iw.components.push_back(Component(Component::SPELL, (*it), 1, 0));
				iw.text.addTxt(MetaString::SPELL_NAME, (*it));
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

void CGameHandler::heroExchange(si32 hero1, si32 hero2)
{
	ui8 player1 = getHero(hero1)->tempOwner;
	ui8 player2 = getHero(hero2)->tempOwner;

	if(player1 == player2)//TODO: allies
	{
		OpenWindow hex;
		hex.window = OpenWindow::EXCHANGE_WINDOW;
		hex.id1 = hero1;
		hex.id2 = hero2;
		sendAndApply(&hex);
		useScholarSkill(hero1,hero2);
	}
}

void CGameHandler::applyAndAsk( Query * sel, ui8 player, boost::function<void(ui32)> &callback )
{
	boost::unique_lock<boost::recursive_mutex> lock(gsm);
	sel->id = QID;
	callbacks[QID] = callback;
	states.addQuery(player,QID);
	QID++; 
	sendAndApply(sel);
}

void CGameHandler::ask( Query * sel, ui8 player, const CFunctionList<void(ui32)> &callback )
{
	boost::unique_lock<boost::recursive_mutex> lock(gsm);
	sel->id = QID;
	callbacks[QID] = callback;
	states.addQuery(player,QID);
	sendToAllClients(sel);
	QID++; 
}

void CGameHandler::sendToAllClients( CPackForClient * info )
{
	tlog5 << "Sending to all clients a package of type " << typeid(*info).name() << std::endl;
	for(std::set<CConnection*>::iterator i=conns.begin(); i!=conns.end();i++)
	{
		(*i)->wmx->lock();
		**i << info;
		(*i)->wmx->unlock();
	}
}

void CGameHandler::sendAndApply( CPackForClient * info )
{
	gs->apply(info);
	sendToAllClients(info);
}

void CGameHandler::sendAndApply( SetGarrisons * info )
{
	sendAndApply((CPackForClient*)info);
	if(gs->map->victoryCondition.condition == gatherTroop)
		for(std::map<ui32,CCreatureSet>::const_iterator i = info->garrs.begin(); i != info->garrs.end(); i++)
			checkLossVictory(getObj(i->first)->tempOwner);
}

void CGameHandler::sendAndApply( SetResource * info )
{
	sendAndApply((CPackForClient*)info);
	if(gs->map->victoryCondition.condition == gatherResource)
		checkLossVictory(info->player);
}

void CGameHandler::sendAndApply( SetResources * info )
{
	sendAndApply((CPackForClient*)info);
	if(gs->map->victoryCondition.condition == gatherResource)
		checkLossVictory(info->player);
}

void CGameHandler::sendAndApply( NewStructures * info )
{
	sendAndApply((CPackForClient*)info);
	if(gs->map->victoryCondition.condition == buildCity)
		checkLossVictory(getTown(info->tid)->tempOwner);
}
void CGameHandler::save( const std::string &fname )
{
	{
		tlog0 << "Ordering clients to serialize...\n";
		SaveGame sg(fname);

		sendToAllClients(&sg);
	}

	{
		tlog0 << "Serializing game info...\n";
		CSaveFile save(GVCMIDirs.UserPath + "/Games/" + fname + ".vlgm1");
		char hlp[8] = "VCMISVG";
		save << hlp << static_cast<CMapHeader&>(*gs->map) << gs->scenarioOps << *VLC << gs;
	}

	{
		tlog0 << "Serializing server info...\n";
		CSaveFile save(GVCMIDirs.UserPath + "/Games/" + fname + ".vsgm1");
		save << *this;
	}
	tlog0 << "Game has been successfully saved!\n";
}

void CGameHandler::close()
{
	tlog0 << "We have been requested to close.\n";	
	//BOOST_FOREACH(CConnection *cc, conns)
	//	if(cc && cc->socket && cc->socket->is_open())
	//		cc->socket->close();
	//exit(0);
}

bool CGameHandler::arrangeStacks( si32 id1, si32 id2, ui8 what, ui8 p1, ui8 p2, si32 val )
{
	CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id1]),
		*s2 = static_cast<CArmedInstance*>(gs->map->objects[id2]);
	CCreatureSet temp1 = s1->getArmy(), temp2 = s2->getArmy(),
		&S1 = temp1, &S2 = (s1!=s2)?(temp2):(temp1);

	if(!isAllowedExchange(id1,id2))
	{
		complain("Cannot exchange stacks between these two objects!\n");
		return false;
	}

	if(what==1) //swap
	{
		std::swap(S1.slots[p1],S2.slots[p2]); //swap slots

		//if one of them is empty, remove entry
		if(!S1.slots[p1].count)
			S1.slots.erase(p1);
		if(!S2.slots[p2].count)
			S2.slots.erase(p2);
	}
	else if(what==2)//merge
	{
		if(S1.slots[p1].type != S2.slots[p2].type) //not same creature
		{
			complain("Cannot merge different creatures stacks!");
			return false; 
		}

		S2.slots[p2].count += S1.slots[p1].count;
		S1.slots.erase(p1);
	}
	else if(what==3) //split
	{
		//general conditions checking
		if((!vstd::contains(S1.slots,p1) && complain("no creatures to split"))
			|| (val<1  && complain("no creatures to split"))  )
		{
			return false;
		}


		if(vstd::contains(S2.slots,p2))	 //dest. slot not free - it must be "rebalancing"...
		{
			int total = S1.slots[p1].count + S2.slots[p2].count;
			if( (total < val   &&   complain("Cannot split that stack, not enough creatures!"))
				|| (S2.slots[p2].type != S1.slots[p1].type && complain("Cannot rebalance different creatures stacks!"))
			)
			{
				return false; 
			}
			
			S2.slots[p2].count = val;
			S1.slots[p1].count = total - val;
		}
		else //split one stack to the two
		{
			if(S1.slots[p1].count < val)//not enough creatures
			{
				complain("Cannot split that stack, not enough creatures!");
				return false; 
			}
			S2.slots[p2].type = S1.slots[p1].type;
			S2.slots[p2].count = val;
			S1.slots[p1].count -= val;
		}

		if(!S1.slots[p1].count) //if we've moved all creatures
			S1.slots.erase(p1);
	}
	if((s1->needsLastStack() && !S1.stacksCount()) //it's not allowed to take last stack from hero army!
		|| (s2->needsLastStack() && !S2.stacksCount())
	)
	{
		complain("Cannot take the last stack!");
		return false; //leave without applying changes to garrison
	}

	//apply changes
	SetGarrisons sg;
	sg.garrs[id1] = S1;
	if(s1 != s2)
		sg.garrs[id2] = S2;
	sendAndApply(&sg);
	return true;
}

int CGameHandler::getPlayerAt( CConnection *c ) const
{
	std::set<int> all;
	for(std::map<int,CConnection*>::const_iterator i=connections.begin(); i!=connections.end(); i++)
		if(i->second == c)
			all.insert(i->first);

	switch(all.size())
	{
	case 0:
		return 255;
	case 1:
		return *all.begin();
	default:
		{
			//if we have more than one player at this connection, try to pick active one
			if(vstd::contains(all,int(gs->currentPlayer)))
				return gs->currentPlayer;
			else
				return 253; //cannot say which player is it
		}
	}
}

bool CGameHandler::disbandCreature( si32 id, ui8 pos )
{
	CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id]);
	if(!vstd::contains(s1->slots,pos))
	{
		complain("Illegal call to disbandCreature - no such stack in army!");
		return false;
	}
	s1->slots.erase(pos);
	SetGarrisons sg;
	sg.garrs[id] = s1->getArmy();
	sendAndApply(&sg);
	return true;
}

bool CGameHandler::buildStructure( si32 tid, si32 bid )
{
	CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[tid]);
	CBuilding * b = VLC->buildh->buildings[t->subID][bid];

	if(gs->canBuildStructure(t,bid) != 7)
	{
		complain("Cannot build that building!");
		return false;
	}
	
	if(bid == 26) //grail
	{
		if(!t->visitingHero || !t->visitingHero->hasArt(2))
		{
			complain("Cannot build grail - hero doesn't have it");
			return false;
		}

		removeArtifact(2, t->visitingHero->id);
	}

	NewStructures ns;
	ns.tid = tid;
	if ( (bid == 18) && (vstd::contains(t->builtBuildings,(t->town->hordeLvl[0]+37))) )
		ns.bid.insert(19);//we have upgr. dwelling, upgr. horde will be builded as well
	else if ( (bid == 24) && (vstd::contains(t->builtBuildings,(t->town->hordeLvl[1]+37))) )
		ns.bid.insert(25);
	else if(bid>36) //upg dwelling
	{
		if ( (bid-37 == t->town->hordeLvl[0]) && (vstd::contains(t->builtBuildings,18)) )
			ns.bid.insert(19);//we have horde, will be upgraded as well as dwelling
		if ( (bid-37 == t->town->hordeLvl[1]) && (vstd::contains(t->builtBuildings,24)) )
			ns.bid.insert(25);

		SetAvailableCreatures ssi;
		ssi.tid = tid;
		ssi.creatures = t->creatures;
		ssi.creatures[bid-37].second.push_back(t->town->upgradedCreatures[bid-37]);
		sendAndApply(&ssi);
	}
	else if(bid >= 30) //bas. dwelling
	{
		int crid = t->town->basicCreatures[bid-30];
		SetAvailableCreatures ssi;
		ssi.tid = tid;
		ssi.creatures = t->creatures;
		ssi.creatures[bid-30].first = VLC->creh->creatures[crid]->growth;
		ssi.creatures[bid-30].second.push_back(crid);
		sendAndApply(&ssi);
	}
	else if(bid == 11)
		ns.bid.insert(27);
	else if(bid == 12)
		ns.bid.insert(28);
	else if(bid == 13)
		ns.bid.insert(29);
	else if (t->subID == 4 && bid == 17) //veil of darkness
	{
		GiveBonus gb(GiveBonus::TOWN);
		gb.bonus.type = Bonus::DARKNESS;
		gb.bonus.val = 20;
		gb.id = t->id;
		gb.bonus.duration = Bonus::PERMANENT;
		gb.bonus.source = Bonus::TOWN_STRUCTURE;
		gb.bonus.id = 17;
		sendAndApply(&gb);
	}

	ns.bid.insert(bid);
	ns.builded = t->builded + 1;
	sendAndApply(&ns);
	
	//reveal ground for lookout tower
	FoWChange fw;
	fw.player = t->tempOwner;
	fw.mode = 1;
	getTilesInRange(fw.tiles,t->pos,t->getSightRadious(),t->tempOwner,1);
	sendAndApply(&fw);

	SetResources sr;
	sr.player = t->tempOwner;
	sr.res = gs->getPlayer(t->tempOwner)->resources;
	for(int i=0;i<b->resources.size();i++)
		sr.res[i]-=b->resources[i];
	sendAndApply(&sr);

	if(bid<5) //it's mage guild
	{
		if(t->visitingHero)
			giveSpells(t,t->visitingHero);
		if(t->garrisonHero)
			giveSpells(t,t->garrisonHero);
	}
	if(t->visitingHero)
		vistiCastleObjects (t, t->visitingHero);
	if(t->garrisonHero)
		vistiCastleObjects (t, t->garrisonHero);

	checkLossVictory(t->tempOwner);
	return true;
}
bool CGameHandler::razeStructure (si32 tid, si32 bid)
{
///incomplete, simply erases target building
	CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[tid]);
	if (t->builtBuildings.find(bid) == t->builtBuildings.end())
		return false;
	RazeStructures rs;
	rs.tid = tid;
	rs.bid.insert(bid);
	rs.destroyed = t->destroyed + 1;
	sendAndApply(&rs);
//TODO: Remove dwellers
	if (t->subID == 4 && bid == 17) //Veil of Darkness
	{
		RemoveBonus rb(RemoveBonus::TOWN);
		rb.whoID = t->id;
		rb.source = Bonus::TOWN_STRUCTURE;
		rb.id = 17;
		sendAndApply(&rb);
	}
	return true;
}

void CGameHandler::sendMessageToAll( const std::string &message )
{
	SystemMessage sm;
	sm.text = message;
	sendToAllClients(&sm);
}

bool CGameHandler::recruitCreatures( si32 objid, ui32 crid, ui32 cram )
{
	const CGDwelling *dw = static_cast<CGDwelling*>(gs->map->objects[objid]);
	const CArmedInstance *dst = NULL;
	const CCreature *c = VLC->creh->creatures[crid];
	bool warMachine = c->hasBonusOfType(Bonus::SIEGE_WEAPON);

	//TODO: test for owning

	if(dw->ID == TOWNI_TYPE)
		dst = dw;
	else if(dw->ID == 17  ||  dw->ID == 20) //advmap dwelling
		dst = getHero(gs->getPlayer(dw->tempOwner)->currentSelection); //TODO: check if current hero is really visiting dwelling
	else if(dw->ID == 106)
		dst = dynamic_cast<const CGHeroInstance *>(getTile(dw->visitablePos())->visitableObjects.back());

	assert(dw && dst);

	//verify
	bool found = false;
	int level = -1;


	typedef std::pair<const int,int> Parka;
	for(level = 0; level < dw->creatures.size(); level++) //iterate through all levels
	{
		const std::pair<ui32, std::vector<ui32> > &cur = dw->creatures[level]; //current level info <amount, list of cr. ids>
		int i = 0;
		for(; i < cur.second.size(); i++) //look for crid among available creatures list on current level
			if(cur.second[i] == crid)
				break;

		if(i < cur.second.size())
		{
			found = true;
			cram = std::min(cram, cur.first); //reduce recruited amount up to available amount
			break;
		}
	}
	int slot = dst->getSlotFor(crid); 

	if(!found && complain("Cannot recruit: no such creatures!")
		|| cram  >  VLC->creh->creatures[crid]->maxAmount(gs->getPlayer(dst->tempOwner)->resources) && complain("Cannot recruit: lack of resources!")
		|| cram<=0  &&  complain("Cannot recruit: cram <= 0!")
		|| slot<0  && !warMachine && complain("Cannot recruit: no available slot!")) 
	{
		return false;
	}

	//recruit
	SetResources sr;
	sr.player = dst->tempOwner;
	for(int i=0;i<RESOURCE_QUANTITY;i++)
		sr.res[i]  =  gs->getPlayer(dst->tempOwner)->resources[i] - (c->cost[i] * cram);

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = dw->creatures;
	sac.creatures[level].first -= cram;

	sendAndApply(&sr);
	sendAndApply(&sac);
	
	if(warMachine)
	{
		switch(crid)
		{
		case 146:
			giveHeroArtifact(4, dst->id, 13);
			break;
		case 147:
			giveHeroArtifact(6, dst->id, 15);
			break;
		case 148:
			giveHeroArtifact(5, dst->id, 14);
			break;
		default:
			complain("This war machine cannot be recruited!");
			return false;
		}
	}
	else
	{
		SetGarrisons sg;
		sg.garrs[dst->id] = dst->getArmy();
		sg.garrs[dst->id] .addToSlot(slot, crid, cram);
		sendAndApply(&sg);
	}
	return true;
}

bool CGameHandler::upgradeCreature( ui32 objid, ui8 pos, ui32 upgID )
{
	CArmedInstance *obj = static_cast<CArmedInstance*>(gs->map->objects[objid]);
	UpgradeInfo ui = gs->getUpgradeInfo(obj,pos);
	int player = obj->tempOwner;
	int crQuantity = obj->slots[pos].count;

	//check if upgrade is possible
	if((ui.oldID<0 || !vstd::contains(ui.newID,upgID)) && complain("That upgrade is not possible!")) 
	{
		return false;
	}

	//check if player has enough resources
	for(int i=0;i<ui.cost.size();i++)
	{
		for (std::set<std::pair<int,int> >::iterator j=ui.cost[i].begin(); j!=ui.cost[i].end(); j++)
		{
			if(gs->getPlayer(player)->resources[j->first] < j->second*crQuantity)
			{
				complain("Cannot upgrade, not enough resources!");
				return false;
			}
		}
	}

	//take resources
	for(int i=0;i<ui.cost.size();i++)
	{
		for (std::set<std::pair<int,int> >::iterator j=ui.cost[i].begin(); j!=ui.cost[i].end(); j++)
		{
			SetResource sr;
			sr.player = player;
			sr.resid = j->first;
			sr.val = gs->getPlayer(player)->resources[j->first] - j->second*crQuantity;
			sendAndApply(&sr);
		}
	}

	//upgrade creature
	SetGarrisons sg;
	sg.garrs[objid] = obj->getArmy();
	sg.garrs[objid].slots[pos].setType(upgID);
	sendAndApply(&sg);	
	return true;
}

bool CGameHandler::garrisonSwap( si32 tid )
{
	CGTownInstance *town = gs->getTown(tid);
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{
		CCreatureSet csn = town->visitingHero->getArmy(), cso = town->getArmy();
		while(!cso.slots.empty())//while there are unmoved creatures
		{
			int pos = csn.getSlotFor(cso.slots.begin()->second.type->idNumber);
			if(pos<0)
			{
				//try to merge two other stacks to make place
				std::pair<TSlot, TSlot> toMerge;
				if(csn.mergableStacks(toMerge, cso.slots.begin()->first))
				{
					//merge
					csn.slots[toMerge.second].count += csn.slots[toMerge.first].count;
					csn.slots[toMerge.first] = cso.slots.begin()->second;
				}
				else
				{
					complain("Cannot make garrison swap, not enough free slots!");
					return false;
				}
			}
			else if(csn.slots.find(pos) != csn.slots.end()) //add creatures to the existing stack
			{
				csn.slots[pos].count += cso.slots.begin()->second.count;
			}
			else //move stack on the free pos
			{
				csn.slots[pos].type = cso.slots.begin()->second.type;
				csn.slots[pos].count = cso.slots.begin()->second.count;
			}
			cso.slots.erase(cso.slots.begin());
		}
		SetGarrisons sg;
		sg.garrs[town->visitingHero->id] = csn;
		sg.garrs[town->id] = csn;
		sendAndApply(&sg);

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.visiting = -1;
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
		intown.garrison = -1;
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(&intown);

		//town will be empty
		SetGarrisons sg;
		sg.garrs[tid] = CCreatureSet();
		sendAndApply(&sg);
		return true;
	}
	else if (town->garrisonHero && town->visitingHero) //swap visiting and garrison hero
	{
		SetGarrisons sg;
		sg.garrs[town->id] = town->visitingHero->getArmy();;
		sg.garrs[town->garrisonHero->id] = town->garrisonHero->getArmy();

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = town->visitingHero->id;
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(&intown);
		sendAndApply(&sg);
		return true;
	}
	else
	{
		complain("Cannot swap garrison hero!");
		return false;
	}
}

// With the amount of changes done to the function, it's more like transferArtifacts.
bool CGameHandler::swapArtifacts(si32 srcHeroID, si32 destHeroID, ui16 srcSlot, ui16 destSlot)
{
	CGHeroInstance *srcHero = gs->getHero(srcHeroID);
	CGHeroInstance *destHero = gs->getHero(destHeroID);

	// Make sure exchange is even possible between the two heroes.
	if ((distance(srcHero->pos,destHero->pos) > 1.5 )|| (srcHero->tempOwner != destHero->tempOwner))
		return false;

	const CArtifact *srcArtifact = srcHero->getArt(srcSlot);
	const CArtifact *destArtifact = destHero->getArt(destSlot);

	if (srcArtifact == NULL) {
		complain("No artifact to swap!");
		return false;
	}

	SetHeroArtifacts sha;
	sha.hid = srcHeroID;
	sha.artifacts = srcHero->artifacts;
	sha.artifWorn = srcHero->artifWorn;

	// Combinational artifacts needs to be removed first so they don't get denied movement because of their own locks.
	if (srcHeroID == destHeroID && srcSlot < 19 && destSlot < 19) {
		sha.setArtAtPos(srcSlot, -1);
		if (!vstd::contains(sha.artifWorn, destSlot))
			destArtifact = NULL;
	}

	// Check if src/dest slots are appropriate for the artifacts exchanged.
	// Moving to the backpack is always allowed.
	if ((!srcArtifact || destSlot < 19)
		&& (srcArtifact && !srcArtifact->fitsAt(srcHeroID == destHeroID ? sha.artifWorn : destHero->artifWorn, destSlot)))
	{
		complain("Cannot swap artifacts!");
		return false;
	}

	if ((srcArtifact && srcArtifact->id == 145) || (destArtifact && destArtifact->id == 145)) {
		complain("Cannot move artifact locks.");
		return false;
	}

	if (destSlot >= 19 && srcArtifact->isBig()) {
		complain("Cannot put big artifacts in backpack!");
		return false;
	}

	if (srcSlot == 16 || destSlot == 16) {
		complain("Cannot move catapult!");
		return false;
	}

	// If dest does not fit in src, put it in dest's backpack instead.
	if (srcHeroID == destHeroID) // To avoid stumbling on own locks, remove artifact first.
		sha.setArtAtPos(destSlot, -1);
	const bool destFits = !destArtifact || srcSlot >= 19 || destSlot >= 19 || destArtifact->fitsAt(sha.artifWorn, srcSlot);
	if (srcHeroID == destHeroID && destArtifact)
		sha.setArtAtPos(destSlot, destArtifact->id);

	sha.setArtAtPos(srcSlot, -1);
	if (destSlot < 19 && (destArtifact || srcSlot < 19) && destFits)
		sha.setArtAtPos(srcSlot, destArtifact ? destArtifact->id : -1);

	// Internal hero artifact arrangement.
	if(srcHero == destHero) {
		// Correction for destination from removing source artifact in backpack.
		if (srcSlot >= 19 && destSlot >= 19 && srcSlot < destSlot)
			destSlot--;

		sha.setArtAtPos(destSlot, srcHero->getArtAtPos(srcSlot));
	}
	sendAndApply(&sha);
	if (srcHeroID != destHeroID) {
		// Exchange between two different heroes.
		sha.hid = destHeroID;
		sha.artifacts = destHero->artifacts;
		sha.artifWorn = destHero->artifWorn;
		sha.setArtAtPos(destSlot, srcArtifact ? srcArtifact->id : -1);
		if (!destFits)
			sha.setArtAtPos(sha.artifacts.size() + 19, destHero->getArtAtPos(destSlot));
		sendAndApply(&sha);
	}

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
bool CGameHandler::assembleArtifacts (si32 heroID, ui16 artifactSlot, bool assemble, ui32 assembleTo)
{
	if (artifactSlot < 0 || artifactSlot > 18) {
		complain("Illegal artifact slot.");
		return false;
	}

	CGHeroInstance *hero = gs->getHero(heroID);
	const CArtifact *destArtifact = hero->getArt(artifactSlot);

	SetHeroArtifacts sha;
	sha.hid = heroID;
	sha.artifacts = hero->artifacts;
	sha.artifWorn = hero->artifWorn;

	if (assemble) {
		if (VLC->arth->artifacts.size() < assembleTo) {
			complain("Illegal artifact to assemble to.");
			return false;
		}

		if (!destArtifact->canBeAssembledTo(hero->artifWorn, assembleTo)) {
			complain("Artifact cannot be assembled.");
			return false;
		}

		const CArtifact &artifact = VLC->arth->artifacts[assembleTo];

		if (artifact.constituents == NULL) {
			complain("Not a combinational artifact.");
			return false;
		}

		// Perform assembly.
		bool destConsumed = false; // Determines which constituent that will be counted for together with the artifact.
		const bool destSpecific = vstd::contains(artifact.possibleSlots, artifactSlot); // Prefer the chosen slot as the location for the assembled artifact.

		BOOST_FOREACH(ui32 constituentID, *artifact.constituents) {
			if (destSpecific && constituentID == destArtifact->id) {
				sha.artifWorn[artifactSlot] = assembleTo;
				destConsumed = true;
				continue;
			}

			bool found = false;
			for (std::map<ui16, ui32>::iterator it = sha.artifWorn.begin(); it != sha.artifWorn.end(); ++it) {
				if (it->second == constituentID) { // Found possible constituent to substitute.
					if (destSpecific && !destConsumed && it->second == destArtifact->id) {
						// Find the specified destination for assembled artifact.
						if (it->first == artifactSlot) {
							it->second = assembleTo;
							destConsumed = true;

							found = true;
							break;
						}
					} else {
						// Either put the assembled artifact in a fitting spot, or put a lock.
						if (!destSpecific && !destConsumed && vstd::contains(artifact.possibleSlots, it->first)) {
							it->second = assembleTo;
							destConsumed = true;
						} else {
							it->second = 145;
						}

						found = true;
						break;
					}
				}
			}
			if (!found) {
				complain("Constituent missing.");
				return false;
			}
		}
	} else {
		// Perform disassembly.
		bool destConsumed = false; // Determines which constituent that will be counted for together with the artifact.
		BOOST_FOREACH(ui32 constituentID, *destArtifact->constituents) {
			const CArtifact &constituent = VLC->arth->artifacts[constituentID];

			if (!destConsumed && vstd::contains(constituent.possibleSlots, artifactSlot)) {
				sha.artifWorn[artifactSlot] = constituentID;
				destConsumed = true;
			} else {
				BOOST_REVERSE_FOREACH(ui16 slotID, constituent.possibleSlots) {
					if (vstd::contains(sha.artifWorn, slotID) && sha.artifWorn[slotID] == 145) {
						sha.artifWorn[slotID] = constituentID;
						break;
					}
				}
			}
		}
	}

	sendAndApply(&sha);

	return true;
}

bool CGameHandler::buyArtifact( ui32 hid, si32 aid )
{
	CGHeroInstance *hero = gs->getHero(hid);
	CGTownInstance *town = hero->visitedTown;
	if(aid==0) //spellbook
	{
		if(!vstd::contains(town->builtBuildings,si32(0)) && complain("Cannot buy a spellbook, no mage guild in the town!")
			|| getResource(hero->getOwner(),6)<500 && complain("Cannot buy a spellbook, not enough gold!") 
			|| hero->getArt(17) && complain("Cannot buy a spellbook, hero already has a one!")
			)
			return false;

		giveResource(hero->getOwner(),6,-500);
		giveHeroArtifact(0,hid,17);
		giveSpells(town,hero);
		return true;
	}
	else if(aid < 7  &&  aid > 3) //war machine
	{
		int price = VLC->arth->artifacts[aid].price;
		if(vstd::contains(hero->artifWorn,ui16(9+aid)) && complain("Hero already has this machine!")
			|| !vstd::contains(town->builtBuildings,si32(16)) && complain("No blackismith!")
			|| gs->getPlayer(hero->getOwner())->resources[6] < price  && complain("Not enough gold!")  //no gold
			|| (!(town->subID == 6 && vstd::contains(town->builtBuildings,si32(22) ) )
			&& town->town->warMachine!= aid ) &&  complain("This machine is unavailable here!") ) 
		{
			return false;
		}

		giveResource(hero->getOwner(),6,-price);
		giveHeroArtifact(aid,hid,9+aid);
		return true;
	}
	return false;
}


bool CGameHandler::tradeResources(const IMarket *market, ui32 val, ui8 player, ui32 id1, ui32 id2)
{
	int r1 = gs->getPlayer(player)->resources[id1], 
		r2 = gs->getPlayer(player)->resources[id2];

	amin(val, r1); //can't trade more resources than have

	int b1, b2; //base quantities for trade
	market->getOffer(id1, id2, b1, b2, RESOURCE_RESOURCE);
	int units = val / b1; //how many base quantities we trade

	if(val%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		//TODO: complain?
		assert(0);
	}

	SetResource sr;
	sr.player = player;
	sr.resid = id1;
	sr.val = r1 - b1 * units;
	sendAndApply(&sr);

	sr.resid = id2;
	sr.val = r2 + b2 * units;
	sendAndApply(&sr);

	return true;
}

bool CGameHandler::sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, ui32 slot, ui32 resourceID)
{
	if(!vstd::contains(hero->Slots(), slot))
		COMPLAIN_RET("Hero doesn't have any creature in that slot!");

	const CStackInstance &s = hero->getStack(slot);

	if(s.count < count  //can't sell more creatures than have
		|| hero->Slots().size() == 1  &&  hero->needsLastStack()  &&  s.count == count) //can't sell last stack
	{
		COMPLAIN_RET("Not enough creatures in army!");
	}

	int b1, b2; //base quantities for trade
 	market->getOffer(s.type->idNumber, resourceID, b1, b2, CREATURE_RESOURCE);
 	int units = count / b1; //how many base quantities we trade
 
 	if(count%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
 	{
 		//TODO: complain?
 		assert(0);
 	}
 

	SetGarrisons sg;
	sg.garrs[hero->id] = hero->getArmy();
	if(s.count > count)
		sg.garrs[hero->id].setStackCount(slot, s.count - count);
	else
		sg.garrs[hero->id].eraseStack(slot);
	sendAndApply(&sg);

 	SetResource sr;
 	sr.player = hero->tempOwner;
 	sr.resid = resourceID;
 	sr.val = getResource(hero->tempOwner, resourceID) + b2 * units;
 	sendAndApply(&sr);

	return true;
}

bool CGameHandler::sendResources(ui32 val, ui8 player, ui32 r1, ui32 r2)
{
	const PlayerState *p2 = gs->getPlayer(r2, false);
	if(!p2  ||  p2->status != PlayerState::INGAME)
	{
		complain("Dest player must be in game!");
		return false;
	}

	si32 curRes1 = gs->getPlayer(player)->resources[r1], curRes2 = gs->getPlayer(r2)->resources[r1];
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

bool CGameHandler::setFormation( si32 hid, ui8 formation )
{
	gs->getHero(hid)-> formation = formation;
	return true;
}

bool CGameHandler::hireHero( ui32 tid, ui8 hid )
{
	CGTownInstance *t = gs->getTown(tid);
	if(!vstd::contains(t->builtBuildings,5)  && complain("No tavern!")
		|| gs->getPlayer(t->tempOwner)->resources[6]<2500  && complain("Not enough gold for buying hero!")
		|| t->visitingHero  && complain("There is visiting hero - no place!")
		|| getHeroCount(t->tempOwner,false) >= 8 && complain("Cannot hire hero, only 8 wandering heroes are allowed!")
		)
		return false;
	CGHeroInstance *nh = gs->getPlayer(t->tempOwner)->availableHeroes[hid];
	assert(nh);

	HeroRecruited hr;
	hr.tid = tid;
	hr.hid = nh->subID;
	hr.player = t->tempOwner;
	hr.tile = t->pos - int3(1,0,0);
	sendAndApply(&hr);


	std::map<ui32,CGHeroInstance *> pool = gs->hpool.heroesPool;
	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
		for(std::vector<CGHeroInstance *>::iterator j = i->second.availableHeroes.begin(); j != i->second.availableHeroes.end(); j++)
			if(*j)
				pool.erase((**j).subID);

	SetAvailableHeroes sah;
	CGHeroInstance *h1 = gs->hpool.pickHeroFor(false,t->tempOwner,t->town, pool),
				*h2 = gs->getPlayer(t->tempOwner)->availableHeroes[!hid];
	(hid ? sah.hid2 : sah.hid1) = h1 ? h1->subID : -1;
	(hid ? sah.hid1 : sah.hid2) = h2 ? h2->subID : -1;
	sah.player = t->tempOwner;
	sah.flags = hid+1;
	sendAndApply(&sah);

	SetResource sr;
	sr.player = t->tempOwner;
	sr.resid = 6;
	sr.val = gs->getPlayer(t->tempOwner)->resources[6] - 2500;
	sendAndApply(&sr);

	vistiCastleObjects (t, nh);
	giveSpells (t,nh);
	return true;
}

bool CGameHandler::queryReply( ui32 qid, ui32 answer )
{
	boost::unique_lock<boost::recursive_mutex> lock(gsm);
	if(vstd::contains(callbacks,qid))
	{
		CFunctionList<void(ui32)> callb = callbacks[qid];
		callbacks.erase(qid);
		if(callb)
			callb(answer);
	}
	else if(vstd::contains(garrisonCallbacks,qid))
	{
		if(garrisonCallbacks[qid])
			garrisonCallbacks[qid]();
		garrisonCallbacks.erase(qid);
		allowedExchanges.erase(qid);
	}
	else
	{
		tlog1 << "Unknown query reply...\n";
		return false;
	}
	return true;
}

bool CGameHandler::makeBattleAction( BattleAction &ba )
{
	tlog1 << "\tMaking action of type " << ba.actionType << std::endl;
	bool ok = true;
	switch(ba.actionType)
	{
	case 2: //walk
		{
			sendAndApply(&StartAction(ba)); //start movement
			moveStack(ba.stackNumber,ba.destinationTile); //move
			sendAndApply(&EndAction());
			break;
		}
	case 3: //defend
	case 8: //wait
		{
			sendAndApply(&StartAction(ba));
			sendAndApply(&EndAction());
			break;
		}
	case 4: //retreat/flee
		{
			if( !gs->battleCanFlee(ba.side ? gs->curB->side2 : gs->curB->side1) )
				break;
			//TODO: remove retreating hero from map and place it in recruitment list
			BattleResult *br = new BattleResult;
			br->result = 1;
			br->winner = !ba.side; //fleeing side loses
			gs->curB->calculateCasualties(br->casualties);
			giveExp(*br);
			battleResult.set(br);
			break;
		}
	case 6: //walk or attack
		{
			sendAndApply(&StartAction(ba)); //start movement and attack
			int startingPos = gs->curB->getStack(ba.stackNumber)->position;
			int distance = moveStack(ba.stackNumber, ba.destinationTile);
			CStack *curStack = gs->curB->getStack(ba.stackNumber),
				*stackAtEnd = gs->curB->getStackT(ba.additionalInfo);

			if(curStack->position != ba.destinationTile //we wasn't able to reach destination tile
				&& !(curStack->doubleWide()
					&&  ( curStack->position == ba.destinationTile + (curStack->attackerOwned ?  +1 : -1 ) )
						) //nor occupy specified hex
				) 
			{
				std::string problem = "We cannot move this stack to its destination " + curStack->type->namePl;
				tlog3 << problem << std::endl;
				complain(problem);
				ok = false;
				sendAndApply(&EndAction());
				break;
			}

			if(curStack->ID == stackAtEnd->ID) //we should just move, it will be handled by following check
			{
				stackAtEnd = NULL;
			}

			if(!stackAtEnd)
			{
				std::ostringstream problem;
				problem << "There is no stack on " << ba.additionalInfo << " tile (no attack)!";
				std::string probl = problem.str();
				tlog3 << probl << std::endl;
				complain(probl);
				ok = false;
				sendAndApply(&EndAction());
				break;
			}

			ui16 curpos = curStack->position, 
				enemypos = stackAtEnd->position;


			if( !(
				(BattleInfo::mutualPosition(curpos, enemypos) >= 0)						//front <=> front
				|| (curStack->doubleWide()									//back <=> front
					&& BattleInfo::mutualPosition(curpos + (curStack->attackerOwned ? -1 : 1), enemypos) >= 0)
				|| (stackAtEnd->doubleWide()									//front <=> back
					&& BattleInfo::mutualPosition(curpos, enemypos + (stackAtEnd->attackerOwned ? -1 : 1)) >= 0)
				|| (stackAtEnd->doubleWide() && curStack->doubleWide()//back <=> back
					&& BattleInfo::mutualPosition(curpos + (curStack->attackerOwned ? -1 : 1), enemypos + (stackAtEnd->attackerOwned ? -1 : 1)) >= 0)
				)
				)
			{
				tlog3 << "Attack cannot be performed!";
				sendAndApply(&EndAction());
				ok = false;
			}

			//attack
			BattleAttack bat;
			prepareAttack(bat, curStack, stackAtEnd, distance);
			sendAndApply(&bat);
			handleAfterAttackCasting(bat);

			//counterattack
			if(!curStack->hasBonusOfType(Bonus::BLOCKS_RETALIATION)
				&& stackAtEnd->alive()
				&& ( stackAtEnd->counterAttacks > 0 || stackAtEnd->hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS) )
				&& !stackAtEnd->hasBonusOfType(Bonus::SIEGE_WEAPON)
				&& !stackAtEnd->hasBonusOfType(Bonus::HYPNOTIZED))
			{
				prepareAttack(bat, stackAtEnd, curStack, 0);
				bat.flags |= 2;
				sendAndApply(&bat);
				handleAfterAttackCasting(bat);
			}

			//second attack
			if(curStack->valOfBonuses(Bonus::ADDITIONAL_ATTACK) > 0
				&& !curStack->hasBonusOfType(Bonus::SHOOTER)
				&& curStack->alive()
				&& stackAtEnd->alive()  )
			{
				bat.flags = 0;
				prepareAttack(bat, curStack, stackAtEnd, 0);
				sendAndApply(&bat);
				handleAfterAttackCasting(bat);
			}

			//return
			if(curStack->hasBonusOfType(Bonus::RETURN_AFTER_STRIKE) && startingPos != curStack->position && curStack->alive())
			{
				moveStack(ba.stackNumber, startingPos);
				//NOTE: curStack->ID == ba.stackNumber (rev 1431)
			}
			sendAndApply(&EndAction());
			break;
		}
	case 7: //shoot
		{
			CStack *curStack = gs->curB->getStack(ba.stackNumber),
				*destStack= gs->curB->getStackT(ba.destinationTile);
			if( !gs->battleCanShoot(ba.stackNumber, ba.destinationTile) )
				break;

			sendAndApply(&StartAction(ba)); //start shooting

			BattleAttack bat;
			bat.flags |= 1;
			prepareAttack(bat, curStack, destStack, 0);
			sendAndApply(&bat);

			if(curStack->valOfBonuses(Bonus::ADDITIONAL_ATTACK) > 0 //if unit shots twice let's make another shot
				&& curStack->alive()
				&& destStack->alive()
				&& curStack->shots
				)
			{
				prepareAttack(bat, curStack, destStack, 0);
				sendAndApply(&bat);
				handleAfterAttackCasting(bat);
			}

			sendAndApply(&EndAction());
			break;
		}
	case 9: //catapult
		{
			sendAndApply(&StartAction(ba));
			const CGHeroInstance * attackingHero = gs->curB->heroes[ba.side];
			CHeroHandler::SBallisticsLevelInfo sbi = VLC->heroh->ballistics[attackingHero->getSecSkillLevel(20)]; //artillery
			
			int attackedPart = gs->curB->hexToWallPart(ba.destinationTile);
			if(attackedPart == -1)
			{
				complain("catapult tried to attack non-catapultable hex!");
				break;
			}
			int wallInitHP = gs->curB->si.wallState[attackedPart];
			int dmgAlreadyDealt = 0; //in successive iterations damage is dealt but not yet substracted from wall's HPs
			for(int g=0; g<sbi.shots; ++g)
			{
				if(wallInitHP + dmgAlreadyDealt == 3) //it's not destroyed
					continue;
				
				CatapultAttack ca; //package for clients
				std::pair< std::pair< ui8, si16 >, ui8> attack; //<< attackedPart , destination tile >, damageDealt >
				attack.first.first = attackedPart;
				attack.first.second = ba.destinationTile;
				attack.second = 0;

				int chanceForHit = 0;
				int dmgChance[3] = {sbi.noDmg, sbi.oneDmg, sbi.twoDmg}; //dmgChance[i] - chance for doing i dmg when hit is successful
				switch(attackedPart)
				{
				case 0: //keep
					chanceForHit = sbi.keep;
					break;
				case 1: //bottom tower
				case 6: //upper tower
					chanceForHit = sbi.tower;
					break;
				case 2: //bottom wall
				case 3: //below gate
				case 4: //over gate
				case 5: //upper wall
					chanceForHit = sbi.wall;
					break;
				case 7: //gate
					chanceForHit = sbi.gate;
					break;
				}

				if(rand()%100 <= chanceForHit) //hit is successful
				{
					int dmgRand = rand()%100;
					//accumulating dmgChance
					dmgChance[1] += dmgChance[0];
					dmgChance[2] += dmgChance[1];
					//calculating dealt damage
					for(int v = 0; v < ARRAY_COUNT(dmgChance); ++v)
					{
						if(dmgRand <= dmgChance[v])
						{
							attack.second = std::min(3 - dmgAlreadyDealt - wallInitHP, v);
							dmgAlreadyDealt += attack.second;
							break;
						}
					}

					//removing creatures in turrets / keep if one is destroyed
					if(attack.second > 0 && (attackedPart == 0 || attackedPart == 1 || attackedPart == 6))
					{
						int posRemove = -1;
						switch(attackedPart)
						{
						case 0: //keep
							posRemove = -2;
							break;
						case 1: //bottom tower
							posRemove = -3;
							break;
						case 6: //upper tower
							posRemove = -4;
							break;
						}

						BattleStacksRemoved bsr;
						for(int g=0; g<gs->curB->stacks.size(); ++g)
						{
							if(gs->curB->stacks[g]->position == posRemove)
							{
								bsr.stackIDs.insert( gs->curB->stacks[g]->ID );
								break;
							}
						}

						sendAndApply(&bsr);
					}
				}
				ca.attacker = ba.stackNumber;
				ca.attackedParts.insert(attack);

				sendAndApply(&ca);
			}
			sendAndApply(&EndAction());
			break;
		}
	case 12: //healing
		{
			static const int healingPerLevel[] = {50, 50, 75, 100};
			sendAndApply(&StartAction(ba));
			const CGHeroInstance * attackingHero = gs->curB->heroes[ba.side];
			CStack *healer = gs->curB->getStack(ba.stackNumber),
				*destStack = gs->curB->getStackT(ba.destinationTile);

			if(healer == NULL || destStack == NULL || !healer->hasBonusOfType(Bonus::HEALER))
			{
				complain("There is either no healer, no destination, or healer cannot heal :P");
			}
			int maxHealable = destStack->MaxHealth() - destStack->firstHPleft;
			int maxiumHeal = healingPerLevel[ attackingHero->getSecSkillLevel(27) ];

			int healed = std::min(maxHealable, maxiumHeal);

			if(healed == 0)
			{
				//nothing to heal.. should we complain?
			}
			else
			{
				StacksHealedOrResurrected shr;
				shr.lifeDrain = false;
				StacksHealedOrResurrected::HealInfo hi;

				hi.healedHP = healed;
				hi.lowLevelResurrection = 0;
				hi.stackID = destStack->ID;

				shr.healedStacks.push_back(hi);
				sendAndApply(&shr);
			}


			sendAndApply(&EndAction());
			break;
		}
	}
	if(ba.stackNumber == gs->curB->activeStack  ||  battleResult.get()) //active stack has moved or battle has finished
		battleMadeAction.setn(true);
	return ok;
}

void CGameHandler::playerMessage( ui8 player, const std::string &message )
{
	bool cheated=true;
	sendAndApply(&PlayerMessage(player,message));
	if(message == "vcmiistari") //give all spells and 999 mana
	{
		SetMana sm;
		ChangeSpells cs;
		SetHeroArtifacts sha;

		CGHeroInstance *h = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!h && complain("Cannot realize cheat, no hero selected!")) return;

		sm.hid = cs.hid = h->id;

		//give all spells
		cs.learn = 1;
		for(int i=0;i<VLC->spellh->spells.size();i++)
		{
			if(!VLC->spellh->spells[i].creatureAbility)
				cs.spells.insert(i);
		}

		//give mana
		sm.val = 999;

		if(!h->getArt(17)) //hero doesn't have spellbook
		{
			//give spellbook
			sha.hid = h->id;
			sha.artifacts = h->artifacts;
			sha.artifWorn = h->artifWorn;
			VLC->arth->equipArtifact(sha.artifWorn, 17, 0);
			sendAndApply(&sha);
		}

		sendAndApply(&cs);
		sendAndApply(&sm);
	}
	else if(message == "vcmiainur") //gives 5 archangels into each slot
	{
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!hero) return;

		SetGarrisons sg;
		CCreatureSet &newArmy = sg.garrs[hero->id];

		newArmy = hero->getArmy();
		for(int i=0; i<ARMY_SIZE; i++)
			if(newArmy.slotEmpty(i))
				newArmy.addToSlot(i, CStackInstance(13,5));
		sendAndApply(&sg);
	}
	else if(message == "vcmiangband") //gives 10 black knight into each slot
	{
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!hero) return;

		SetGarrisons sg;
		CCreatureSet &newArmy = sg.garrs[hero->id];

		newArmy = hero->getArmy();
		for(int i=0; i<ARMY_SIZE; i++)
			if(newArmy.slotEmpty(i))
				newArmy.addToSlot(i, CStackInstance(66,10));
		sendAndApply(&sg);
	}
	else if(message == "vcminoldor") //all war machines
	{
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!hero) return;
		SetHeroArtifacts sha;
		sha.hid = hero->id;
		sha.artifacts = hero->artifacts;
		sha.artifWorn = hero->artifWorn;
		VLC->arth->equipArtifact(sha.artifWorn, 13, 4);
		VLC->arth->equipArtifact(sha.artifWorn, 14, 5);
		VLC->arth->equipArtifact(sha.artifWorn, 15, 6);
		sendAndApply(&sha);
	}
	else if(message == "vcminahar") //1000000 movement points
	{
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
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
		for(int i=0;i<7;i++)
			sr.res[i] += 100;
		sr.res[6] += 19900;
		sendAndApply(&sr);
	}
	else if(message == "vcmieagles") //reveal FoW
	{
		FoWChange fc;
		fc.mode = 1;
		fc.player = player;
		for(int i=0;i<gs->map->width;i++)
			for(int j=0;j<gs->map->height;j++)
				for(int k=0;k<gs->map->twoLevel+1;k++)
					if(!gs->getPlayer(fc.player)->fogOfWarMap[i][j][k])
						fc.tiles.insert(int3(i,j,k));
		sendAndApply(&fc);
	}
	else if(message == "vcmiglorfindel")
	{
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		changePrimSkill(hero->id,4,VLC->heroh->reqExp(hero->level+1) - VLC->heroh->reqExp(hero->level));
	}
	else
		cheated = false;
	if(cheated)
	{
		sendAndApply(&SystemMessage(VLC->generaltexth->allTexts[260]));
	}
}

static ui32 calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack)
{
	switch(spell->id)
	{
	case 37: //cure
		{
			int healedHealth = caster->getPrimSkillLevel(2) * 5 + spell->powers[caster->getSpellSchoolLevel(spell)];
			return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft);
			break;
		}
	case 38: //resurrection
		{
			int healedHealth = caster->getPrimSkillLevel(2) * 50 + spell->powers[caster->getSpellSchoolLevel(spell)];
			return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + stack->baseAmount * stack->MaxHealth());
			break;
		}
	case 39: //animate dead
		{
			int healedHealth = caster->getPrimSkillLevel(2) * 50 + spell->powers[caster->getSpellSchoolLevel(spell)];
			return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + stack->baseAmount * stack->MaxHealth());
			break;
		}
	}
	//we shouldn't be here
	tlog1 << "calculateHealedHP called for non-healing spell: " << spell->name << std::endl;
	return 0;
}

static std::vector<ui32> calculateResistedStacks(const CSpell * sp, const CGHeroInstance * caster, const CGHeroInstance * hero2, const std::set<CStack*> affectedCreatures)
{
	std::vector<ui32> ret;
	for(std::set<CStack*>::const_iterator it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
	{
		if (NBonus::hasOfType(caster, Bonus::NEGATE_ALL_NATURAL_IMMUNITIES) ||
			NBonus::hasOfType(hero2, Bonus::NEGATE_ALL_NATURAL_IMMUNITIES))
		{
			//don't use natural immunities when one of heroes has this bonus
 			BonusList bl = (*it)->getBonuses(Selector::type(Bonus::SPELL_IMMUNITY)),
				b2 = (*it)->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));

			bl.insert(bl.end(), b2.begin(), b2.end());
 
 			BOOST_FOREACH(Bonus bb, bl)
 			{
 				if( (bb.type == Bonus::SPELL_IMMUNITY && bb.subtype == sp->id || //100% sure spell immunity
 					bb.type == Bonus::LEVEL_SPELL_IMMUNITY && bb.val >= sp->level) //some creature abilities have level 0
					&& bb.source != Bonus::CREATURE_ABILITY)
 				{
 					ret.push_back((*it)->ID);
 					continue;
 				}
 			}
		}
		else
		{
			if ((*it)->hasBonusOfType(Bonus::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
				|| ( (*it)->hasBonusOfType(Bonus::LEVEL_SPELL_IMMUNITY) &&
				(*it)->valOfBonuses(Bonus::LEVEL_SPELL_IMMUNITY) >= sp->level) ) //some creature abilities have level 0
			{
				ret.push_back((*it)->ID);
				continue;
			}
		}
		

		//non-negative spells on friendly stacks should always succeed, unless immune
		if(sp->positiveness >= 0 && (*it)->owner == caster->tempOwner)
			continue;

		const CGHeroInstance * bonusHero; //hero we should take bonuses from
		if(caster && (*it)->owner == caster->tempOwner)
			bonusHero = caster;
		else
			bonusHero = hero2;

		int prob = (*it)->valOfBonuses(Bonus::MAGIC_RESISTANCE); //probability of resistance in %
		if(bonusHero)
		{
			//bonusHero's resistance support (secondary skils and artifacts)
			prob += bonusHero->valOfBonuses(Bonus::MAGIC_RESISTANCE);

			switch(bonusHero->getSecSkillLevel(26)) //resistance
			{
			case 1: //basic
				prob += 5;
				break;
			case 2: //advanced
				prob += 10;
				break;
			case 3: //expert
				prob += 20;
				break;
			}
		}

		if(prob > 100) prob = 100;

		if(rand()%100 < prob) //immunity from resistance
			ret.push_back((*it)->ID);

	}

	if(sp->id == 60) //hypnotize
	{
		for(std::set<CStack*>::const_iterator it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
		{
			if( (*it)->hasBonusOfType(Bonus::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
				|| ( (*it)->count - 1 ) * (*it)->MaxHealth() + (*it)->firstHPleft 
				> 
				caster->getPrimSkillLevel(2) * 25 + sp->powers[caster->getSpellSchoolLevel(sp)]
			)
			{
				ret.push_back((*it)->ID);
			}
		}
	}

	return ret;
}

void CGameHandler::handleSpellCasting( int spellID, int spellLvl, int destination, ui8 casterSide, ui8 casterColor,
	const CGHeroInstance * caster, const CGHeroInstance * secHero, int usedSpellPower )
{
	CSpell *spell = &VLC->spellh->spells[spellID];

	BattleSpellCast sc;
	sc.side = casterSide;
	sc.id = spellID;
	sc.skill = spellLvl;
	sc.tile = destination;
	sc.dmgToDisplay = 0;
	sc.castedByHero = (bool)caster;

	//calculating affected creatures for all spells
	std::set<CStack*> attackedCres = gs->curB->getAttackedCreatures(spell, spellLvl, casterColor, destination);
	for(std::set<CStack*>::const_iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
	{
		sc.affectedCres.insert((*it)->ID);
	}

	//checking if creatures resist
	sc.resisted = calculateResistedStacks(spell, caster, secHero, attackedCres);

	//calculating dmg to display
	for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
	{
		if(vstd::contains(sc.resisted, (*it)->ID)) //this creature resisted the spell
			continue;
		sc.dmgToDisplay += gs->curB->calculateSpellDmg(spell, caster, *it, spellLvl, usedSpellPower);
	}

	sendAndApply(&sc);

	//applying effects
	switch(spellID)
	{
	case 15: //magic arrow
	case 16: //ice bolt
	case 17: //lightning bolt
	case 18: //implosion
	case 20: //frost ring
	case 21: //fireball
	case 22: //inferno
	case 23: //meteor shower
	case 24: //death ripple
	case 25: //destroy undead
	case 26: //armageddon
	case 77: //Thunderbolt (thunderbirds)
		{
			StacksInjured si;
			for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
			{
				if(vstd::contains(sc.resisted, (*it)->ID)) //this creature resisted the spell
					continue;

				BattleStackAttacked bsa;
				bsa.flags |= 2;
				bsa.effect = spell->mainEffectAnim;
				bsa.damageAmount = gs->curB->calculateSpellDmg(spell, caster, *it, spellLvl, usedSpellPower);
				bsa.stackAttacked = (*it)->ID;
				bsa.attackerID = -1;
				prepareAttacked(bsa,*it);
				si.stacks.push_back(bsa);
			}
			if(!si.stacks.empty())
				sendAndApply(&si);
			break;
		}
	case 27: //shield 
	case 28: //air shield
	case 29: //fire shield
	case 30: //protection from air
	case 31: //protection from fire
	case 32: //protection from water
	case 33: //protection from earth
	case 34: //anti-magic
	case 41: //bless
	case 42: //curse
	case 43: //bloodlust
	case 44: //precision
	case 45: //weakness
	case 46: //stone skin
	case 47: //disrupting ray
	case 48: //prayer
	case 49: //mirth
	case 50: //sorrow
	case 51: //fortune
	case 52: //misfortune
	case 53: //haste
	case 54: //slow
	case 55: //slayer
	case 56: //frenzy
	case 58: //counterstrike
	case 59: //berserk
	case 60: //hypnotize
	case 61: //forgetfulness
	case 62: //blind
		{
			SetStackEffect sse;
			for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
			{
				if(vstd::contains(sc.resisted, (*it)->ID)) //this creature resisted the spell
					continue;
				sse.stacks.push_back((*it)->ID);
			}
			sse.effect.id = spellID;
			sse.effect.level = spellLvl;
			sse.effect.turnsRemain = BattleInfo::calculateSpellDuration(spell, caster, usedSpellPower);
			if(!sse.stacks.empty())
				sendAndApply(&sse);
			break;
		}
	case 63: //teleport
		{
			BattleStackMoved bsm;
			bsm.distance = -1;
			bsm.stack = gs->curB->activeStack;
			bsm.ending = true;
			bsm.tile = destination;
			bsm.teleporting = true;
			sendAndApply(&bsm);

			break;
		}
	case 37: //cure
	case 38: //resurrection
	case 39: //animate dead
		{
			StacksHealedOrResurrected shr;
			shr.lifeDrain = false;
			for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
			{
				if(vstd::contains(sc.resisted, (*it)->ID) //this creature resisted the spell
					|| (spellID == 39 && !(*it)->hasBonusOfType(Bonus::UNDEAD)) //we try to cast animate dead on living stack
					) 
					continue;
				StacksHealedOrResurrected::HealInfo hi;
				hi.stackID = (*it)->ID;
				hi.healedHP = calculateHealedHP(caster, spell, *it);
				hi.lowLevelResurrection = spellLvl <= 1;
				shr.healedStacks.push_back(hi);
			}
			if(!shr.healedStacks.empty())
				sendAndApply(&shr);
			break;
		}
	case 64: //remove obstacle
		{
			ObstaclesRemoved obr;
			for(int g=0; g<gs->curB->obstacles.size(); ++g)
			{
				std::vector<int> blockedHexes = VLC->heroh->obstacles[gs->curB->obstacles[g].ID].getBlocked(gs->curB->obstacles[g].pos);

				if(vstd::contains(blockedHexes, destination)) //this obstacle covers given hex
				{
					obr.obstacles.insert(gs->curB->obstacles[g].uniqueID);
				}
			}
			if(!obr.obstacles.empty())
				sendAndApply(&obr);

			break;
		}
	}

}

bool CGameHandler::makeCustomAction( BattleAction &ba )
{
	switch(ba.actionType)
	{
	case 1: //hero casts spell
		{
			const CGHeroInstance *h = gs->curB->heroes[ba.side];
			const CGHeroInstance *secondHero = gs->curB->heroes[!ba.side];
			if(!h)
			{
				tlog2 << "Wrong caster!\n";
				return false;
			}
			if(ba.additionalInfo >= VLC->spellh->spells.size())
			{
				tlog2 << "Wrong spell id (" << ba.additionalInfo << ")!\n";
				return false;
			}

			const CSpell *s = &VLC->spellh->spells[ba.additionalInfo];
			ui8 skill = h->getSpellSchoolLevel(s); //skill level

			if(   !(h->canCastThisSpell(s)) //hero cannot cast this spell at all
				|| (h->mana < gs->curB->getSpellCost(s, h)) //not enough mana
				|| (ba.additionalInfo < 10) //it's adventure spell (not combat)
				|| (gs->curB->castSpells[ba.side]) //spell has been cast
				|| (NBonus::hasOfType(secondHero, Bonus::SPELL_IMMUNITY, s->id)) //non - casting hero provides immunity for this spell 
				|| (gs->battleMaxSpellLevel() < s->level) //non - casting hero stops caster from casting this spell
				)
			{
				tlog2 << "Spell cannot be cast!\n";
				return false;
			}

			sendAndApply(&StartAction(ba)); //start spell casting

			handleSpellCasting(ba.additionalInfo, skill, ba.destinationTile, ba.side, h->tempOwner, h, secondHero, h->getPrimSkillLevel(2));

			sendAndApply(&EndAction());
			if( !gs->curB->getStack(gs->curB->activeStack, false)->alive() )
			{
				battleMadeAction.setn(true);
			}
			checkForBattleEnd(gs->curB->stacks);
			if(battleResult.get())
			{
				endBattle(gs->curB->tile, gs->curB->heroes[0], gs->curB->heroes[1]);
			}

			return true;
		}
	}
	return false;
}

void CGameHandler::handleTimeEvents()
{
	gs->map->events.sort(evntCmp);
	while(gs->map->events.size() && gs->map->events.front()->firstOccurence+1 == gs->day)
	{
		CMapEvent *ev = gs->map->events.front();
		for(int player = 0; player < PLAYER_LIMIT; player++)
		{
			PlayerState *pinfo = gs->getPlayer(player);

			if( pinfo  //player exists
				&& (ev->players & 1<<player) //event is enabled to this player
				&& ((ev->computerAffected && !pinfo->human) 
					|| (ev->humanAffected && pinfo->human)
				)
			)
			{
				//give resources
				SetResources sr;
				sr.player = player;
				sr.res = pinfo->resources;

				//prepare dialog
				InfoWindow iw;
				iw.player = player;
				iw.text << ev->message;
				for (int i=0; i<ev->resources.size(); i++)
				{
					if(ev->resources[i]) //if resource is changed, we add it to the dialog
					{
						// If removing too much resources, adjust the
						// amount so the total doesn't become negative.
						if (sr.res[i] + ev->resources[i] < 0)
							ev->resources[i] = -sr.res[i];

						if(ev->resources[i]) //if non-zero res change
						{
							iw.components.push_back(Component(Component::RESOURCE,i,ev->resources[i],0));
							sr.res[i] += ev->resources[i];
						}
					}
				}
				if (iw.components.size())
				{
					sendAndApply(&sr); //update player resources if changed
				}

				sendAndApply(&iw); //show dialog
			}
		} //PLAYERS LOOP

		if(ev->nextOccurence)
		{
			ev->firstOccurence += ev->nextOccurence;
			gs->map->events.sort(evntCmp);
		}
		else
		{
			delete ev;
			gs->map->events.pop_front();
		}
	}
}

bool CGameHandler::complain( const std::string &problem )
{
	sendMessageToAll("Server encountered a problem: " + problem);
	tlog1 << problem << std::endl;
	return true;
}

ui32 CGameHandler::getQueryResult( ui8 player, int queryID )
{
	//TODO: write
	return 0;
}

void CGameHandler::showGarrisonDialog( int upobj, int hid, bool removableUnits, const boost::function<void()> &cb )
{
	ui8 player = getOwner(hid);
	GarrisonDialog gd;
	gd.hid = hid;
	gd.objid = upobj;

	{
		boost::unique_lock<boost::recursive_mutex> lock(gsm);
		gd.id = QID;
		garrisonCallbacks[QID] = cb;
		allowedExchanges[QID] = std::pair<si32,si32>(upobj,hid);
		states.addQuery(player,QID);
		QID++; 
		gd.removableUnits = removableUnits;
		sendAndApply(&gd);
	}
}

void CGameHandler::showThievesGuildWindow(int requestingObjId)
{
	OpenWindow ow;
	ow.window = OpenWindow::THIEVES_GUILD;
	ow.id1 = requestingObjId;
	sendAndApply(&ow);
}

bool CGameHandler::isAllowedExchange( int id1, int id2 )
{
	if(id1 == id2)
		return true;

	{
		boost::unique_lock<boost::recursive_mutex> lock(gsm);
		for(std::map<ui32, std::pair<si32,si32> >::const_iterator i = allowedExchanges.begin(); i!=allowedExchanges.end(); i++)
			if(id1 == i->second.first && id2 == i->second.second   ||   id2 == i->second.first && id1 == i->second.second)
				return true;
	}

	const CGObjectInstance *o1 = getObj(id1), *o2 = getObj(id2);

	if(o1->ID == TOWNI_TYPE)
	{
		const CGTownInstance *t = static_cast<const CGTownInstance*>(o1);
		if(t->visitingHero == o2  ||  t->garrisonHero == o2)
			return true;
	}
	if(o2->ID == TOWNI_TYPE)
	{
		const CGTownInstance *t = static_cast<const CGTownInstance*>(o2);
		if(t->visitingHero == o1  ||  t->garrisonHero == o1)
			return true;
	}
	if(o1->ID == HEROI_TYPE && o2->ID == HEROI_TYPE
		&& distance(o1->pos, o2->pos) < 2) //hero stands on the same tile or on the neighbouring tiles
	{
		//TODO: it's workaround, we should check if first hero visited second and player hasn't closed exchange window
		//(to block moving stacks for free [without visiting] beteen heroes)
		return true;
	}

	return false;
}

void CGameHandler::objectVisited( const CGObjectInstance * obj, const CGHeroInstance * h )
{
	obj->onHeroVisit(h);
}

bool CGameHandler::buildBoat( ui32 objid )
{
	const IShipyard *obj = IShipyard::castFrom(getObj(objid));
	int boatType = 1; 

	if(obj->state())
	{
		complain("Cannot build boat in this shipyard!");
		return false;
	}
	else if(obj->o->ID == TOWNI_TYPE
		&& !vstd::contains((static_cast<const CGTownInstance*>(obj))->builtBuildings,6))
	{
		complain("Cannot build boat in the town - no shipyard!");
		return false;
	}

	//TODO use "real" cost via obj->getBoatCost
	if(getResource(obj->o->tempOwner, 6) < 1000  ||  getResource(obj->o->tempOwner, 0) < 10)
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
	sr.player = obj->o->tempOwner;
	sr.res = gs->getPlayer(obj->o->tempOwner)->resources;
	sr.res[0] -= 10;
	sr.res[6] -= 1000;
	sendAndApply(&sr);

	//create boat
	NewObject no;
	no.ID = 8;
	no.subID = obj->getBoatType();
	no.pos = tile + int3(1,0,0);
	sendAndApply(&no);

	return true;
}

void CGameHandler::engageIntoBattle( ui8 player )
{
	if(vstd::contains(states.players, player))
		states.setFlag(player,&PlayerStatus::engagedIntoBattle,true);

	//notify interfaces
	PlayerBlocked pb;
	pb.player = player;
	pb.reason = PlayerBlocked::UPCOMING_BATTLE;
	sendAndApply(&pb);
}

void CGameHandler::winLoseHandle(ui8 players )
{
	for(size_t i = 0; i < PLAYER_LIMIT; i++)
	{
		if(players & 1<<i  &&  gs->getPlayer(i))
		{
			checkLossVictory(i);
		}
	}
}

void CGameHandler::checkLossVictory( ui8 player )
{
	const PlayerState *p = gs->getPlayer(player);
	if(p->status) //player already won / lost
		return;

	int loss = gs->lossCheck(player);
	int vic = gs->victoryCheck(player);

	if(!loss && !vic)
		return;

	InfoWindow iw;
	getLossVicMessage(player, vic ? vic : loss , vic, iw);
	sendAndApply(&iw);

	PlayerEndsGame peg;
	peg.player = player;
	peg.victory = vic;
	sendAndApply(&peg);

	if(vic > 0) //one player won -> all enemies lost  //TODO: allies
	{
		iw.text.localStrings.front().second++; //message about losing because enemy won first is just after victory message

		for (std::map<ui8,PlayerState>::const_iterator i = gs->players.begin(); i!=gs->players.end(); i++)
		{
			if(i->first < PLAYER_LIMIT && i->first != player)
			{
				iw.player = i->first;
				sendAndApply(&iw);

				peg.player = i->first;
				peg.victory = false;
				sendAndApply(&peg);
			}
		}
	}
	else //player lost -> all his objects become unflagged (neutral)
	{
		std::vector<CGHeroInstance*> hlp = p->heroes;
		for (std::vector<CGHeroInstance*>::const_iterator i = hlp.begin(); i != hlp.end(); i++) //eliminate heroes
			removeObject((*i)->id);

		for (std::vector<CGObjectInstance*>::const_iterator i = gs->map->objects.begin(); i != gs->map->objects.end(); i++) //unflag objs
		{
			if(*i  &&  (*i)->tempOwner == player)
				setOwner((**i).id,NEUTRAL_PLAYER);
		}

		//eliminating one player may cause victory of another:
		winLoseHandle(ALL_PLAYERS & ~(1<<player));
	}

	if(vic)
		end2 = true;
}

void CGameHandler::getLossVicMessage( ui8 player, ui8 standard, bool victory, InfoWindow &out ) const
{
	const PlayerState *p = gs->getPlayer(player);
// 	if(!p->human)
// 		return; //AI doesn't need text info of loss

	out.player = player;

	if(victory)
	{
		if(standard < 0) //not std loss
		{
			switch(gs->map->victoryCondition.condition)
			{
			case artifact:
				out.text.addTxt(MetaString::GENERAL_TXT, 280); //Congratulations! You have found the %s, and can claim victory!
				out.text.addReplacement(MetaString::ART_NAMES,gs->map->victoryCondition.ID); //artifact name
				break;
			case gatherTroop:
				out.text.addTxt(MetaString::GENERAL_TXT, 276); //Congratulations! You have over %d %s in your armies. Your enemies have no choice but to bow down before your power!
				out.text.addReplacement(gs->map->victoryCondition.count);
				out.text.addReplacement(MetaString::CRE_PL_NAMES, gs->map->victoryCondition.ID);
				break;
			case gatherResource:
				out.text.addTxt(MetaString::GENERAL_TXT, 278); //Congratulations! You have collected over %d %s in your treasury. Victory is yours!
				out.text.addReplacement(gs->map->victoryCondition.count);
				out.text.addReplacement(MetaString::RES_NAMES, gs->map->victoryCondition.ID);
				break;
			case buildCity:
				out.text.addTxt(MetaString::GENERAL_TXT, 282); //Congratulations! You have successfully upgraded your town, and can claim victory!
				break;
			case buildGrail:
				out.text.addTxt(MetaString::GENERAL_TXT, 284); //Congratulations! You have constructed a permanent home for the Grail, and can claim victory!
				break;
			case beatHero:
				{
					out.text.addTxt(MetaString::GENERAL_TXT, 252); //Congratulations! You have completed your quest to defeat the enemy hero %s. Victory is yours!
					const CGHeroInstance *h = dynamic_cast<const CGHeroInstance*>(gs->map->victoryCondition.obj);
					assert(h);
					out.text.addReplacement(h->name);
				}
				break;
			case captureCity:
				{
					out.text.addTxt(MetaString::GENERAL_TXT, 249); //Congratulations! You captured %s, and are victorious!
					const CGTownInstance *t = dynamic_cast<const CGTownInstance*>(gs->map->victoryCondition.obj);
					assert(t);
					out.text.addReplacement(t->name);
				}
				break;
			case beatMonster:
				out.text.addTxt(MetaString::GENERAL_TXT, 286); //Congratulations! You have completed your quest to kill the fearsome beast, and can claim victory!
				break;
			case takeDwellings:
				out.text.addTxt(MetaString::GENERAL_TXT, 288); //Congratulations! Your flag flies on the dwelling of every creature. Victory is yours!
				break;
			case takeMines:
				out.text.addTxt(MetaString::GENERAL_TXT, 290); //Congratulations! Your flag flies on every mine. Victory is yours!
				break;
			case transportItem:
				out.text.addTxt(MetaString::GENERAL_TXT, 292); //Congratulations! You have reached your destination, precious cargo intact, and can claim victory!
				break;
			}
		}
		else
		{
			out.text.addTxt(MetaString::GENERAL_TXT, 659); //Congratulations! You have reached your destination, precious cargo intact, and can claim victory!
		}
	}
	else
	{
		if(standard < 0) //not std loss
		{
			switch(gs->map->lossCondition.typeOfLossCon)
			{
			case lossCastle:
				{
					out.text.addTxt(MetaString::GENERAL_TXT, 251); //The town of %s has fallen - all is lost!
					const CGTownInstance *t = dynamic_cast<const CGTownInstance*>(gs->map->lossCondition.obj);
					assert(t);
					out.text.addReplacement(t->name);
				}
				break;
			case lossHero:
				{
					out.text.addTxt(MetaString::GENERAL_TXT, 253); //The hero, %s, has suffered defeat - your quest is over!
					const CGHeroInstance *h = dynamic_cast<const CGHeroInstance*>(gs->map->lossCondition.obj);
					assert(h);
					out.text.addReplacement(h->name);
				}
				break;
			case timeExpires:
				out.text.addTxt(MetaString::GENERAL_TXT, 254); //Alas, time has run out on your quest. All is lost.
				break;
			}
		}
		else if(standard == 2)
		{
			out.text.addTxt(MetaString::GENERAL_TXT, 7);//%s, your heroes abandon you, and you are banished from this land.
			out.text.addReplacement(MetaString::COLOR, player);
			out.components.push_back(Component(Component::FLAG,player,0,0));
		}
		else //lost all towns and heroes
		{
			out.text.addTxt(MetaString::GENERAL_TXT, 660); //All your forces have been defeated, and you are banished from this land!
		}
	}
}

bool CGameHandler::dig( const CGHeroInstance *h )
{
	for (std::vector<CGObjectInstance*>::const_iterator i = gs->map->objects.begin(); i != gs->map->objects.end(); i++) //unflag objs
	{
		if(*i && (*i)->ID == 124  &&  (*i)->pos == h->getPosition())
		{
			complain("Cannot dig - there is already a hole under the hero!");
			return false;
		}
	}

	NewObject no;
	no.ID = 124;
	no.pos = h->getPosition();
	no.subID = getTile(no.pos)->tertype;

	if(no.subID >= 8) //no digging on water / rock
	{
		complain("Cannot dig - wrong terrain type!");
		return false;
	}
	sendAndApply(&no);

	SetMovePoints smp;
	smp.hid = h->id;
	smp.val = 0;
	sendAndApply(&smp);

	InfoWindow iw;
	iw.player = h->tempOwner;
	if(gs->map->grailPos == h->getPosition())
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 58); //"Congratulations! After spending many hours digging here, your hero has uncovered the "
		iw.text.addTxt(MetaString::ART_NAMES, 2);
		iw.soundID = soundBase::ULTIMATEARTIFACT;
		giveHeroArtifact(2, h->id, -1); //give grail
		sendAndApply(&iw);

		iw.text.clear();
		iw.text.addTxt(MetaString::ART_DESCR, 2);
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

void CGameHandler::handleAfterAttackCasting( const BattleAttack & bat )
{
	const CStack * attacker = gs->curB->getStack(bat.stackAttacking);
	if( attacker->hasBonusOfType(Bonus::SPELL_AFTER_ATTACK) )
	{
		BOOST_FOREACH(const Bonus & sf, attacker->getBonuses(Selector::type(Bonus::SPELL_AFTER_ATTACK)))
		{
			if (sf.type == Bonus::SPELL_AFTER_ATTACK)
			{
				const CStack * oneOfAttacked = NULL;
				for(int g=0; g<bat.bsa.size(); ++g)
				{
					if (bat.bsa[g].newAmount > 0)
					{
						oneOfAttacked = gs->curB->getStack(bat.bsa[g].stackAttacked);
						break;
					}
				}
				if(oneOfAttacked == NULL) //all attacked creatures have been killed
					return;

				int spellID = sf.subtype;
				int spellLevel = sf.val;
				int chance = sf.additionalInfo % 1000;
				int meleeRanged = sf.additionalInfo / 1000;
				int destination = oneOfAttacked->position;
				//check if spell should be casted (probability handling)
				if( rand()%100 >= chance )
					continue;

				//casting
				handleSpellCasting(spellID, spellLevel, destination, !attacker->attackerOwned, attacker->owner, NULL, NULL, attacker->count);
			}
		}
	}
}

bool CGameHandler::castSpell(const CGHeroInstance *h, int spellID, const int3 &pos)
{
	const CSpell *s = &VLC->spellh->spells[spellID];
	int cost = h->getSpellCost(s);
	int schoolLevel = h->getSpellSchoolLevel(s);

	if(!h->canCastThisSpell(s))
		COMPLAIN_RET("Hero cannot cast this spell!");
	if(h->mana < cost) 
		COMPLAIN_RET("Hero doesn't have enough spell points to cast this spell!");
	if(s->combatSpell)
		COMPLAIN_RET("This function can be used only for adventure map spells!");

	AdvmapSpellCast asc;
	asc.caster = h;
	asc.spellID = spellID;
	sendAndApply(&asc);

	using namespace Spells;
	switch(spellID)
	{
	case SUMMON_BOAT: //Summon Boat 
		{
			//check if spell works at all
			if(rand() % 100 >= s->powers[schoolLevel]) //power is % chance of success
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 336); //%s tried to summon a boat, but failed.
				iw.text.addReplacement(h->name);
				sendAndApply(&iw);
				return true; //TODO? or should it be false? request was correct and realized, but spell failed...
			}

			//try to find unoccupied boat to summon
			const CGBoat *nearest = NULL;
			double dist = 0;
			int3 summonPos = h->bestLocation();
			if(summonPos.x < 0)
				COMPLAIN_RET("There is no water tile available!");

			BOOST_FOREACH(const CGObjectInstance *obj, gs->map->objects)
			{
				if(obj && obj->ID == 8)
				{
					const CGBoat *b = static_cast<const CGBoat*>(obj);
					if(b->hero) continue; //we're looking for unoccupied boat

					double nDist = distance(b->pos, h->getPosition());
					if(!nearest || nDist < dist) //it's first boat or closer than previous
					{
						nearest = b;
						dist = nDist;
					}
				}
			}

			if(nearest) //we found boat to summon
			{
				ChangeObjPos cop;
				cop.objid = nearest->id;
				cop.nPos = summonPos + int3(1,0,0);;
				cop.flags = 1;
				sendAndApply(&cop);
			}
			else if(schoolLevel < 2) //none or basic level -> cannot create boat :(
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 335); //There are no boats to summon.
				sendAndApply(&iw);
			}
			else //create boat
			{
				NewObject no;
				no.ID = 8;
				no.subID = h->getBoatType();
				no.pos = summonPos + int3(1,0,0);;
				sendAndApply(&no);
			}
			break;
		}

	case SCUTTLE_BOAT: //Scuttle Boat 
		{
			//check if spell works at all
			if(rand() % 100 >= s->powers[schoolLevel]) //power is % chance of success
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 337); //%s tried to scuttle the boat, but failed
				iw.text.addReplacement(h->name);
				sendAndApply(&iw);
				return true; //TODO? or should it be false? request was correct and realized, but spell failed...
			}
			if(!gs->map->isInTheMap(pos))
				COMPLAIN_RET("Invalid dst tile for scuttle!");

			//TODO: test range, visibility
			const TerrainTile *t = &gs->map->getTile(pos);
			if(!t->visitableObjects.size() || t->visitableObjects.back()->ID != 8)
				COMPLAIN_RET("There is no boat to scuttle!");

			RemoveObject ro;
			ro.id = t->visitableObjects.back()->id;
			sendAndApply(&ro);
			break;
		}
	case DIMENSION_DOOR: //Dimension Door
		{
			const TerrainTile *dest = getTile(pos);
			const TerrainTile *curr = getTile(h->getSightCenter());

			if(!dest)
				COMPLAIN_RET("Destination tile doesn't exist!");
			if(!h->movement)
				COMPLAIN_RET("Hero needs movement points to cast Dimension Door!");
			if(h->getBonusesCount(Bonus::CASTED_SPELL, Spells::DIMENSION_DOOR) >= s->powers[schoolLevel]) //limit casts per turn
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 338); //%s is not skilled enough to cast this spell again today.
				iw.text.addReplacement(h->name);
				sendAndApply(&iw);
				break;
			}

			GiveBonus gb;
			gb.id = h->id;
			gb.bonus = Bonus(Bonus::ONE_DAY, Bonus::NONE, Bonus::CASTED_SPELL, 0, Spells::DIMENSION_DOOR);
			sendAndApply(&gb);
			
			if(!dest->isClear(curr)) //wrong dest tile
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 70); //Dimension Door failed!
				sendAndApply(&iw);
				break;
			}

			//we need obtain guard pos before moving hero, otherwise we get nothing, because tile will be "unguarded" by hero
			int3 guardPos = gs->guardingCreaturePosition(pos);

			TryMoveHero tmh;
			tmh.id = h->id;
			tmh.movePoints = std::max<int>(0, h->movement - 300);
			tmh.result = TryMoveHero::TELEPORTATION;
			tmh.start = h->pos;
			tmh.end = pos + h->getVisitableOffset();
			getTilesInRange(tmh.fowRevealed, pos, h->getSightRadious(), h->tempOwner,1);
			sendAndApply(&tmh);

			tryAttackingGuard(guardPos, h);
		}
		break;
	case FLY: //Fly 
		{
			int subtype = schoolLevel >= 2 ? 1 : 2; //adv or expert

			GiveBonus gb;
			gb.id = h->id;
			gb.bonus = Bonus(Bonus::ONE_DAY, Bonus::FLYING_MOVEMENT, Bonus::CASTED_SPELL, 0, Spells::FLY, subtype);
			sendAndApply(&gb);
		}
		break;
	case WATER_WALK: //Water Walk 
		{
			int subtype = schoolLevel >= 2 ? 1 : 2; //adv or expert

			GiveBonus gb;
			gb.id = h->id;
			gb.bonus = Bonus(Bonus::ONE_DAY, Bonus::WATER_WALKING, Bonus::CASTED_SPELL, 0, Spells::FLY, subtype);
			sendAndApply(&gb);
		}
		break;
	case VISIONS: //Visions 
	case VIEW_EARTH: //View Earth 
	case DISGUISE: //Disguise 
	case VIEW_AIR: //View Air 
	case TOWN_PORTAL: //Town Portal 
	default:
		COMPLAIN_RET("This spell is not implemented yet!");
	}

	SetMana sm;
	sm.hid = h->id;
	sm.val = h->mana - cost;
	sendAndApply(&sm);

	return true;
}

void CGameHandler::visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h)
{
	//to prevent self-visiting heroes on space press
	if(t.visitableObjects.back() != h)
		objectVisited(t.visitableObjects.back(), h);
	else if(t.visitableObjects.size() > 1)
		objectVisited(*(t.visitableObjects.end()-2),h);
}

bool CGameHandler::tryAttackingGuard(const int3 &guardPos, const CGHeroInstance * h)
{
	if(!gs->map->isInTheMap(guardPos))
		return false;

	const TerrainTile &guardTile = gs->map->terrain[guardPos.x][guardPos.y][guardPos.z];
	objectVisited(guardTile.visitableObjects.back(), h);
	visitObjectAfterVictory = true;
	return true;
}