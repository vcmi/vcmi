#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
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
		r.exp[r.winner] += VLC->creh->creatures[i->first].hitPoints * i->second;
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
		if(bat->stacks[i]->hasFeatureOfType(StackFeature::SUMMONED)) //don't take into account sumoned stacks
			continue;

		CStack *st = bat->stacks[i];
		if(st->owner==color && vstd::contains(set.slots,st->slot) && st->amount < set.slots.find(st->slot)->second.second)
		{
			if(st->alive())
				ret.slots[st->slot].second = st->amount;
			else
				ret.slots.erase(st->slot);
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
		setupBattle(curB, tile, army1->army, army2->army, hero1, hero2, creatureBank, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces
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
			if(next->Morale() < 0 &&
				!((hero1->hasBonusOfType(HeroBonus::BLOCK_MORALE)) || (hero2->hasBonusOfType(HeroBonus::BLOCK_MORALE))) //checking if heroes have (or don't have) morale blocking bonuses)
				)
			{
				if( rand()%24   <   (-next->Morale())*2 )
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

			if(next->hasFeatureOfType(StackFeature::ATTACKS_NEAREST_CREATURE)) //while in berserk
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

			if(next->position < 0 && (!curOwner || curOwner->getSecSkillLevel(10) == 0)) //arrow turret, hero has no ballistics
			{
				BattleAction attack;
				attack.actionType = 7;
				attack.side = !next->attackerOwned;
				attack.stackNumber = next->ID;

				for(int g=0; g<gs->curB->stacks.size(); ++g)
				{
					if(gs->curB->stacks[g]->attackerOwned && gs->curB->stacks[g]->alive())
					{
						attack.destinationTile = gs->curB->stacks[g]->position;
						break;
					}
				}

				makeBattleAction(attack);

				checkForBattleEnd(stacks);
				continue;
			}

			if(next->creature->idNumber == 145 && (!curOwner || curOwner->getSecSkillLevel(10) == 0)) //catapult, hero has no ballistics
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
			if(!vstd::contains(next->state,HAD_MORALE)  //only one extra move per turn possible
				&& !vstd::contains(next->state,DEFENDING)
				&& !vstd::contains(next->state,WAITING)
				&&  next->alive()
				&&  next->Morale() > 0
				&& !((hero1->hasBonusOfType(HeroBonus::BLOCK_MORALE)) || (hero2->hasBonusOfType(HeroBonus::BLOCK_MORALE)) ) //checking if heroes have (or don't have) morale blocking bonuses
			)
				if(rand()%24 < next->Morale()) //this stack hasn't got morale this turn
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
	sg.garrs[bEndArmy1->id] = takeCasualties(bEndArmy1->tempOwner, bEndArmy1->army, gs->curB);
	sg.garrs[bEndArmy2->id] = takeCasualties(bEndArmy2->tempOwner, bEndArmy2->army, gs->curB);
	sendAndApply(&sg);

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
	if (winnerHero) {
		std::pair<ui32, si32> raisedStack = winnerHero->calculateNecromancy(*battleResult.data);

		// Give raised units to winner and show dialog, if any were raised.
		if (raisedStack.first != -1) {
			int slot = winnerHero->army.getSlotFor(raisedStack.first);

			if (slot != -1) {
				SetGarrisons sg;

				sg.garrs[winnerHero->id] = winnerHero->army;
				if (vstd::contains(winnerHero->army.slots, slot)) // Add to existing stack.
					sg.garrs[winnerHero->id].slots[slot].second += raisedStack.second;
				else // Create a new stack.
					sg.garrs[winnerHero->id].slots[slot] = raisedStack;
				winnerHero->showNecromancyDialog(raisedStack);
				sendAndApply(&sg);
			}
		}
	}

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

	if(def->amount <= bsa.killedAmount) //stack killed
	{
		bsa.newAmount = 0;
		bsa.flags |= 1;
		bsa.killedAmount = def->amount; //we cannot kill more creatures than we have
	}
	else
	{
		bsa.newAmount = def->amount - bsa.killedAmount;
	}
}

void CGameHandler::prepareAttack(BattleAttack &bat, const CStack *att, const CStack *def, int distance)
{
	bat.bsa.clear();
	bat.stackAttacking = att->ID;
	std::set<BattleStackAttacked>::iterator i = bat.bsa.insert(BattleStackAttacked()).first;
	#ifdef __GNUC__
		BattleStackAttacked *bsa = (BattleStackAttacked *)&*i;
	#else
		BattleStackAttacked *bsa = &*i;
	#endif

	bsa->stackAttacked = def->ID;
	bsa->attackerID = att->ID;
	bsa->damageAmount = BattleInfo::calculateDmg(att, def, gs->battleGetOwner(att->ID), gs->battleGetOwner(def->ID), bat.shot(), distance);//counting dealt damage
	if(att->Luck() > 0  &&  rand()%24 < att->Luck())
	{
		bsa->damageAmount *= 2;
		bat.flags |= 4;
	}
	prepareAttacked(*bsa, def);
}
void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	srand(time(NULL));
	CPack *pack = NULL;
	try
	{
		while(!end2)
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
		tlog1 << e.what() << std::endl;
		end2 = true;
	}
	HANDLE_EXCEPTION(end2 = true);
handleConEnd:
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
	if(!stackAtEnd && curStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && !accessibility[dest])
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

	std::pair< std::vector<int>, int > path = gs->curB->getPath(curStack->position, dest, accessibilityWithOccupyable, curStack->hasFeatureOfType(StackFeature::FLYING), curStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE), curStack->attackerOwned);

	ret = path.second;

	if(curStack->hasFeatureOfType(StackFeature::FLYING))
	{
		if(path.second <= curStack->Speed() && path.first.size() > 0)
		{
			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			sm.tile = path.first[0];
			sm.distance = path.second;
			sm.ending = true;
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
}

CGameHandler::~CGameHandler(void)
{
	delete applier;
	applier = NULL;
	delete gs;
}
void CGameHandler::init(StartInfo *si, int Seed)
{
	Mapa *map = new Mapa(si->mapname);
	tlog0 << "Map loaded!" << std::endl;
	gs = new CGameState();
	tlog0 << "Gamestate created!" << std::endl;
	gs->init(si,map,Seed);
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

	std::map<ui32,CGHeroInstance *> pool = gs->hpool.heroesPool;

	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->first == 255) continue;
		else if(i->first > PLAYER_LIMIT) assert(0); //illegal player number!

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

				for(std::list<HeroBonus>::iterator  j = h->bonuses.begin(); j != h->bonuses.end(); j++)
					if(j->type == HeroBonus::GENERATE_RESOURCE)
						n.res[i->first][j->subtype] += j->val;
			}
		}
		//n.res.push_back(r);
	}
	for(std::vector<CGTownInstance *>::iterator j = gs->map->towns.begin(); j!=gs->map->towns.end(); j++)//handle towns
	{
		ui8 player = (*j)->tempOwner;
		if(gs->getDate(1)==7) //first day of week
		{
			SetAvailableCreatures sac;
			sac.tid = (**j).id;
			sac.creatures = (**j).creatures;
			for(int k=0;k<CREATURES_PER_TOWN;k++) //creature growths
			{
				if((**j).creatureDwelling(k))//there is dwelling (k-level)
				{
					sac.creatures[k].first += (**j).creatureGrowth(k);
					if(!gs->getDate(0)) //first day of game: use only basic growths
						amin(sac.creatures[k].first, VLC->creh->creatures[(*j)->town->basicCreatures[k]].growth);
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
	}

	sendAndApply(&n);
	tlog5 << "Info about turn " << n.day << "has been sent!" << std::endl;
	handleTimeEvents();
	//call objects
	for(size_t i = 0; i<gs->map->objects.size(); i++)
		if(gs->map->objects[i])
			gs->map->objects[i]->newTurn();
}
void CGameHandler::run(bool resume)
{	
	BOOST_FOREACH(CConnection *cc, conns)
	{//init conn.
		ui8 quantity, pom;
		//ui32 seed;
		if(!resume)
			(*cc) << gs->scenarioOps->mapname << gs->map->checksum << gs->seed;

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
		else
			resume = false;

		std::map<ui8,PlayerState>::iterator i;
		if(!resume)
			i = gs->players.begin();
		else
			i = gs->players.find(gs->currentPlayer);

		for(; i != gs->players.end(); i++)
		{

			if((i->second.towns.size()==0 && i->second.heroes.size()==0)  || i->second.color<0 || i->first>=PLAYER_LIMIT  ) continue; //players has not towns/castle - loser
			states.setFlag(i->first,&PlayerStatus::makingTurn,true);
			gs->currentPlayer = i->first;
			{
				YourTurn yt;
				yt.player = i->first;
				boost::unique_lock<boost::mutex> lock(*connections[i->first]->wmx);
				*connections[i->first] << &yt;    
			}

			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(states.mx);
			while(states.players[i->first].makingTurn && !end2)
			{
				boost::posix_time::time_duration p;
				p = boost::posix_time::milliseconds(200);
				states.cv.timed_wait(lock,p); 
			}
		}
	}
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

void CGameHandler::setupBattle( BattleInfo * curB, int3 tile, const CCreatureSet &army1, const CCreatureSet &army2, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool creatureBank, const CGTownInstance *town)
{
	battleResult.set(NULL);
	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->army1=army1;
	curB->army2=army2;
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
	for(std::map<si32,std::pair<ui32,si32> >::const_iterator i = army1.slots.begin(); i!=army1.slots.end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = attackerCreBank[army1.slots.size()-1][k];
		else if(army1.formation)
			pos = attackerTight[army1.slots.size()-1][k];
		else
			pos = attackerLoose[army1.slots.size()-1][k];

		CStack * stack = curB->generateNewStack(hero1, i->second.first, i->second.second, stacks.size(), true, i->first, gs->map->terrain[tile.x][tile.y][tile.z].tertype, pos);
		stacks.push_back(stack);
	}
	
	k = 0;
	for(std::map<si32,std::pair<ui32,si32> >::const_iterator i = army2.slots.begin(); i!=army2.slots.end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = defenderCreBank[army2.slots.size()-1][k];
		else if(army2.formation)
			pos = defenderTight[army2.slots.size()-1][k];
		else
			pos = defenderLoose[army2.slots.size()-1][k];

		CStack * stack = curB->generateNewStack(hero2, i->second.first, i->second.second, stacks.size(), false, i->first, gs->map->terrain[tile.x][tile.y][tile.z].tertype, pos);
		stacks.push_back(stack);
	}

	for(unsigned g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		if((stacks[g]->position%17)==1 && stacks[g]->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && stacks[g]->attackerOwned)
		{
			stacks[g]->position += 1;
		}
		else if((stacks[g]->position%17)==15 && stacks[g]->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && !stacks[g]->attackerOwned)
		{
			stacks[g]->position -= 1;
		}
	}

	//adding war machines
	if(hero1)
	{
		if(hero1->getArt(13)) //ballista
		{
			CStack * stack = curB->generateNewStack(hero1, 146, 1, stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 52);
			stacks.push_back(stack);
		}
		if(hero1->getArt(14)) //ammo cart
		{
			CStack * stack = curB->generateNewStack(hero1, 148, 1, stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 18);
			stacks.push_back(stack);
		}
		if(hero1->getArt(15)) //first aid tent
		{
			CStack * stack = curB->generateNewStack(hero1, 147, 1, stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 154);
			stacks.push_back(stack);
		}
	}
	if(hero2)
	{
		if(hero2->getArt(13)) //ballista
		{
			CStack * stack = curB->generateNewStack(hero2, 146, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 66);
			stacks.push_back(stack);
		}
		if(hero2->getArt(14)) //ammo cart
		{
			CStack * stack = curB->generateNewStack(hero2, 148, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 32);
			stacks.push_back(stack);
		}
		if(hero2->getArt(15)) //first aid tent
		{
			CStack * stack = curB->generateNewStack(hero2, 147, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 168);
			stacks.push_back(stack);
		}
	}
	if(town && hero1 && town->hasFort()) //catapult
	{
		CStack * stack = curB->generateNewStack(hero1, 145, 1, stacks.size(), true, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, 120);
		stacks.push_back(stack);
	}
	//war machines added

	switch(curB->siege) //adding towers
	{
		
	case 3: //castle
		{//lower tower / upper tower
			CStack * stack = curB->generateNewStack(hero2, 149, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -4);
			stacks.push_back(stack);
			stack = curB->generateNewStack(hero2, 149, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -3);
			stacks.push_back(stack);
		}
	case 2: //citadel
		{//main tower
			CStack * stack = curB->generateNewStack(hero2, 149, 1, stacks.size(), false, 255, gs->map->terrain[tile.x][tile.y][tile.z].tertype, -2);
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
				gs.bonus = HeroBonus(HeroBonus::ONE_BATTLE, HeroBonus::MAGIC_SCHOOL_SKILL, HeroBonus::OBJECT, 3, -1, "", bonusSubtype);
				gs.hid = cHero->id;

				sendAndApply(&gs);
			}

			break;
		}

	case 18: //holy ground
		{
			for(int g=0; g<stacks.size(); ++g) //+1 morale bonus for good creatures, -1 morale bonus for evil creatures
			{
				if (stacks[g]->creature->isGood())
					stacks[g]->features.push_back(makeFeature(StackFeature::MORALE_BONUS, StackFeature::WHOLE_BATTLE, 0, 1, StackFeature::OTHER_SOURCE));
				else if (stacks[g]->creature->isEvil())
					stacks[g]->features.push_back(makeFeature(StackFeature::MORALE_BONUS, StackFeature::WHOLE_BATTLE, 0, -1, StackFeature::OTHER_SOURCE));
			}
			break;
		}
	case 19: //clover field
		{
			for(int g=0; g<stacks.size(); ++g)
			{
				if(stacks[g]->creature->faction == -1) //+2 luck bonus for neutral creatures
				{
					stacks[g]->features.push_back(makeFeature(StackFeature::LUCK_BONUS, StackFeature::WHOLE_BATTLE, 0, 2, StackFeature::OTHER_SOURCE));
				}
			}
			break;
		}
	case 20: //evil fog
		{
			for(int g=0; g<stacks.size(); ++g) //-1 morale bonus for good creatures, +1 morale bonus for evil creatures
			{
				if (stacks[g]->creature->isGood())
					stacks[g]->features.push_back(makeFeature(StackFeature::MORALE_BONUS, StackFeature::WHOLE_BATTLE, 0, -1, StackFeature::OTHER_SOURCE));
				else if (stacks[g]->creature->isEvil())
					stacks[g]->features.push_back(makeFeature(StackFeature::MORALE_BONUS, StackFeature::WHOLE_BATTLE, 0, 1, StackFeature::OTHER_SOURCE));
			}
			break;
		}
	case 22: //cursed ground
		{
			for(int g=0; g<stacks.size(); ++g) //no luck nor morale
			{
				stacks[g]->features.push_back(makeFeature(StackFeature::NO_MORALE, StackFeature::WHOLE_BATTLE, 0, 0, StackFeature::OTHER_SOURCE));
				stacks[g]->features.push_back(makeFeature(StackFeature::NO_LUCK, StackFeature::WHOLE_BATTLE, 0, 0, StackFeature::OTHER_SOURCE));
			}

			const CGHeroInstance * cHero = NULL;
			for(int i=0; i<2; ++i) //blocking spells above level 1
			{
				if(i == 0) cHero = hero1;
				else cHero = hero2;

				if(cHero == NULL) continue;

				GiveBonus gs;
				gs.bonus = HeroBonus(HeroBonus::ONE_BATTLE, HeroBonus::BLOCK_SPELLS_ABOVE_LEVEL, HeroBonus::OBJECT, 1, -1, "", bonusSubtype);
				gs.hid = cHero->id;

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
		if(stacks[b]->alive() && !stacks[b]->hasFeatureOfType(StackFeature::SIEGE_WEAPON))
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
		for(int j=0; j<t->spellsAtLevel(i+1,true) && j<t->spells[i].size(); j++)
		{
			if(!vstd::contains(h->spells,t->spells[i][j]))
				cs.spells.insert(t->spells[i][j]);
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
		tlog1 << "Destination tile os out of the map!\n";
		return false;
	}

	TerrainTile t = gs->map->terrain[hmpos.x][hmpos.y][hmpos.z];
	int cost = gs->getMovementCost(h,h->getPosition(false),CGHeroInstance::convertPosition(dst,false),h->movement);

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
	if((t.tertype == TerrainTile::rock  ||  (t.blocked && !t.visitable)) 
			&& complain("Cannot move hero, destination tile is blocked!") 
		|| (!h->boat && !h->canWalkOnSea() && t.tertype == TerrainTile::water && (t.visitableObjects.size() != 1 ||  (t.visitableObjects.front()->ID != 8 && t.visitableObjects.front()->ID != HEROI_TYPE)))  //hero is not on boat/water walking and dst water tile doesn't contain boat/hero (objs visitable from land)
			&& complain("Cannot move hero, destination tile is on water!")
		|| (h->boat && t.tertype != TerrainTile::water && t.blocked)
			&& complain("Cannot disembark hero, tile is blocked!")
		|| (h->movement < cost  &&  dst != h->pos)
			&& complain("Hero don't have any movement points left!")
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
		return true;
	}

	//checks for standard movement
	if(!instant)
	{
		if( distance(h->pos,dst) >= 1.5  &&  complain("Tiles are not neighbouring!")
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
			BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
			{
				if (obj->blockVisit)
				{
					objectVisited(obj, h);
				}
			}
			tlog5 << "Blocking visit at " << hmpos << std::endl;
			return true;
		}
		else //normal move
		{
			tmh.result = TryMoveHero::SUCCESS;

			BOOST_FOREACH(CGObjectInstance *obj, gs->map->terrain[h->pos.x-1][h->pos.y][h->pos.z].visitableObjects)
			{
				obj->onHeroLeave(h);
			}
			getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);
			sendAndApply(&tmh);
			tlog5 << "Moved to " <<tmh.end<<std::endl;

			//call objects if they are visited
			BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
			{
				objectVisited(obj, h);
			}
		}
		tlog5 << "Movement end!\n";
		return true;
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
	SetObjectProperty sop(objid,1,owner);
	sendAndApply(&sop);
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
	if (creatures.slots.size() <= 0)
		return;
	CCreatureSet heroArmy = h->army;
	while (creatures.slots.size() > 0)
	{
		int slot = heroArmy.getSlotFor (creatures.slots.begin()->second.first);
		if (slot < 0)
			break;
		heroArmy.slots[slot].first = creatures.slots.begin()->second.first;
		heroArmy.slots[slot].second += creatures.slots.begin()->second.second;
		creatures.slots.erase (creatures.slots.begin());
	}

	if (creatures.slots.size() == 0) //all creatures can be moved to hero army - do that
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
	vistiCastleObjects (getTown(obj), getHero(heroID));
	giveSpells (getTown(obj), getHero(heroID));
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
					sha.artifWorn[art.possibleSlots[i]] = artid;
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
			sha.artifWorn[position] = artid;
		}
		else if (!art.isBig())
		{
			sha.artifacts.push_back(artid);
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
	startBattleI(army1, army2, army2->pos - army2->getVisitableOffset(), cb, creatureBank);
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

void CGameHandler::heroExchange(si32 hero1, si32 hero2)
{
	ui8 player1 = getHero(hero1)->tempOwner;
	ui8 player2 = getHero(hero2)->tempOwner;

	if(player1 == player2)
	{
		OpenWindow hex;
		hex.window = OpenWindow::EXCHANGE_WINDOW;
		hex.id1 = hero1;
		hex.id2 = hero2;
		sendAndApply(&hex);
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
		save << hlp << static_cast<CMapHeader&>(*gs->map) << gs->scenarioOps->difficulty << *VLC << gs;
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
	CCreatureSet temp1 = s1->army, temp2 = s2->army,
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
		if(!S1.slots[p1].second)
			S1.slots.erase(p1);
		if(!S2.slots[p2].second)
			S2.slots.erase(p2);
	}
	else if(what==2)//merge
	{
		if(S1.slots[p1].first != S2.slots[p2].first) //not same creature
		{
			complain("Cannot merge different creatures stacks!");
			return false; 
		}

		S2.slots[p2].second += S1.slots[p1].second;
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
			int total = S1.slots[p1].second + S2.slots[p2].second;
			if( (total < val   &&   complain("Cannot split that stack, not enough creatures!"))
				|| (S2.slots[p2].first != S1.slots[p1].first && complain("Cannot rebalance different creatures stacks!"))
			)
			{
				return false; 
			}
			
			S2.slots[p2].second = val;
			S1.slots[p1].second = total - val;
		}
		else //split one stack to the two
		{
			if(S1.slots[p1].second < val)//not enough creatures
			{
				complain("Cannot split that stack, not enough creatures!");
				return false; 
			}
			S2.slots[p2].first = S1.slots[p1].first;
			S2.slots[p2].second = val;
			S1.slots[p1].second -= val;
		}

		if(!S1.slots[p1].second) //if we've moved all creatures
			S1.slots.erase(p1);
	}
	if((s1->needsLastStack() && !S1.slots.size()) //it's not allowed to take last stack from hero army!
		|| (s2->needsLastStack() && !S2.slots.size())
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
	if(!vstd::contains(s1->army.slots,pos))
	{
		complain("Illegal call to disbandCreature - no such stack in army!");
		return false;
	}
	s1->army.slots.erase(pos);
	SetGarrisons sg;
	sg.garrs[id] = s1->army;
	sendAndApply(&sg);
	return true;
}

bool CGameHandler::buildStructure( si32 tid, si32 bid )
{
	CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[tid]);
	CBuilding * b = VLC->buildh->buildings[t->subID][bid];

	if(gs->canBuildStructure(t,bid) != 7)
	{
		complain("Cannot raze that building!");
		return false;
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
		ssi.creatures[bid-30].first = VLC->creh->creatures[crid].growth;
		ssi.creatures[bid-30].second.push_back(crid);
		sendAndApply(&ssi);
	}
	else if(bid == 11)
		ns.bid.insert(27);
	else if(bid == 12)
		ns.bid.insert(28);
	else if(bid == 13)
		ns.bid.insert(29);

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

	if(dw->ID == TOWNI_TYPE)
		dst = dw;
	else if(dw->ID == 17  ||  dw->ID == 20) //advmap dwelling
		dst = getHero(gs->getPlayer(dw->tempOwner)->currentSelection); //TODO: check if current hero is really visiting dwelling

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
	int slot = dst->army.getSlotFor(crid);

	if(!found && complain("Cannot recruit: no such creatures!")
		|| cram > VLC->creh->creatures[crid].maxAmount(gs->getPlayer(dst->tempOwner)->resources) && complain("Cannot recruit: lack of resources!")
		|| cram<=0	&& complain("Cannot recruit: cram <= 0!")
		|| slot<0  && complain("Cannot recruit: no available slot!")) 
	{
		return false;
	}

	//recruit
	SetResources sr;
	sr.player = dst->tempOwner;
	for(int i=0;i<RESOURCE_QUANTITY;i++)
		sr.res[i]  =  gs->getPlayer(dst->tempOwner)->resources[i] - (VLC->creh->creatures[crid].cost[i] * cram);

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = dw->creatures;
	sac.creatures[level].first -= cram;

	SetGarrisons sg;
	sg.garrs[dst->id] = dst->army;
	if(sg.garrs[dst->id].slots.find(slot) == sg.garrs[dst->id].slots.end()) //take a free slot
	{
		sg.garrs[dst->id].slots[slot] = std::make_pair(crid,cram);
	}
	else //add creatures to a already existing stack
	{
		sg.garrs[dst->id].slots[slot].second += cram;
	}
	sendAndApply(&sr); 
	sendAndApply(&sac);
	sendAndApply(&sg);
	return true;
}

bool CGameHandler::upgradeCreature( ui32 objid, ui8 pos, ui32 upgID )
{
	CArmedInstance *obj = static_cast<CArmedInstance*>(gs->map->objects[objid]);
	UpgradeInfo ui = gs->getUpgradeInfo(obj,pos);
	int player = obj->tempOwner;
	int crQuantity = obj->army.slots[pos].second;

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
	sg.garrs[objid] = obj->army;
	sg.garrs[objid].slots[pos].first = upgID;
	sendAndApply(&sg);	
	return true;
}

bool CGameHandler::garrisonSwap( si32 tid )
{
	CGTownInstance *town = gs->getTown(tid);
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{
		CCreatureSet csn = town->visitingHero->army, cso = town->army;
		while(!cso.slots.empty())//while there are unmoved creatures
		{
			int pos = csn.getSlotFor(cso.slots.begin()->second.first);
			if(pos<0)
			{
				//try to merge two other stacks to make place
				std::pair<TSlot, TSlot> toMerge;
				if(csn.mergableStacks(toMerge, cso.slots.begin()->first))
				{
					//merge
					csn.slots[toMerge.second].second += csn.slots[toMerge.first].second;
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
				csn.slots[pos].second += cso.slots.begin()->second.second;
			}
			else //move stack on the free pos
			{
				csn.slots[pos].first = cso.slots.begin()->second.first;
				csn.slots[pos].second = cso.slots.begin()->second.second;
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
		sg.garrs[town->id] = town->visitingHero->army;
		sg.garrs[town->garrisonHero->id] = town->garrisonHero->army;

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

bool CGameHandler::swapArtifacts(si32 srcHeroID, si32 destHeroID, ui16 srcSlot, ui16 destSlot)
{
	CGHeroInstance *srcHero = gs->getHero(srcHeroID);
	CGHeroInstance *destHero = gs->getHero(destHeroID);

	// Make sure exchange is even possible between the two heroes.
	if ((distance(srcHero->pos,destHero->pos) > 1.5 )|| (srcHero->tempOwner != destHero->tempOwner))
		return false;

	const CArtifact *srcArtifact = srcHero->getArt(srcSlot); 
	const CArtifact *destArtifact = destHero->getArt(destSlot);

	// Check if src/dest slots are appropriate for the artifacts exchanged.
	// Moving to the backpack is always allowed.
	if ((!srcArtifact || destSlot < 19)
		&& (((srcArtifact && !vstd::contains(srcArtifact->possibleSlots, destSlot))
			|| (destArtifact && srcSlot < 19 && !vstd::contains(destArtifact->possibleSlots, srcSlot)))))
	{
		complain("Cannot swap artifacts!");
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

	// Perform the exchange.
	SetHeroArtifacts sha;
	sha.hid = srcHeroID;
	sha.artifacts = srcHero->artifacts;
	sha.artifWorn = srcHero->artifWorn;

	sha.setArtAtPos(srcSlot, -1);
	if (destSlot < 19 && (destArtifact || srcSlot < 19))
		sha.setArtAtPos(srcSlot, destHero->getArtAtPos(destSlot));

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
		sendAndApply(&sha);
	}

	return true;
}

/**
 * Sets a hero artifact slot to contain a specific artifact.
 *
 * @param artID ID of an artifact or -1 for no artifact.
 */
bool CGameHandler::setArtifact(si32 heroID, ui16 slot, int artID)
{
	const CGHeroInstance *hero = gs->getHero(heroID);

	if (artID != -1) {
		const CArtifact &artifact = VLC->arth->artifacts[artID]; 

		if (slot < 19 && !vstd::contains(artifact.possibleSlots, slot)) {
			complain("Artifact does not fit!");
			return false;
		}

		if (slot >= 19 && artifact.isBig()) {
			complain("Cannot put big artifacts in backpack!");
			return false;
		}
	}

	if (slot == 16) {
		complain("Cannot alter catapult slot!");
		return false;
	}

	// Perform the exchange.
	SetHeroArtifacts sha;
	sha.hid = heroID;
	sha.artifacts = hero->artifacts;
	sha.artifWorn = hero->artifWorn;
	sha.setArtAtPos(slot, artID);
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
			|| town->town->warMachine!= aid  &&  complain("This machine is unavailable here!") ) //TODO: ballista yard in Stronghold
		{
			return false;
		}

		giveResource(hero->getOwner(),6,-price);
		giveHeroArtifact(aid,hid,9+aid);
		return true;
	}
	return false;
}

bool CGameHandler::tradeResources( ui32 val, ui8 player, ui32 id1, ui32 id2 )
{
	val = std::min(si32(val),gs->getPlayer(player)->resources[id1]);
	double yield = (double)gs->resVals[id1] * val * gs->getMarketEfficiency(player);
	yield /= gs->resVals[id2];
	SetResource sr;
	sr.player = player;
	sr.resid = id1;
	sr.val = gs->getPlayer(player)->resources[id1] - val;
	sendAndApply(&sr);

	sr.resid = id2;
	sr.val = gs->getPlayer(player)->resources[id2] + (int)yield;
	sendAndApply(&sr);

	return true;
}

bool CGameHandler::setFormation( si32 hid, ui8 formation )
{
	gs->getHero(hid)->army.formation = formation;
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
				&& !(curStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE)
					&&  ( curStack->position == ba.destinationTile + (curStack->attackerOwned ?  +1 : -1 ) )
						) //nor occupy specified hex
				) 
			{
				std::string problem = "We cannot move this stack to its destination " + curStack->creature->namePl;
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
				|| (curStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE)									//back <=> front
					&& BattleInfo::mutualPosition(curpos + (curStack->attackerOwned ? -1 : 1), enemypos) >= 0)
				|| (stackAtEnd->hasFeatureOfType(StackFeature::DOUBLE_WIDE)									//front <=> back
					&& BattleInfo::mutualPosition(curpos, enemypos + (stackAtEnd->attackerOwned ? -1 : 1)) >= 0)
				|| (stackAtEnd->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && curStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE)//back <=> back
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

			//counterattack
			if(!curStack->hasFeatureOfType(StackFeature::BLOCKS_RETALIATION)
				&& stackAtEnd->alive()
				&& ( stackAtEnd->counterAttacks > 0 || stackAtEnd->hasFeatureOfType(StackFeature::UNLIMITED_RETALIATIONS) )
				&& !stackAtEnd->hasFeatureOfType(StackFeature::SIEGE_WEAPON)
				&& !stackAtEnd->hasFeatureOfType(StackFeature::HYPNOTIZED))
			{
				prepareAttack(bat, stackAtEnd, curStack, 0);
				bat.flags |= 2;
				sendAndApply(&bat);
			}

			//second attack
			if(curStack->valOfFeatures(StackFeature::ADDITIONAL_ATTACK) > 0
				&& !curStack->hasFeatureOfType(StackFeature::SHOOTER)
				&& curStack->alive()
				&& stackAtEnd->alive()  )
			{
				bat.flags = 0;
				prepareAttack(bat, curStack, stackAtEnd, 0);
				sendAndApply(&bat);
			}

			//return
			if(curStack->hasFeatureOfType(StackFeature::RETURN_AFTER_STRIKE) && startingPos != curStack->position)
			{
				moveStack(ba.stackNumber, startingPos);
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
			prepareAttack(bat, curStack, destStack, 0);
			bat.flags |= 1;
			sendAndApply(&bat);

			if(curStack->valOfFeatures(StackFeature::ADDITIONAL_ATTACK) > 0 //if unit shots twice let's make another shot
				&& curStack->alive()
				&& destStack->alive()
				&& curStack->shots
				)
			{
				prepareAttack(bat, curStack, destStack, 0);
				sendAndApply(&bat);
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
			for(int g=0; g<sbi.shots; ++g)
			{
				if(gs->curB->si.wallState[attackedPart] == 3) //it's not destroyed
					continue;
				
				CatapultAttack ca; //package for clients
				std::pair< std::pair< ui8, si16 >, ui8> attack;
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
							attack.second = v;
							break;
						}
					}
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
	}
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
			sha.artifWorn[17] = 0;
			sendAndApply(&sha);
		}

		sendAndApply(&cs);
		sendAndApply(&sm);
	}
	else if(message == "vcmiainur") //gives 5 archangels into each slot
	{
		SetGarrisons sg;
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!hero) return;
		sg.garrs[hero->id] = hero->army;
		for(int i=0;i<7;i++)
			if(!vstd::contains(sg.garrs[hero->id].slots,i))
				sg.garrs[hero->id].slots[i] = std::pair<ui32,si32>(13,5);
		sendAndApply(&sg);
	}
	else if(message == "vcmiangband") //gives 10 black knight into each slot
	{
		SetGarrisons sg;
		CGHeroInstance *hero = gs->getHero(gs->getPlayer(player)->currentSelection);
		if(!hero) return;
		sg.garrs[hero->id] = hero->army;
		for(int i=0;i<7;i++)
			if(!vstd::contains(sg.garrs[hero->id].slots,i))
				sg.garrs[hero->id].slots[i] = std::pair<ui32,si32>(66,10);
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
		sha.artifWorn[13] = 4;
		sha.artifWorn[14] = 5;
		sha.artifWorn[15] = 6;
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
		sendAndApply(&SystemMessage("CHEATER!!!"));
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
		if ((*it)->hasFeatureOfType(StackFeature::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
			|| ((*it)->valOfFeatures(StackFeature::LEVEL_SPELL_IMMUNITY) >= sp->level))
		{
			ret.push_back((*it)->ID);
			continue;
		}

		//non-negative spells on friendly stacks should always succeed, unless immune
		if(sp->positiveness >= 0 && (*it)->owner == caster->tempOwner)
			continue;

		const CGHeroInstance * bonusHero; //hero we should take bonuses from
		if((*it)->owner == caster->tempOwner)
			bonusHero = caster;
		else
			bonusHero = hero2;

		int prob = (*it)->valOfFeatures(StackFeature::MAGIC_RESISTANCE); //probability of resistance in %
		if(bonusHero)
		{
			//bonusHero's resistance support (secondary skils and artifacts)
			prob += bonusHero->valOfBonuses(HeroBonus::MAGIC_RESISTANCE);

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
			if( (*it)->hasFeatureOfType(StackFeature::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
				|| ( (*it)->amount - 1 ) * (*it)->MaxHealth() + (*it)->firstHPleft 
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

			CSpell *s = &VLC->spellh->spells[ba.additionalInfo];
			ui8 skill = h->getSpellSchoolLevel(s); //skill level

			if(   !(h->canCastThisSpell(s)) //hero cannot cast this spell at all
				|| (h->mana < gs->curB->getSpellCost(s, h)) //not enough mana
				|| (ba.additionalInfo < 10) //it's adventure spell (not combat)
				|| (gs->curB->castSpells[ba.side]) //spell has been cast
				|| (secondHero->hasBonusOfType(HeroBonus::SPELL_IMMUNITY, s->id)) //non - casting hero provides immunity for this spell 
				|| (gs->battleMaxSpellLevel() < s->level) //non - casting hero stops caster from casting this spell
				)
			{
				tlog2 << "Spell cannot be cast!\n";
				return false;
			}

			sendAndApply(&StartAction(ba)); //start spell casting

			SpellCast sc;
			sc.side = ba.side;
			sc.id = ba.additionalInfo;
			sc.skill = skill;
			sc.tile = ba.destinationTile;
			sc.dmgToDisplay = 0;

			//calculating affected creatures for all spells
			std::set<CStack*> attackedCres = gs->curB->getAttackedCreatures(s, h, ba.destinationTile);
			for(std::set<CStack*>::const_iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
			{
				sc.affectedCres.insert((*it)->ID);
			}

			//checking if creatures resist
			sc.resisted = calculateResistedStacks(s, h, secondHero, attackedCres);

			//calculating dmg to display
			for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
			{
				if(vstd::contains(sc.resisted, (*it)->ID)) //this creature resisted the spell
					continue;
				sc.dmgToDisplay += gs->curB->calculateSpellDmg(s, h, *it);
			}
			
			sendAndApply(&sc);

			//applying effects
			switch(ba.additionalInfo) //spell id
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
				{
					StacksInjured si;
					for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
					{
						if(vstd::contains(sc.resisted, (*it)->ID)) //this creature resisted the spell
							continue;

						BattleStackAttacked bsa;
						bsa.flags |= 2;
						bsa.effect = VLC->spellh->spells[ba.additionalInfo].mainEffectAnim;
						bsa.damageAmount = gs->curB->calculateSpellDmg(s, h, *it);
						bsa.stackAttacked = (*it)->ID;
						bsa.attackerID = -1;
						prepareAttacked(bsa,*it);
						si.stacks.insert(bsa);
					}
					if(!si.stacks.empty())
						sendAndApply(&si);
					break;
				}
			case 27: //shield 
			case 28: //air shield
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
						sse.stacks.insert((*it)->ID);
					}
					sse.effect.id = ba.additionalInfo;
					sse.effect.level = h->getSpellSchoolLevel(s);
					sse.effect.turnsRemain = BattleInfo::calculateSpellDuration(s, h);
					if(!sse.stacks.empty())
						sendAndApply(&sse);
					break;
				}
			case 37: //cure
			case 38: //resurrection
			case 39: //animate dead
				{
					StacksHealedOrResurrected shr;
					for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
					{
						if(vstd::contains(sc.resisted, (*it)->ID) //this creature resisted the spell
							|| (s->id == 39 && !(*it)->hasFeatureOfType(StackFeature::UNDEAD)) //we try to cast animate dead on living stack
							) 
							continue;
						StacksHealedOrResurrected::HealInfo hi;
						hi.stackID = (*it)->ID;
						hi.healedHP = calculateHealedHP(h, s, *it);
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

						if(vstd::contains(blockedHexes, ba.destinationTile)) //this obstacle covers given hex
						{
							obr.obstacles.insert(gs->curB->obstacles[g].uniqueID);
						}
					}
					if(!obr.obstacles.empty())
						sendAndApply(&obr);

					break;
				}
			}
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
	if (obj->o->ID == TOWNI_TYPE)
	{ //check what kind  of creatures are avaliable in town
		if (VLC->creh->creatures[(static_cast<const CGTownInstance*>(obj))->creatures[0].second[0]].isGood())
			boatType = 1;
		else if (VLC->creh->creatures[(static_cast<const CGTownInstance*>(obj))->creatures[0].second[0]].isEvil())
			boatType = 0;
		else //neutral
			boatType = 2;
	}
	no.subID = boatType;
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
