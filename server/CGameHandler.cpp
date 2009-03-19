#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CTownHandler.h"
#include "../CGameState.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "../lib/Connection.h"
#include "../lib/VCMI_Lib.h"
#include "../map.h"
#include "CGameHandler.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp> //no i/o just types
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <fstream>

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


#define COMPLAIN(text) sendMessageToAll(text);tlog1<<text<<std::endl;

boost::mutex gsm;
ui32 CGameHandler::QID = 1;

CondSh<bool> battleMadeAction;
CondSh<BattleResult *> battleResult(NULL);

std::map<ui32, CFunctionList<void(ui32)> > callbacks; //question id => callback function - for selection dialogs

class CBaseForGHApply
{
public:
	virtual void applyOnGH(CGameHandler *gh, CConnection *c, void *pack) const =0; 
};
template <typename T> class CApplyOnGH : public CBaseForGHApply
{
public:
	void applyOnGH(CGameHandler *gh, CConnection *c, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->c = c;
		ptr->applyGh(gh);
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
		return (a->speed())>(b->speed());
	}
} cmpst ;

double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}
int getSchoolLevel(const CGHeroInstance *h, const CSpell *s)
{
	ui8 ret = 0;
	if(s->fire)
		ret = std::max(ret,h->getSecSkillLevel(14));
	if(s->air)
		ret = std::max(ret,h->getSecSkillLevel(15));
	if(s->water)
		ret = std::max(ret,h->getSecSkillLevel(16));
	if(s->earth)
		ret = std::max(ret,h->getSecSkillLevel(17));
	return ret;
}
void giveExp(BattleResult &r)
{
	r.exp[0] = 0;
	r.exp[1] = 0;
	for(std::set<std::pair<ui32,si32> >::iterator i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
	{
		r.exp[r.winner] += VLC->creh->creatures[i->first].hitPoints * i->second;
	}
}
//bool CGameState::checkFunc(int obid, std::string name)
//{
//	if (objscr.find(obid)!=objscr.end())
//	{
//		if(objscr[obid].find(name)!=objscr[obid].end())
//		{
//			return true;
//		}
//	}
//	return false;
//}
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
//void CGameHandler::handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script)
//{
//	std::vector<int> tempv = script->yourObjects();
//	for (unsigned i=0;i<tempv.size();i++)
//	{
//		(*mapa)[tempv[i]]=script;
//	}
//	cppscripts.insert(script);
//}
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
		if(hero->exp >= VLC->heroh->reqExp(hero->level+1)) //new level
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
				changeSecSkill(ID,hlu.skills.back(),1,false);
				sendAndApply(&hlu);
			}
			else //apply and send info
			{
				sendAndApply(&hlu);
			}
		}
	}
}

CCreatureSet takeCasualties(int color, const CCreatureSet &set, BattleInfo *bat)
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
			if(next->Morale() < 0)
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
			{
				BattleSetActiveStack sas;
				sas.stack = next->ID;
				sendAndApply(&sas);
				boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
				while(!battleMadeAction.data  &&  !battleResult.get()) //active stack hasn't made its action and battle is still going
					battleMadeAction.cond.wait(lock);
				battleMadeAction.data = false;
			}
			//we're after action, all results applied
			checkForBattleEnd(stacks); //check if this action ended the battle

			//check for good morale
			if(!vstd::contains(next->state,HAD_MORALE)  //only one extra move per turn possible
				&& !vstd::contains(next->state,DEFENDING)
				&& !vstd::contains(next->state,WAITING)
				&&  next->alive()
				&&  next->Morale() > 0
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
	if(cb)
		cb(battleResult.data);

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

	delete battleResult.data;

}
void CGameHandler::prepareAttacked(BattleStackAttacked &bsa, CStack *def)
{	
	bsa.killedAmount = bsa.damageAmount / def->creature->hitPoints;
	unsigned damageFirst = bsa.damageAmount % def->creature->hitPoints;

	if( def->firstHPleft <= damageFirst )
	{
		bsa.killedAmount++;
		bsa.newHP = def->firstHPleft + def->creature->hitPoints - damageFirst;
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
	bat.stackAttacking = att->ID;
	bat.bsa.stackAttacked = def->ID;
	bat.bsa.damageAmount = BattleInfo::calculateDmg(att, def, gs->getHero(att->attackerOwned ? gs->curB->hero1 : gs->curB->hero2), gs->getHero(def->attackerOwned ? gs->curB->hero1 : gs->curB->hero2), bat.shot());//counting dealt damage
	if(att->Luck() > 0  &&  rand()%24 < att->Luck())
	{
		bat.bsa.damageAmount *= 2;
		bat.bsa.flags |= 4;
	}
	prepareAttacked(bat.bsa,def);
}
void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	CPack *pack = NULL;
	try
	{
		while(!end2)
		{
			c >> pack;
			tlog5 << "Received client message of type " << typeid(*pack).name() << std::endl;
			CBaseForGHApply *apply = applier->apps[typeList.getTypeID(pack)];
			if(apply)
			{
				apply->applyOnGH(this,&c,pack);
				tlog5 << "Message successfully applied!\n";
			}
			else
			{
				tlog5 << "Message cannot be applied, cannot find applier!\n";
			}
			delete pack;
			pack = NULL;
		}
	}
	HANDLE_EXCEPTION(end2 = true);
handleConEnd:
	tlog1 << "Ended handling connection\n";
#undef SPELL_CAST_TEMPLATE_1
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
		if(path.second <= curStack->speed() && path.first.size() > 0)
		{
			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			sm.tile = path.first[0];
			sm.distance = path.second;
			sendAndApply(&sm);
		}
	}
	else //for non-flying creatures
	{
		int tilesToMove = std::max((int)(path.first.size() - curStack->speed()), 0);
		for(int v=path.first.size()-1; v>=tilesToMove; --v)
		{
			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->ID;
			sm.tile = path.first[v];
			sm.distance = path.second;
			sendAndApply(&sm);
		}
	}
}
CGameHandler::CGameHandler(void)
{
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

	/****************************LUA OBJECT SCRIPTS************************************************/
	//std::vector<std::string> * lf = CLuaHandler::searchForScripts("scripts/lua/objects"); //files
	//for (int i=0; i<lf->size(); i++)
	//{
	//	try
	//	{
	//		std::vector<std::string> * temp =  CLuaHandler::functionList((*lf)[i]);
	//		CLuaObjectScript * objs = new CLuaObjectScript((*lf)[i]);
	//		CLuaCallback::registerFuncs(objs->is);
	//		//objs
	//		for (int j=0; j<temp->size(); j++)
	//		{
	//			int obid ; //obj ID
	//			int dspos = (*temp)[j].find_first_of('_');
	//			obid = atoi((*temp)[j].substr(dspos+1,(*temp)[j].size()-dspos-1).c_str());
	//			std::string fname = (*temp)[j].substr(0,dspos);
	//			if (skrypty->find(obid)==skrypty->end())
	//				skrypty->insert(std::pair<int, std::map<std::string, CObjectScript*> >(obid,std::map<std::string,CObjectScript*>()));
	//			(*skrypty)[obid].insert(std::pair<std::string, CObjectScript*>(fname,objs));
	//		}
	//		delete temp;
	//	}HANDLE_EXCEPTION
	//}

	//delete lf;

	for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		states.addPlayer(i->first);
}

bool evntCmp(const CMapEvent *a, const CMapEvent *b)
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
			hth.mana = std::max(h->mana,std::min(h->mana+1+h->getSecSkillLevel(8), h->manaLimit())); //hero regains 1 mana point + mysticism lvel
			n.heroes.insert(hth);
			
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
		}
		for(std::vector<CGTownInstance *>::iterator j=i->second.towns.begin();j!=i->second.towns.end();j++)//handle towns
		{
			if(vstd::contains((**j).builtBuildings,15)) //there is resource silo
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
				sac.creatures = (**j).strInfo.creatures;
				for(int k=0;k<CREATURES_PER_TOWN;k++) //creature growths
				{
					if((**j).creatureDwelling(k))//there is dwelling (k-level)
						sac.creatures[k] += (**j).creatureGrowth(k);
				}
				n.cres.push_back(sac);
			}
			if((gs->day) && i->first<PLAYER_LIMIT)//not the first day and town not neutral
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
			gsm.lock();
			connections[pom] = cc;
			gsm.unlock();
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

	/****************************SCRIPTS************************************************/
	//std::map<int, std::map<std::string, CObjectScript*> > * skrypty = &objscr; //alias for easier access
	/****************************C++ OBJECT SCRIPTS************************************************/
	//std::map<int,CCPPObjectScript*> scripts;
	//CScriptCallback * csc = new CScriptCallback();
	//csc->gh = this;
	//handleCPPObjS(&scripts,new CVisitableOPH(csc));
	//handleCPPObjS(&scripts,new CVisitableOPW(csc));
	//handleCPPObjS(&scripts,new CPickable(csc));
	//handleCPPObjS(&scripts,new CMines(csc));
	//handleCPPObjS(&scripts,new CTownScript(csc));
	//handleCPPObjS(&scripts,new CHeroScript(csc));
	//handleCPPObjS(&scripts,new CMonsterS(csc));
	//handleCPPObjS(&scripts,new CCreatureGen(csc));
	//handleCPPObjS(&scripts,new CTeleports(csc));

	/****************************INITIALIZING OBJECT SCRIPTS************************************************/
	//std::string temps("newObject");
	//for (unsigned i=0; i<gs->map->objects.size(); i++)
	//{
		//c++ scripts
		//if (scripts.find(gs->map->objects[i]->ID) != scripts.end())
		//{
		//	gs->map->objects[i]->state = scripts[gs->map->objects[i]->ID];
		//	gs->map->objects[i]->state->newObject(gs->map->objects[i]->id);
		//}
		//else 
		//{
		//	gs->map->objects[i]->state = NULL;
		//}

		//// lua scripts
		//if(checkFunc(map->objects[i]->ID,temps))
		//	(*skrypty)[map->objects[i]->ID][temps]->newObject(map->objects[i]);
	//}

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
				*connections[i->first] << &yt;    
			}

			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(states.mx);
			while(states.players[i->first].makingTurn && !end2)
			{
				boost::posix_time::time_duration p;
				p = boost::posix_time::milliseconds(200);
#ifdef _MSC_VER
				states.cv.timed_wait(lock,p); 
#else
				boost::xtime time={0,0};
				time.nsec = static_cast<boost::xtime::xtime_nsec_t>(p.total_nanoseconds());
				states.cv.timed_wait(lock,time);
#endif
			}
		}
	}
}

namespace CGH
{
	using namespace std;
	void readItTo(ifstream & input, vector< vector<int> > & dest)
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
		
		//base luck/morale calculations
		//TODO: check if terrain is native, add bonuses for neutral stacks, bonuses from town
		if(hero1)
		{
			stacks.back()->morale = hero1->getCurrentMorale(i->first,false);
			stacks.back()->luck = hero1->getCurrentLuck(i->first,false);
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
			stacks.back()->morale = hero2->getCurrentMorale(i->first,false);
			stacks.back()->luck = hero2->getCurrentLuck(i->first,false);
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
	//war machiens added
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
		if(g->second.allowedTerrains[terType] == '1')
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
		if(stacks[b]->alive())
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
	for(int i=0; i<std::min(t->mageGuildLevel(),h->getSecSkillLevel(7)+4);i++)
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
void CGameHandler::removeObject(int objid)
{
	RemoveObject ro;
	ro.id = objid;
	sendAndApply(&ro);
}

void CGameHandler::setAmount(int objid, ui32 val)
{
	SetObjectProperty sop(objid,3,val);
	sendAndApply(&sop);
}

void CGameHandler::moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker)
{
	bool blockvis = false;
	const CGHeroInstance *h = getHero(hid);

	if(!h  ||  asker != 255 && (instant  ||   h->getOwner() != gs->currentPlayer) //not turn of that hero or player can't simply teleport hero (at least not with this function)
	  )
	{
		tlog1 << "Illegal call to move hero!\n";
		return;
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
	tmh.result = 0;
	tmh.movePoints = h->movement;

	//check if destination tile is available
	if(	t.tertype == rock   
		|| (!h->canWalkOnSea() && t.tertype == water)
		|| (t.blocked && !t.visitable) //tile is blocked andnot visitable
	  )
	{
		tlog2 << "Cannot move hero, destination tile is blocked!\n";
		sendAndApply(&tmh);
		return;
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
			return;
		}

		//check if there is blocking visitable object
		blockvis = false;
		tmh.movePoints = std::max(si32(0),h->movement-cost); //take move points
		BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
		{
			if(obj->blockVisit)
			{
				blockvis = true;
				break;
			}
		}
		//we start moving
		if(blockvis)//interaction with blocking object (like resources)
		{
			sendAndApply(&tmh); 
			//failed to move to that tile but we visit object
			BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
			{
				if (obj->blockVisit)
				{
					obj->onHeroVisit(h);
				}
			}
			tlog5 << "Blocking visit at " << hmpos << std::endl;
			return;
		}
		else //normal move
		{
			tmh.result = 1;

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
				obj->onHeroVisit(h);
			}
		}
		tlog5 << "Movement end!\n";
	}
	else //instant move - teleportation
	{
		BOOST_FOREACH(CGObjectInstance* obj, t.blockingObjects)
		{
			if(obj->ID==HEROI_TYPE)
			{
				if(obj->tempOwner==h->tempOwner) 
					return;//TODO: exchange
				//TODO: check for ally
				CGHeroInstance *dh = static_cast<CGHeroInstance *>(obj);
				startBattleI(&h->army,&dh->army,dst,h,dh,0);
				return;
			}
		}
		tmh.result = instant+1;
		getTilesInRange(tmh.fowRevealed,h->getSightCenter()+(tmh.end-tmh.start),h->getSightRadious(),h->tempOwner,1);
		sendAndApply(&tmh);
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
void CGameHandler::showYesNoDialog( YesNoDialog *iw, const CFunctionList<void(ui32)> &callback )
{
	ask(iw,iw->player,callback);
}
void CGameHandler::showSelectionDialog(SelectionDialog *iw, const CFunctionList<void(ui32)> &callback)
{
	ask(iw,iw->player,callback);
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
	sr.val = (gs->players.find(player)->second.resources[which]+val);
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

	SetHeroArtifacts sha;
	sha.hid = hid;
	sha.artifacts = h->artifacts;
	sha.artifWorn = h->artifWorn;
	if(position<0)
	{
		if(position == -2)
		{
			int i;
			for(i=0; i<VLC->arth->artifacts[artid].possibleSlots.size(); i++) //try to put artifact into first avaialble slot
			{
				if( !vstd::contains(sha.artifWorn,VLC->arth->artifacts[artid].possibleSlots[i]) )
				{
					sha.artifWorn[VLC->arth->artifacts[artid].possibleSlots[i]] = artid;
					break;
				}
			}
			if(i==VLC->arth->artifacts[artid].possibleSlots.size()) //if haven't find proper slot, use backpack
				sha.artifacts.push_back(artid);
		}
		else //should be -1 => putartifact into backpack
		{
			sha.artifacts.push_back(artid);
		}
	}
	else
	{
		if(!vstd::contains(sha.artifWorn,ui16(position)))
			sha.artifWorn[position] = artid;
		else
			sha.artifacts.push_back(artid);
	}
	sendAndApply(&sha);
}

void CGameHandler::startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb) //use hero=NULL for no hero
{
	boost::thread(boost::bind(&CGameHandler::startBattle,this,*(CCreatureSet *)army1,*(CCreatureSet *)army2,tile,(CGHeroInstance *)hero1,(CGHeroInstance *)hero2,cb));
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

void CGameHandler::applyAndAsk( Query * sel, ui8 player, boost::function<void(ui32)> &callback )
{
	gsm.lock();
	sel->id = QID;
	callbacks[QID] = callback;
	states.addQuery(player,QID);
	QID++; 
	sendAndApply(sel);
	gsm.unlock();
}

void CGameHandler::ask( Query * sel, ui8 player, const CFunctionList<void(ui32)> &callback )
{
	gsm.lock();
	sel->id = QID;
	callbacks[QID] = callback;
	states.addQuery(player,QID);
	sendToAllClients(sel);
	QID++; 
	gsm.unlock();
}

void CGameHandler::sendToAllClients( CPackForClient * info )
{
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
		tlog0 << "Serializing game info...\n";
		CSaveFile save(std::string("Games") + PATHSEPARATOR + fname + ".vlgm1");
		char hlp[8] = "VCMISVG";
		save << hlp << version << static_cast<CMapHeader&>(*gs->map) << gs->scenarioOps->difficulty << *VLC << gs;
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
	exit(0);
}

void CGameHandler::arrangeStacks(si32 id1, si32 id2, ui8 what, ui8 p1, ui8 p2, si32 val)
{
	CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id1]),
		*s2 = static_cast<CArmedInstance*>(gs->map->objects[id2]);
	CCreatureSet temp1 = s1->army, temp2 = s2->army,
		&S1 = temp1, &S2 = (s1!=s2)?(temp2):(temp1);

	if(what==1) //swap
	{
		std::swap(S1.slots[p1],S2.slots[p2]);

		if(!S1.slots[p1].second)
			S1.slots.erase(p1);
		if(!S2.slots[p2].second)
			S2.slots.erase(p2);
	}
	else if(what==2)//merge
	{
		if(S1.slots[p1].first != S2.slots[p2].first) //not same creature
		{
			COMPLAIN("Cannot merge different creatures stacks!");
			return; 
		}

		S2.slots[p2].second += S1.slots[p1].second;
		S1.slots.erase(p1);
	}
	else if(what==3) //split
	{
		if(	vstd::contains(S2.slots,p2)		//dest. slot not free
			|| !vstd::contains(S1.slots,p1)	//no creatures to split
			|| S1.slots[p1].second < val		//not enough creatures
			|| val<1							//val must be positive
		) 
		{
			COMPLAIN("Cannot split that stack!");
			return; 
		}

		S2.slots[p2].first = S1.slots[p1].first;
		S2.slots[p2].second = val;
		S1.slots[p1].second -= val;
		if(!S1.slots[p1].second) //if we've moved all creatures
			S1.slots.erase(p1); 
	}
	if((s1->needsLastStack() && !S1.slots.size()) //it's not allowed to take last stack from hero army!
		|| (s2->needsLastStack() && !S2.slots.size())
	)
	{
		COMPLAIN("Cannot take the last stack!");
		return; //leave without applying changes to garrison
	}

	SetGarrisons sg;
	sg.garrs[id1] = S1;
	if(s1 != s2)
		sg.garrs[id2] = S2;
	sendAndApply(&sg);
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

void CGameHandler::disbandCreature(si32 id, ui8 pos)
{
	CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id]);
	if(!vstd::contains(s1->army.slots,pos))
	{
		COMPLAIN("Illegal call to disbandCreature - no such stack in army!");
		return;
	}
	s1->army.slots.erase(pos);
	SetGarrisons sg;
	sg.garrs[id] = s1->army;
	sendAndApply(&sg);
}

void CGameHandler::buildStructure(si32 tid, si32 bid)
{
	CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[tid]);
	CBuilding * b = VLC->buildh->buildings[t->subID][bid];

	if(gs->canBuildStructure(t,bid) != 7)
	{
		COMPLAIN("Cannot build that building!");
		return;
	}

	NewStructures ns;
	ns.tid = tid;
	if(bid>36) //upg dwelling
	{
		if(t->getHordeLevel(0) == (bid-37))
			ns.bid.insert(19);
		else if(t->getHordeLevel(1) == (bid-37))
			ns.bid.insert(25);
	}
	else if(bid >= 30) //bas. dwelling
	{
		SetAvailableCreatures ssi;
		ssi.tid = tid;
		ssi.creatures = t->strInfo.creatures;
		ssi.creatures[bid-30] = VLC->creh->creatures[t->town->basicCreatures[bid-30]].growth;
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
}

void CGameHandler::sendMessageToAll( const std::string &message )
{
	SystemMessage sm;
	sm.text = message;
	sendToAllClients(&sm);
}

void CGameHandler::recruitCreatures( si32 objid, ui32 crid, ui32 cram )
{
	si32 ser = -1;
	CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[objid]);

	//verify
	bool found = false;
	typedef std::pair<const int,int> Parka;
	for(std::map<si32,ui32>::iterator av = t->strInfo.creatures.begin();  av!=t->strInfo.creatures.end();  av++)
	{
		if(	(   found  = (crid == t->town->basicCreatures[av->first])   ) //creature is available among basic cretures
			|| (found  = (crid == t->town->upgradedCreatures[av->first]))			)//creature is available among upgraded cretures
		{
			cram = std::min(cram,av->second); //reduce recruited amount up to available amount
			ser = av->first;
			break;
		}
	}
	int slot = t->army.getSlotFor(crid);

	if(!found ||	//no such creature
		cram > VLC->creh->creatures[crid].maxAmount(gs->getPlayer(t->tempOwner)->resources) ||  //lack of resources
		cram<=0	||
		slot<0	) 
		return;

	//recruit
	SetResources sr;
	sr.player = t->tempOwner;
	for(int i=0;i<RESOURCE_QUANTITY;i++)
		sr.res[i]  =  gs->getPlayer(t->tempOwner)->resources[i] - (VLC->creh->creatures[crid].cost[i] * cram);

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = t->strInfo.creatures;
	sac.creatures[ser] -= cram;

	SetGarrisons sg;
	sg.garrs[objid] = t->army;
	if(sg.garrs[objid].slots.find(slot) == sg.garrs[objid].slots.end()) //take a free slot
	{
		sg.garrs[objid].slots[slot] = std::make_pair(crid,cram);
	}
	else //add creatures to a already existing stack
	{
		sg.garrs[objid].slots[slot].second += cram;
	}
	sendAndApply(&sr); 
	sendAndApply(&sac);
	sendAndApply(&sg);
}

void CGameHandler::upgradeCreature( ui32 objid, ui8 pos, ui32 upgID )
{
	CArmedInstance *obj = static_cast<CArmedInstance*>(gs->map->objects[objid]);
	UpgradeInfo ui = gs->getUpgradeInfo(obj,pos);
	int player = obj->tempOwner;
	int crQuantity = obj->army.slots[pos].second;

	//check if upgrade is possible
	if(ui.oldID<0 || !vstd::contains(ui.newID,upgID)) 
		return;

	//check if player has enough resources
	for(int i=0;i<ui.cost.size();i++)
	{
		for (std::set<std::pair<int,int> >::iterator j=ui.cost[i].begin(); j!=ui.cost[i].end(); j++)
		{
			if(gs->getPlayer(player)->resources[j->first] < j->second*crQuantity)
				return;
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
}

void CGameHandler::garrisonSwap(si32 tid)
{
	CGTownInstance *town = gs->getTown(tid);
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies
	{
		CCreatureSet csn = town->visitingHero->army, cso = town->army;
		while(!cso.slots.empty())//while there are unmoved creatures
		{
			int pos = csn.getSlotFor(cso.slots.begin()->second.first);
			if(pos<0)
				return;
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
	}					
	else if (town->garrisonHero && !town->visitingHero) //move hero out of the garrison
	{
		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = -1;
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(&intown);

		//town will be empty
		SetGarrisons sg;
		sg.garrs[tid] = CCreatureSet();
		sendAndApply(&sg);
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
	}
	else
	{
		COMPLAIN("Cannot swap garrison hero!");
	}
}

void CGameHandler::swapArtifacts( si32 hid1, si32 hid2, ui16 slot1, ui16 slot2 )
{
	CGHeroInstance *h1 = gs->getHero(hid1), *h2 = gs->getHero(hid2);
	if((distance(h1->pos,h2->pos) > 1.0)   ||   (h1->tempOwner != h2->tempOwner))
		return;
	int a1=h1->getArtAtPos(slot1), a2=h2->getArtAtPos(slot2);

	h2->setArtAtPos(slot2,a1);
	h1->setArtAtPos(slot1,a2);
	SetHeroArtifacts sha;
	sha.hid = hid1;
	sha.artifacts = h1->artifacts;
	sha.artifWorn = h1->artifWorn;
	sendAndApply(&sha);
	if(hid1 != hid2)
	{
		sha.hid = hid2;
		sha.artifacts = h2->artifacts;
		sha.artifWorn = h2->artifWorn;
		sendAndApply(&sha);
	}
}

void CGameHandler::buyArtifact( ui32 hid, si32 aid )
{
	CGHeroInstance *hero = gs->getHero(hid);
	CGTownInstance *town = hero->visitedTown;
	if(aid==0) //spellbook
	{
		if(!vstd::contains(town->builtBuildings,si32(0)))
			return;
		SetResource sr;
		sr.player = hero->tempOwner;
		sr.resid = 6;
		sr.val = gs->getPlayer(hero->getOwner())->resources[6] - 500;
		sendAndApply(&sr);

		SetHeroArtifacts sha;
		sha.hid = hid;
		sha.artifacts = hero->artifacts;
		sha.artifWorn = hero->artifWorn;
		sha.artifWorn[17] = 0;
		sendAndApply(&sha);

		giveSpells(town,hero);
	}
	else if(aid < 7  &&  aid > 3) //war machine
	{
		int price = VLC->arth->artifacts[aid].price;
		if(vstd::contains(hero->artifWorn,ui16(9+aid))  //hero already has this machine
			|| !vstd::contains(town->builtBuildings,si32(16)) //no blackismith
			|| gs->getPlayer(hero->getOwner())->resources[6] < price //no gold
			|| town->town->warMachine!= aid ) //this machine is not available here (//TODO: support ballista yard in stronghold)
		{
			return;
		}
		SetResource sr;
		sr.player = hero->tempOwner;
		sr.resid = 6;
		sr.val = gs->getPlayer(hero->getOwner())->resources[6] - price;
		sendAndApply(&sr);

		SetHeroArtifacts sha;
		sha.hid = hid;
		sha.artifacts = hero->artifacts;
		sha.artifWorn = hero->artifWorn;
		sha.artifWorn[9+aid] = aid;
		sendAndApply(&sha);
	}
}

void CGameHandler::tradeResources( ui32 val, ui8 player, ui32 id1, ui32 id2 )
{
	val = std::min(si32(val),gs->getPlayer(player)->resources[id1]);
	double uzysk = (double)gs->resVals[id1] * val * gs->getMarketEfficiency(player);
	uzysk /= gs->resVals[id2];
	SetResource sr;
	sr.player = player;
	sr.resid = id1;
	sr.val = gs->getPlayer(player)->resources[id1] - val;
	sendAndApply(&sr);

	sr.resid = id2;
	sr.val = gs->getPlayer(player)->resources[id2] + (int)uzysk;
	sendAndApply(&sr);
}

void CGameHandler::setFormation( si32 hid, ui8 formation )
{
	gs->getHero(hid)->army.formation = formation;
}

void CGameHandler::hireHero( ui32 tid, ui8 hid )
{
	CGTownInstance *t = gs->getTown(tid);
	if(!vstd::contains(t->builtBuildings,5) //no tavern in the town
		|| gs->getPlayer(t->tempOwner)->resources[6]<2500 //not enough gold
		|| t->visitingHero //there is visiting hero - no place
		|| gs->getPlayer(t->tempOwner)->heroes.size()>7 //8 hero limit
		)
		return;
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
}

void CGameHandler::queryReply( ui32 qid, ui32 answer )
{
	gsm.lock();
	CFunctionList<void(ui32)> callb = callbacks[qid];
	callbacks.erase(qid);
	gsm.unlock();
	callb(answer);
}

void CGameHandler::makeBattleAction( BattleAction &ba )
{
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
			}

			if(!stackAtEnd)
			{
				tlog3 << "There is no stack on " << ba.additionalInfo << " tile (no attack)!";
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
				break;
			}

			//attack
			BattleAttack bat;
			prepareAttack(bat,curStack,stackAtEnd);
			sendAndApply(&bat);

			//counterattack
			if(!vstd::contains(curStack->abilities,NO_ENEMY_RETALIATION)
				&& stackAtEnd->alive()
				&& stackAtEnd->counterAttacks
				&& !vstd::contains(stackAtEnd->abilities, SIEGE_WEAPON)) //TODO: support for multiple retaliatons per turn
			{
				prepareAttack(bat,stackAtEnd,curStack);
				bat.flags |= 2;
				sendAndApply(&bat);
			}

			//second attack
			if(vstd::contains(curStack->abilities,TWICE_ATTACK)
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
				|| !vstd::contains(curStack->abilities,SHOOTER) //our stack is shooting unit
				)
				break;
			for(int g=0; g<curStack->effects.size(); ++g)
			{
				if(61 == curStack->effects[g].id) //forgetfulness
					break;
			}

			sendAndApply(&StartAction(ba)); //start shooting

			BattleAttack bat;
			prepareAttack(bat,curStack,destStack);
			bat.flags |= 1;
			sendAndApply(&bat);

			if(vstd::contains(curStack->abilities,TWICE_ATTACK) //if unit shots twice let's make another shot
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

void CGameHandler::makeCustomAction( BattleAction &ba )
{
	switch(ba.actionType)
	{
	case 1: //hero casts spell
		{
			CGHeroInstance *h = (ba.side) ? gs->getHero(gs->curB->hero2) : gs->getHero(gs->curB->hero1);
			if(!h)
			{
				tlog2 << "Wrong caster!\n";
				return;
			}
			if(ba.additionalInfo >= VLC->spellh->spells.size())
			{
				tlog2 << "Wrong spell id (" << ba.additionalInfo << ")!\n";
				return;
			}

			CSpell *s = &VLC->spellh->spells[ba.additionalInfo];
			ui8 skill = 0; //skill level

			if(s->fire)
				skill = std::max(skill,h->getSecSkillLevel(14));
			if(s->air)
				skill = std::max(skill,h->getSecSkillLevel(15));
			if(s->water)
				skill = std::max(skill,h->getSecSkillLevel(16));
			if(s->earth)
				skill = std::max(skill,h->getSecSkillLevel(17));

			//TODO: skill level may be different on special terrain

			if(   !(vstd::contains(h->spells,ba.additionalInfo)) //hero doesn't know this spell 
				|| (h->mana < s->costs[skill]) //not enough mana
				|| (ba.additionalInfo < 10) //it's adventure spell (not combat)
				|| (gs->curB->castedSpells[ba.side])     
				)
			{
				tlog2 << "Spell cannot be casted!\n";
				return;
			}

			sendAndApply(&StartAction(ba)); //start spell casting

			//TODO: check resistances

#define SPELL_CAST_TEMPLATE_1(NUMBER, DURATION) SetStackEffect sse; \
if(getSchoolLevel(h,s) < 3)  /*not expert */ \
			{ \
			sse.stack = gs->curB->getStackT(ba.destinationTile)->ID; \
			sse.effect.id = (NUMBER); \
			sse.effect.level = getSchoolLevel(h,s); \
			sse.effect.turnsRemain = (DURATION); /*! - any duration */ \
			sendAndApply(&sse); \
			} \
			else \
			{ \
			for(int it=0; it<gs->curB->stacks.size(); ++it) \
			{ \
			/*if it's non negative spell and our unit or non positive spell and hostile unit */ \
			if((VLC->spellh->spells[ba.additionalInfo].positiveness >= 0 && gs->curB->stacks[it]->owner == h->tempOwner) \
			||(VLC->spellh->spells[ba.additionalInfo].positiveness <= 0 && gs->curB->stacks[it]->owner != h->tempOwner ) \
			) \
			{ \
			sse.stack = gs->curB->stacks[it]->ID; \
			sse.effect.id = (NUMBER); \
			sse.effect.level = getSchoolLevel(h,s); \
			sse.effect.turnsRemain = (DURATION); \
			sendAndApply(&sse); \
			} \
			} \
			}

			SpellCasted sc;
			sc.side = ba.side;
			sc.id = ba.additionalInfo;
			sc.skill = skill;
			sc.tile = ba.destinationTile;
			sendAndApply(&sc);
			switch(ba.additionalInfo) //spell id
			{
			case 15: //magic arrow
				{
					CStack * attacked = gs->curB->getStackT(ba.destinationTile);
					if(!attacked) break;
					BattleStackAttacked bsa;
					bsa.flags |= 2;
					bsa.effect = 64;
					bsa.damageAmount = h->getPrimSkillLevel(2) * 10  +  s->powers[getSchoolLevel(h,s)]; 
					bsa.stackAttacked = attacked->ID;
					prepareAttacked(bsa,attacked);
					sendAndApply(&bsa);
					break;
				}
			case 16: //ice bolt
				{
					CStack * attacked = gs->curB->getStackT(ba.destinationTile);
					if(!attacked) break;
					BattleStackAttacked bsa;
					bsa.flags |= 2;
					bsa.effect = 46;
					bsa.damageAmount = h->getPrimSkillLevel(2) * 20  +  s->powers[getSchoolLevel(h,s)]; 
					bsa.stackAttacked = attacked->ID;
					prepareAttacked(bsa,attacked);
					sendAndApply(&bsa);
					break;
				}
			case 17: //lightning bolt
				{
					CStack * attacked = gs->curB->getStackT(ba.destinationTile);
					if(!attacked) break;
					BattleStackAttacked bsa;
					bsa.flags |= 2;
					bsa.effect = 38;
					bsa.damageAmount = h->getPrimSkillLevel(2) * 25  +  s->powers[getSchoolLevel(h,s)]; 
					bsa.stackAttacked = attacked->ID;
					prepareAttacked(bsa,attacked);
					sendAndApply(&bsa);
					break;
				}
			case 18: //implosion
				{
					CStack * attacked = gs->curB->getStackT(ba.destinationTile);
					if(!attacked) break;
					BattleStackAttacked bsa;
					bsa.flags |= 2;
					bsa.effect = 10;
					bsa.damageAmount = h->getPrimSkillLevel(2) * 75  +  s->powers[getSchoolLevel(h,s)]; 
					bsa.stackAttacked = attacked->ID;
					prepareAttacked(bsa,attacked);
					sendAndApply(&bsa);
					break;
				}
			case 27: //shield
				{
					SPELL_CAST_TEMPLATE_1(27, h->getPrimSkillLevel(2))
						break;
				}
			case 28: //air shield
				{
					SPELL_CAST_TEMPLATE_1(28, h->getPrimSkillLevel(2))
						break;
				}
			case 41: //bless
				{
					SPELL_CAST_TEMPLATE_1(41, h->getPrimSkillLevel(2))
						break;
				}
			case 42: //curse
				{
					SPELL_CAST_TEMPLATE_1(42, h->getPrimSkillLevel(2))
						break;
				}
			case 43: //bloodlust
				{
					SPELL_CAST_TEMPLATE_1(43, h->getPrimSkillLevel(2))
						break;
				}
			case 45: //weakness
				{
					SPELL_CAST_TEMPLATE_1(45, h->getPrimSkillLevel(2))
						break;
				}
			case 46: //stone skin
				{
					SPELL_CAST_TEMPLATE_1(46, h->getPrimSkillLevel(2))
						break;
				}
			case 48: //prayer
				{
					SPELL_CAST_TEMPLATE_1(48, h->getPrimSkillLevel(2))
						break;
				}
			case 49: //mirth
				{
					SPELL_CAST_TEMPLATE_1(49, h->getPrimSkillLevel(2))
						break;
				}
			case 50: //sorrow
				{
					SPELL_CAST_TEMPLATE_1(50, h->getPrimSkillLevel(2))
						break;
				}
			case 51: //fortune
				{
					SPELL_CAST_TEMPLATE_1(51, h->getPrimSkillLevel(2))
						break;
				}
			case 52: //misfortune
				{
					SPELL_CAST_TEMPLATE_1(52, h->getPrimSkillLevel(2))
						break;
				}
			case 53: //haste
				{
					SPELL_CAST_TEMPLATE_1(53, h->getPrimSkillLevel(2))
						break;
				}
			case 54: //slow
				{
					SPELL_CAST_TEMPLATE_1(54, h->getPrimSkillLevel(2))
						break;
				}
			case 56: //frenzy
				{
					SPELL_CAST_TEMPLATE_1(56, 1)
						break;
				}
			case 61: //forgetfulness
				{
					SPELL_CAST_TEMPLATE_1(61, h->getPrimSkillLevel(2))
						break;
				}
			}
			sendAndApply(&EndAction());
		}
	}
}

void CGameHandler::handleTimeEvents()
{
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
						iw.components.push_back(Component(Component::RESOURCE,i,ev->resources[i],0));
						sr.res[i] += ev->resources[i];
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