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
#include "CGameHandler.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp> //no i/o just types
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <fstream>

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

class CMP_stack
{
public:
	inline bool operator ()(const CStack* a, const CStack* b)
	{
		return (a->Speed())>(b->Speed());
	}
} cmpst ;

static inline double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}
static void giveExp(BattleResult &r)
{
	r.exp[0] = 0;
	r.exp[1] = 0;
	for(std::set<std::pair<ui32,si32> >::iterator i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
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
}

void CGameHandler::changePrimSkill(int ID, int which, int val, bool abs)
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
	CCreatureSet ret(set);
	for(int i=0; i<bat->stacks.size();i++)
	{
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

void CGameHandler::startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb)
{
	BattleInfo *curB = new BattleInfo;
	setupBattle(curB, tile, army1, army2, hero1, hero2); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces
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
		CStack *next;
		while(!battleResult.get() && (next=gs->curB->getNextStack()))
		{
			next->state -= WAITING; //if stack was waiting it'll now make move, so it won't be "waiting" anymore

			//check for bad morale => freeze
			if(next->Morale() < 0 &&
				!((hero1 && hero1->hasBonusOfType(HeroBonus::BLOCK_MORALE)) || (hero2 && hero2->hasBonusOfType(HeroBonus::BLOCK_MORALE))) //checking if heroes have (or don't have) morale blocking bonuses)
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

askInterfaceForMove:
			//ask interface and wait for answer
			if(!battleResult.get())
			{
				BattleSetActiveStack sas;
				sas.stack = next->ID;
				sendAndApply(&sas);
				boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
				while(!battleMadeAction.data  &&  !battleResult.get()) //active stack hasn't made its action and battle is still going
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
				&& !((hero1 && hero1->hasBonusOfType(HeroBonus::BLOCK_MORALE)) || (hero2 && hero2->hasBonusOfType(HeroBonus::BLOCK_MORALE)) ) //checking if heroes have (or don't have) morale blocking bonuses
			)
				if(rand()%24 < next->Morale()) //this stack hasn't got morale this turn
					goto askInterfaceForMove; //move this stack once more
		}
	}

	//unblock engaged players
	if(hero1->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero1->tempOwner,&PlayerStatus::engagedIntoBattle,false);
	if(hero2 && hero2->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero2->tempOwner,&PlayerStatus::engagedIntoBattle,false);	

	//casualties among heroes armies
	SetGarrisons sg;
	if(hero1)
		sg.garrs[hero1->id] = takeCasualties(hero1->tempOwner,hero1->army,gs->curB);
	if(hero2)
		sg.garrs[hero2->id] = takeCasualties(hero2->tempOwner,hero2->army,gs->curB);
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

	if(cb)
		cb(battleResult.data);

	delete battleResult.data;

}
void CGameHandler::prepareAttacked(BattleStackAttacked &bsa, CStack *def)
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

void CGameHandler::prepareAttack(BattleAttack &bat, CStack *att, CStack *def)
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
	bsa->damageAmount = BattleInfo::calculateDmg(att, def, gs->getHero(att->attackerOwned ? gs->curB->hero1 : gs->curB->hero2), gs->getHero(def->attackerOwned ? gs->curB->hero1 : gs->curB->hero2), bat.shot());//counting dealt damage
	if(att->Luck() > 0  &&  rand()%24 < att->Luck())
	{
		bsa->damageAmount *= 2;
		bat.flags |= 4;
	}
	prepareAttacked(*bsa,def);
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
				PackageApplied applied;
				applied.result = result;
				applied.packType = packType;
				{
					boost::unique_lock<boost::mutex> lock(*c.wmx);
					c << &applied;
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
	HANDLE_EXCEPTION(end2 = true);
handleConEnd:
	tlog1 << "Ended handling connection\n";
#undef SPELL_CAST_TEMPLATE_1
#undef SPELL_CAST_TEMPLATE_2
}
void CGameHandler::moveStack(int stack, int dest)
{							
	CStack *curStack = gs->curB->getStack(stack),
		*stackAtEnd = gs->curB->getStackT(dest);

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
	if(!stackAtEnd && curStack->creature->isDoubleWide() && !accessibility[dest])
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
		return;

	//if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
	//	return false;

	std::pair< std::vector<int>, int > path = gs->curB->getPath(curStack->position, dest, accessibility, curStack->creature->isFlying());
	if(curStack->creature->isFlying())
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

	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->first == 255) continue;
		if(gs->getDate(1)==7) //first day of week - new heroes in tavern
		{
			SetAvailableHeroes sah;
			sah.player = i->first;
			CGHeroInstance *h = gs->hpool.pickHeroFor(true,i->first,&VLC->townh->towns[gs->scenarioOps->getIthPlayersSettings(i->first).castle]);
			if(h)
				sah.hid1 = h->subID;
			else
				sah.hid1 = -1;
			h = gs->hpool.pickHeroFor(false,i->first,&VLC->townh->towns[gs->scenarioOps->getIthPlayersSettings(i->first).castle],sah.hid1);
			if(h)
				sah.hid2 = h->subID;
			else
				sah.hid2 = -1;
			sendAndApply(&sah);
		}
		if(i->first>=PLAYER_LIMIT) continue;
		SetResources r;
		r.player = i->first;
		for(int j=0;j<RESOURCE_QUANTITY;j++)
			r.res[j] = i->second.resources[j];
		
		BOOST_FOREACH(CGHeroInstance *h, (*i).second.heroes)
		{
			NewTurn::Hero hth;
			hth.id = h->id;
			hth.move = h->maxMovePoints(true); //TODO: check if hero is really on the land

			if(h->visitedTown && vstd::contains(h->visitedTown->builtBuildings,0)) //if hero starts turn in town with mage guild
				hth.mana = h->manaLimit(); //restore all mana
			else
				hth.mana = std::max(si32(0), std::min(h->mana + h->manaRegain(), h->manaLimit()) ); 

			n.heroes.insert(hth);
			
			if(gs->day) //not first day
			{
				switch(h->getSecSkillLevel(13)) //handle estates - give gold
				{
				case 1: //basic
					r.res[6] += 125;
					break;
				case 2: //advanced
					r.res[6] += 250;
					break;
				case 3: //expert
					r.res[6] += 500;
					break;
				}

				for(std::list<HeroBonus>::iterator i = h->bonuses.begin(); i != h->bonuses.end(); i++)
					if(i->type == HeroBonus::GENERATE_RESOURCE)
						r.res[i->subtype] += i->val;
			}
		}
		for(std::vector<CGTownInstance *>::iterator j=i->second.towns.begin();j!=i->second.towns.end();j++)//handle towns
		{
			if(gs->day && vstd::contains((**j).builtBuildings,15)) //not first day and there is resource silo
			{
				if((**j).town->primaryRes == 127) //we'll give wood and ore
				{
					r.res[0] += 1;
					r.res[2] += 1;
				}
				else
				{
					r.res[(**j).town->primaryRes] += 1;
				}
			}
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
			if(gs->day  &&  i->first<PLAYER_LIMIT)//not the first day and town not neutral
				r.res[6] += (**j).dailyIncome();
		}
		n.res.push_back(r);
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
	static void readItTo(ifstream & input, vector< vector<int> > & dest)
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

void CGameHandler::setupBattle( BattleInfo * curB, int3 tile, CCreatureSet &army1, CCreatureSet &army2, CGHeroInstance * hero1, CGHeroInstance * hero2 )
{
	battleResult.set(NULL);
	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->siege = 0; //TODO: add sieges
	curB->army1=army1;
	curB->army2=army2;
	curB->hero1=(hero1)?(hero1->id):(-1);
	curB->hero2=(hero2)?(hero2->id):(-1);
	curB->side1=(hero1)?(hero1->tempOwner):(-1);
	curB->side2=(hero2)?(hero2->tempOwner):(-1);
	curB->round = -2;
	curB->activeStack = -1;
	for(std::map<si32,std::pair<ui32,si32> >::iterator i = army1.slots.begin(); i!=army1.slots.end(); i++)
	{
		stacks.push_back(new CStack(&VLC->creh->creatures[i->second.first],i->second.second,hero1->tempOwner, stacks.size(), true,i->first));
		if(hero1)
		{
			stacks.back()->features.push_back(makeFeature(StackFeature::SPEED_BONUS, StackFeature::WHOLE_BATTLE, 0, hero1->valOfBonuses(HeroBonus::STACKS_SPEED), StackFeature::BONUS_FROM_HERO));
			//base luck/morale calculations
			//TODO: check if terrain is native, add bonuses for neutral stacks, bonuses from town
			stacks.back()->morale = hero1->getCurrentMorale(i->first,false);
			stacks.back()->luck = hero1->getCurrentLuck(i->first,false);
			stacks.back()->features.push_back(makeFeature(StackFeature::ATTACK_BONUS, StackFeature::WHOLE_BATTLE, 0, hero1->getPrimSkillLevel(0), StackFeature::BONUS_FROM_HERO));
			stacks.back()->features.push_back(makeFeature(StackFeature::DEFENCE_BONUS, StackFeature::WHOLE_BATTLE, 0, hero1->getPrimSkillLevel(1), StackFeature::BONUS_FROM_HERO));
			stacks.back()->features.push_back(makeFeature(StackFeature::HP_BONUS, StackFeature::WHOLE_BATTLE, 0, hero1->valOfBonuses(HeroBonus::STACK_HEALTH), StackFeature::BONUS_FROM_HERO));
			stacks.back()->firstHPleft = stacks.back()->MaxHealth();
		}
		else
		{
			stacks.back()->morale = 0;
			stacks.back()->luck = 0;
		}

		stacks[stacks.size()-1]->ID = stacks.size()-1;
	}
	//initialization of positions
	std::ifstream positions;
	positions.open("config" PATHSEPARATOR "battleStartpos.txt", std::ios_base::in|std::ios_base::binary);
	if(!positions.is_open())
	{
		tlog1<<"Unable to open battleStartpos.txt!"<<std::endl;
	}
	std::string dump;
	positions>>dump; positions>>dump;
	std::vector< std::vector<int> > attackerLoose, defenderLoose, attackerTight, defenderTight;
	CGH::readItTo(positions, attackerLoose);
	positions>>dump;
	CGH::readItTo(positions, defenderLoose);
	positions>>dump;
	positions>>dump;
	CGH::readItTo(positions, attackerTight);
	positions>>dump;
	CGH::readItTo(positions, defenderTight);
	positions.close();
	
	if(army1.formation)
		for(int b=0; b<army1.slots.size(); ++b) //tight
		{
			stacks[b]->position = attackerTight[army1.slots.size()-1][b];
		}
	else
		for(int b=0; b<army1.slots.size(); ++b) //loose
		{
			stacks[b]->position = attackerLoose[army1.slots.size()-1][b];
		}
	for(std::map<si32,std::pair<ui32,si32> >::iterator i = army2.slots.begin(); i!=army2.slots.end(); i++)
	{
		stacks.push_back(new CStack(&VLC->creh->creatures[i->second.first],i->second.second,hero2 ? hero2->tempOwner : 255, stacks.size(), false, i->first));
		//base luck/morale calculations
		//TODO: check if terrain is native, add bonuses for neutral stacks, bonuses from town
		if(hero2)
		{
			stacks.back()->features.push_back(makeFeature(StackFeature::SPEED_BONUS, StackFeature::WHOLE_BATTLE, 0, hero2->valOfBonuses(HeroBonus::STACKS_SPEED), StackFeature::BONUS_FROM_HERO));
			stacks.back()->morale = hero2->getCurrentMorale(i->first,false);
			stacks.back()->luck = hero2->getCurrentLuck(i->first,false);
			stacks.back()->features.push_back(makeFeature(StackFeature::ATTACK_BONUS, StackFeature::WHOLE_BATTLE, 0, hero2->getPrimSkillLevel(0), StackFeature::BONUS_FROM_HERO));
			stacks.back()->features.push_back(makeFeature(StackFeature::DEFENCE_BONUS, StackFeature::WHOLE_BATTLE, 0, hero2->getPrimSkillLevel(1), StackFeature::BONUS_FROM_HERO));
			stacks.back()->features.push_back(makeFeature(StackFeature::HP_BONUS, StackFeature::WHOLE_BATTLE, 0, hero2->valOfBonuses(HeroBonus::STACK_HEALTH), StackFeature::BONUS_FROM_HERO));
			stacks.back()->firstHPleft = stacks.back()->MaxHealth();
		}
		else
		{
			stacks.back()->morale = 0;
			stacks.back()->luck = 0;
		}
	}

	if(army2.formation)
		for(int b=0; b<army2.slots.size(); ++b) //tight
		{
			stacks[b+army1.slots.size()]->position = defenderTight[army2.slots.size()-1][b];
		}
	else
		for(int b=0; b<army2.slots.size(); ++b) //loose
		{
			stacks[b+army1.slots.size()]->position = defenderLoose[army2.slots.size()-1][b];
		}

	for(unsigned g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		if((stacks[g]->position%17)==1 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position += 1;
		}
		else if((stacks[g]->position%17)==15 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position -= 1;
		}
	}

	//adding native terrain bonuses
	for(int g=0; g<stacks.size(); ++g)
	{
		int faction = stacks[g]->creature->faction;
		if(faction >= 0 && VLC->heroh->nativeTerrains[faction] == gs->map->terrain[tile.x][tile.y][tile.z].tertype )
		{
			stacks[g]->features.push_back(makeFeature(StackFeature::SPEED_BONUS, StackFeature::WHOLE_BATTLE, 0, 1, StackFeature::OTHER_SOURCE));
			stacks[g]->features.push_back(makeFeature(StackFeature::ATTACK_BONUS, StackFeature::WHOLE_BATTLE, 0, 1, StackFeature::OTHER_SOURCE));
			stacks[g]->features.push_back(makeFeature(StackFeature::DEFENCE_BONUS, StackFeature::WHOLE_BATTLE, 0, 1, StackFeature::OTHER_SOURCE));
		}
	}

	//adding war machines
	if(hero1)
	{
		if(hero1->getArt(13)) //ballista
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[146], 1, hero1->tempOwner, stacks.size(), true, 255));
			stacks[stacks.size()-1]->position = 52;
			stacks.back()->morale = hero1->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero1->getCurrentLuck(stacks.back()->ID,false);
		}
		if(hero1->getArt(14)) //ammo cart
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[148], 1, hero1->tempOwner, stacks.size(), true, 255));
			stacks[stacks.size()-1]->position = 18;
			stacks.back()->morale = hero1->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero1->getCurrentLuck(stacks.back()->ID,false);
		}
		if(hero1->getArt(15)) //first aid tent
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[147], 1, hero1->tempOwner, stacks.size(), true, 255));
			stacks[stacks.size()-1]->position = 154;
			stacks.back()->morale = hero1->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero1->getCurrentLuck(stacks.back()->ID,false);
		}
	}
	if(hero2)
	{
		if(hero2->getArt(13)) //ballista
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[146], 1, hero2->tempOwner, stacks.size(), false, 255));
			stacks[stacks.size()-1]->position = 66;
			stacks.back()->morale = hero2->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero2->getCurrentLuck(stacks.back()->ID,false);
		}
		if(hero2->getArt(14)) //ammo cart
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[148], 1, hero2->tempOwner, stacks.size(), false, 255));
			stacks[stacks.size()-1]->position = 32;
			stacks.back()->morale = hero2->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero2->getCurrentLuck(stacks.back()->ID,false);
		}
		if(hero2->getArt(15)) //first aid tent
		{
			stacks.push_back(new CStack(&VLC->creh->creatures[147], 1, hero2->tempOwner, stacks.size(), false, 255));
			stacks[stacks.size()-1]->position = 168;
			stacks.back()->morale = hero2->getCurrentMorale(stacks.back()->ID,false);
			stacks.back()->luck = hero2->getCurrentLuck(stacks.back()->ID,false);
		}
	}
	//war machines added
	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//randomize obstacles
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

	int terType = gs->battleGetBattlefieldType(tile);

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
			coi.ID = possibleObstacles[rand()%possibleObstacles.size()];
			coi.pos = rand()%BFIELD_SIZE;
			std::vector<int> block = VLC->heroh->obstacles[coi.ID].getBlocked(coi.pos);
			bool badObstacle = false;
			for(int b=0; b<block.size(); ++b)
			{
				if(!obAv[block[b]])
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

	//block engaged players
	if(hero1->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero1->tempOwner,&PlayerStatus::engagedIntoBattle,true);
	if(hero2 && hero2->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero2->tempOwner,&PlayerStatus::engagedIntoBattle,true);

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
		|| (!h->boat && !h->canWalkOnSea() && t.tertype == TerrainTile::water && (t.visitableObjects.size() != 1 ||  t.visitableObjects.front()->ID != 8)) 
			&& complain("Cannot move hero, destination tile is on water!"))
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

	//checks for standard movement
	if(!instant)
	{
		if( (distance(h->pos,dst)>=1.5) //tiles are not neighouring
			|| (h->movement < cost  &&  h->movement < 100) //lack of movement points
		  ) 
		{
			tlog2 << "Cannot move hero, not enough move points or tiles are not neighbouring!\n";
			sendAndApply(&tmh);
			return false;
		}

		//check if there is blocking visitable object
		blockvis = false;
		tmh.movePoints = std::max(si32(0),h->movement-cost); //take move points
		BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
		{
			if(obj != h  &&  obj->blockVisit)
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
				if(obj->tempOwner==h->tempOwner) 
					return true;//TODO: exchange
				//TODO: check for ally
				CGHeroInstance *dh = static_cast<CGHeroInstance *>(obj);
				startBattleI(&h->army,&dh->army,dst,h,dh,0);
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
	SetResource sr;
	sr.player = player;
	sr.resid = which;
	sr.val = gs->players.find(player)->second.resources[which]+val;
	sendAndApply(&sr);
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
	giveSpells(getTown(obj),getHero(heroID));
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
			if(i == art.possibleSlots.size()) //if haven't find proper slot, use backpack
				sha.artifacts.push_back(artid);
		}
		else //should be -1 => put artifact into backpack
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
		else
		{
			sha.artifacts.push_back(artid);
		}
	}

	sendAndApply(&sha);
}

void CGameHandler::startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb) //use hero=NULL for no hero
{
	boost::thread(boost::bind(&CGameHandler::startBattle,this,*const_cast<CCreatureSet *>(army1),*const_cast<CCreatureSet *>(army2),tile,const_cast<CGHeroInstance *>(hero1), const_cast<CGHeroInstance *>(hero2),cb));
}
void CGameHandler::startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb) //for hero<=>neutral army
{
	CGHeroInstance* h = const_cast<CGHeroInstance*>(getHero(heroID));
	startBattleI(&h->army,&army,tile,h,NULL,cb);
	//battle(&h->army,army,tile,h,NULL);
}

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

void CGameHandler::setObjProperty( int objid, int prop, int val )
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
		CSaveFile save(std::string("Games") + PATHSEPARATOR + fname + ".vlgm1");
		char hlp[8] = "VCMISVG";
		save << hlp << static_cast<CMapHeader&>(*gs->map) << gs->scenarioOps->difficulty << *VLC << gs;
	}

	{
		tlog0 << "Serializing server info...\n";
		CSaveFile save(std::string("Games") + PATHSEPARATOR + fname + ".vsgm1");
		save << *this;
	}
	tlog0 << "Game has been succesfully saved!\n";
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
		complain("Cannot build that building!");
		return false;
	}

	NewStructures ns;
	ns.tid = tid;
	if(bid>36) //upg dwelling
	{
		if(t->getHordeLevel(0) == (bid-37))
			ns.bid.insert(19);
		else if(t->getHordeLevel(1) == (bid-37))
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

	ns.bid.insert(bid);
	ns.builded = t->builded + 1;
	sendAndApply(&ns);

	SetResources sr;
	sr.player = t->tempOwner;
	sr.res = gs->getPlayer(t->tempOwner)->resources;
	for(int i=0;i<7;i++)
		sr.res[i]-=b->resources[i];
	sendAndApply(&sr);

	if(bid<5) //it's mage guild
	{
		if(t->visitingHero)
			giveSpells(t,t->visitingHero);
		if(t->garrisonHero)
			giveSpells(t,t->garrisonHero);
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
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies
	{
		CCreatureSet csn = town->visitingHero->army, cso = town->army;
		while(!cso.slots.empty())//while there are unmoved creatures
		{
			int pos = csn.getSlotFor(cso.slots.begin()->second.first);
			if(pos<0)
			{
				complain("Cannot make garrison swap, not enough free slots!");
				return false;
			}
			if(csn.slots.find(pos)!=csn.slots.end()) //add creatures to the existing stack
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

bool CGameHandler::swapArtifacts( si32 hid1, si32 hid2, ui16 slot1, ui16 slot2 )
{
	CGHeroInstance *h1 = gs->getHero(hid1), *h2 = gs->getHero(hid2);
	if((distance(h1->pos,h2->pos) > 1.0)   ||   (h1->tempOwner != h2->tempOwner))
		return false;

	const CArtifact *a1 = h1->getArt(slot1), 
		*a2=h2->getArt(slot2);

	//check if 
	//	1) slots are appropriate for that artifacts 
	//	2) they are not war machine
	if((a1 && slot2<19 && !vstd::contains(a1->possibleSlots,slot2) || (a2 && slot1<19 && !vstd::contains(a2->possibleSlots,slot1))) && complain("Cannot swap artifacts!")
		|| (slot1>=13 && slot1<=16 || slot2>=13 && slot2<=16) && complain("Cannot move war machine!")
	)
	{
		return false;
	}

	SetHeroArtifacts sha;
	sha.hid = hid1;
	sha.artifacts = h1->artifacts;
	sha.artifWorn = h1->artifWorn;
	sha.setArtAtPos(slot1,h2->getArtAtPos(slot2));
	if(h1 == h2) sha.setArtAtPos(slot2,h1->getArtAtPos(slot1));
	sendAndApply(&sha);
	if(hid1 != hid2)
	{
		sha.hid = hid2;
		sha.artifacts = h2->artifacts;
		sha.artifWorn = h2->artifWorn;
		sha.setArtAtPos(slot2,h1->getArtAtPos(slot1));
		sendAndApply(&sha);
	}

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
			|| town->town->warMachine!= aid  &&  complain("This machine is unavailale here!") ) //TODO: ballista yard in Stronghold
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

	HeroRecruited hr;
	hr.tid = tid;
	hr.hid = nh->subID;
	hr.player = t->tempOwner;
	hr.tile = t->pos - int3(1,0,0);
	sendAndApply(&hr);

	SetAvailableHeroes sah;
	(hid ? sah.hid2 : sah.hid1) = gs->hpool.pickHeroFor(false,t->tempOwner,t->town)->subID;
	(hid ? sah.hid1 : sah.hid2) = gs->getPlayer(t->tempOwner)->availableHeroes[!hid]->subID;
	sah.player = t->tempOwner;
	sah.flags = hid+1;
	sendAndApply(&sah);

	SetResource sr;
	sr.player = t->tempOwner;
	sr.resid = 6;
	sr.val = gs->getPlayer(t->tempOwner)->resources[6] - 2500;
	sendAndApply(&sr);

	giveSpells(t,nh);
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
			//TODO: check if fleeing is possible (e.g. enemy may have Shackles of War)
			//TODO: calculate casualties
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
			moveStack(ba.stackNumber,ba.destinationTile);
			CStack *curStack = gs->curB->getStack(ba.stackNumber),
				*stackAtEnd = gs->curB->getStackT(ba.additionalInfo);

			if(curStack->position != ba.destinationTile) //we wasn't able to reach destination tile
			{
				tlog3<<"We cannot move this stack to its destination "<<curStack->creature->namePl<<std::endl;
				ok = false;
			}

			if(!stackAtEnd)
			{
				tlog3 << "There is no stack on " << ba.additionalInfo << " tile (no attack)!";
				ok = false;
				break;
			}

			ui16 curpos = curStack->position, 
				enemypos = stackAtEnd->position;


			if( !(
				(BattleInfo::mutualPosition(curpos, enemypos) >= 0)						//front <=> front
				|| (curStack->creature->isDoubleWide()									//back <=> front
					&& BattleInfo::mutualPosition(curpos + (curStack->attackerOwned ? -1 : 1), enemypos) >= 0)
				|| (stackAtEnd->creature->isDoubleWide()									//front <=> back
					&& BattleInfo::mutualPosition(curpos, enemypos + (stackAtEnd->attackerOwned ? -1 : 1)) >= 0)
				|| (stackAtEnd->creature->isDoubleWide() && curStack->creature->isDoubleWide()//back <=> back
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
			prepareAttack(bat,curStack,stackAtEnd);
			sendAndApply(&bat);

			//counterattack
			if(!curStack->hasFeatureOfType(StackFeature::BLOCKS_RETALIATION)
				&& stackAtEnd->alive()
				&& stackAtEnd->counterAttacks
				&& !stackAtEnd->hasFeatureOfType(StackFeature::SIEGE_WEAPON)) //TODO: support for multiple retaliatons per turn
			{
				prepareAttack(bat,stackAtEnd,curStack);
				bat.flags |= 2;
				sendAndApply(&bat);
			}

			//second attack
			if(curStack->valOfFeatures(StackFeature::ADDITIONAL_ATTACK) > 0
				&& curStack->alive()
				&& stackAtEnd->alive()  )
			{
				bat.flags = 0;
				prepareAttack(bat,curStack,stackAtEnd);
				sendAndApply(&bat);
			}
			sendAndApply(&EndAction());
			break;
		}
	case 7: //shoot
		{
			CStack *curStack = gs->curB->getStack(ba.stackNumber),
				*destStack= gs->curB->getStackT(ba.destinationTile);
			if(!curStack //our stack exists
				|| !destStack //there is a stack at destination tile
				|| !curStack->shots //stack has shots
				|| gs->curB->isStackBlocked(curStack->ID) //we are not blocked
				|| !curStack->hasFeatureOfType(StackFeature::SHOOTER) //our stack is shooting unit
				)
				break;
			//for(int g=0; g<curStack->effects.size(); ++g)
			//{
			//	if(61 == curStack->effects[g].id) //forgetfulness
			//		break;
			//}
			if(curStack->hasFeatureOfType(StackFeature::FORGETFULL)) //forgetfulness
				break;

			sendAndApply(&StartAction(ba)); //start shooting

			BattleAttack bat;
			prepareAttack(bat,curStack,destStack);
			bat.flags |= 1;
			sendAndApply(&bat);

			if(curStack->valOfFeatures(StackFeature::ADDITIONAL_ATTACK) > 0 //if unit shots twice let's make another shot
				&& curStack->alive()
				&& destStack->alive()
				&& curStack->shots
				)
			{
				prepareAttack(bat,curStack,destStack);
				sendAndApply(&bat);
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
		cs.learn = 1;
		for(int i=0;i<VLC->spellh->spells.size();i++)
		{
			if(!VLC->spellh->spells[i].creatureAbility)
				cs.spells.insert(i);
		}
		sm.hid = cs.hid = gs->players[player].currentSelection;
		sm.val = 999;
		if(gs->getHero(cs.hid))
		{
			sendAndApply(&cs);
			sendAndApply(&sm);
		}
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
	else if(message == "vcmiangband") //gives 10 black knightinto each slot
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

static ui32 calculateSpellDmg(const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature)
{
	ui32 ret = 0; //value to return
	switch(sp->id)
	{
		case 15: //magic arrow
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 16: //ice bolt
			{
				ret = caster->getPrimSkillLevel(2) * 20  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 17: //lightning bolt
			{
				ret = caster->getPrimSkillLevel(2) * 25  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 18: //implosion
			{
				ret = caster->getPrimSkillLevel(2) * 75  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 20: //frost ring
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 21: //fireball
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 22: //inferno
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 23: //meteor shower
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 24: //death ripple
			{
				ret = caster->getPrimSkillLevel(2) * 5  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 25: //destroy undead
			{
				ret = caster->getPrimSkillLevel(2) * 10  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
		case 26: //armageddon
			{
				ret = caster->getPrimSkillLevel(2) * 50  +  sp->powers[caster->getSpellSchoolLevel(sp)];
			}
	}
	//applying sorcerery secondary skill
	switch(caster->getSecSkillLevel(25))
	{
	case 1: //basic
		ret *= 1.05f;
		break;
	case 2: //advanced
		ret *= 1.1f;
		break;
	case 3: //expert
		ret *= 1.15f;
		break;
	}
	//applying hero bonuses
	if(sp->air && caster->valOfBonuses(HeroBonus::AIR_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(HeroBonus::AIR_SPELL_DMG_PREMY) / 100.0f);
	}
	else if(sp->fire && caster->valOfBonuses(HeroBonus::FIRE_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(HeroBonus::FIRE_SPELL_DMG_PREMY) / 100.0f);
	}
	else if(sp->water && caster->valOfBonuses(HeroBonus::WATER_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(HeroBonus::WATER_SPELL_DMG_PREMY) / 100.0f);
	}
	else if(sp->earth && caster->valOfBonuses(HeroBonus::EARTH_SPELL_DMG_PREMY) != 0)
	{
		ret *= (100.0f + caster->valOfBonuses(HeroBonus::EARTH_SPELL_DMG_PREMY) / 100.0f);
	}

	//applying protections - when spell has more then one elements, only one protection should be applied (I think)
	if(sp->air && affectedCreature->hasFeatureOfType(StackFeature::SPELL_DAMAGE_REDUCTION, 0)) //air spell & protection from air
	{
		ret *= affectedCreature->valOfFeatures(StackFeature::SPELL_DAMAGE_REDUCTION, 0);
		ret /= 100;
	}
	else if(sp->fire && affectedCreature->hasFeatureOfType(StackFeature::SPELL_DAMAGE_REDUCTION, 1)) //fire spell & protection from fire
	{
		ret *= affectedCreature->valOfFeatures(StackFeature::SPELL_DAMAGE_REDUCTION, 1);
		ret /= 100;
	}
	else if(sp->water && affectedCreature->hasFeatureOfType(StackFeature::SPELL_DAMAGE_REDUCTION, 2)) //water spell & protection from water
	{
		ret *= affectedCreature->valOfFeatures(StackFeature::SPELL_DAMAGE_REDUCTION, 2);
		ret /= 100;
	}
	else if (sp->earth && affectedCreature->hasFeatureOfType(StackFeature::SPELL_DAMAGE_REDUCTION, 3)) //earth spell & protection from earth
	{
		ret *= affectedCreature->valOfFeatures(StackFeature::SPELL_DAMAGE_REDUCTION, 3);
		ret /= 100;
	}

	//general spell dmg reduction
	if(sp->air && affectedCreature->hasFeatureOfType(StackFeature::SPELL_DAMAGE_REDUCTION, -1)) //air spell & protection from air
	{
		ret *= affectedCreature->valOfFeatures(StackFeature::SPELL_DAMAGE_REDUCTION, -1);
		ret /= 100;
	}

	return ret;
}

bool CGameHandler::makeCustomAction( BattleAction &ba )
{
	switch(ba.actionType)
	{
	case 1: //hero casts spell
		{
			CGHeroInstance *h = (ba.side) ? gs->getHero(gs->curB->hero2) : gs->getHero(gs->curB->hero1);
			CGHeroInstance *secondHero = (!ba.side) ? gs->getHero(gs->curB->hero2) : gs->getHero(gs->curB->hero1);
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
				|| (h->mana < s->costs[skill]) //not enough mana
				|| (ba.additionalInfo < 10) //it's adventure spell (not combat)
				|| (gs->curB->castSpells[ba.side]) //spell has been cast
				|| (secondHero && secondHero->hasBonusOfType(HeroBonus::SPELL_IMMUNITY, s->id)) //non - casting hero provides immunity for this spell 
				|| (gs->battleMaxSpellLevel() < s->level) //non - casting hero stops caster from casting this spell
				)
			{
				tlog2 << "Spell cannot be cast!\n";
				return false;
			}

			sendAndApply(&StartAction(ba)); //start spell casting

			//TODO: check resistances

			SpellCast sc;
			sc.side = ba.side;
			sc.id = ba.additionalInfo;
			sc.skill = skill;
			sc.tile = ba.destinationTile;
			sendAndApply(&sc);

			//calculating affected creatures for all spells
			std::set<CStack*> attackedCres = gs->curB->getAttackedCreatures(s, h, ba.destinationTile);

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
						BattleStackAttacked bsa;
						bsa.flags |= 2;
						bsa.effect = VLC->spellh->spells[ba.additionalInfo].mainEffectAnim;
						bsa.damageAmount = calculateSpellDmg(&VLC->spellh->spells[ba.additionalInfo], h, *it);
						bsa.stackAttacked = (*it)->ID;
						prepareAttacked(bsa,*it);
						si.stacks.insert(bsa);
					}
					sendAndApply(&si);
					break;
				}
			case 27: //shield 
			case 28: //air shield
			case 30: //protection from air
			case 31: //protection from fire
			case 32: //protection from water
			case 33: //protection from earth
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
			case 61: //forgetfulness
				{
					SetStackEffect sse;
					for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
					{
						sse.stacks.insert((*it)->ID);
					}
					sse.effect.id = ba.additionalInfo;
					sse.effect.level = h->getSpellSchoolLevel(s);
					sse.effect.turnsRemain = h->getPrimSkillLevel(2) + h->valOfBonuses(HeroBonus::SPELL_DURATION);
					sendAndApply(&sse);
					break;
				}
			case 56: //frenzy
				{
					SetStackEffect sse;
					for(std::set<CStack*>::iterator it = attackedCres.begin(); it != attackedCres.end(); ++it)
					{
						sse.stacks.insert((*it)->ID);
					}
					sse.effect.id = ba.additionalInfo;
					sse.effect.level = h->getSpellSchoolLevel(s);
					sse.effect.turnsRemain = 1;
					sendAndApply(&sse);
					break;
				}
			}
			sendAndApply(&EndAction());
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

void CGameHandler::showGarrisonDialog( int upobj, int hid, const boost::function<void()> &cb )
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
