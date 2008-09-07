#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/bind.hpp>
#include "CGameHandler.h"
#include "CScriptCallback.h"
#include "../CLua.h"
#include "../CGameState.h"
#include "../StartInfo.h"
#include "../map.h"
#include "../lib/NetPacks.h"
#include "../lib/Connection.h"
#include "../hch/CArtHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CHeroHandler.h"
#include "boost/date_time/posix_time/posix_time_types.hpp" //no i/o just types
#include "../lib/VCMI_Lib.h"
#include "../lib/CondSh.h"
#ifndef _MSC_VER
#include <boost/thread/xtime.hpp>
#endif
extern bool end2;
#include "../lib/BattleAction.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define NEW_ROUND 		BattleNextRound bnr;\
		bnr.round = gs->curB->round + 1;\
		sendAndApply(&bnr);

boost::mutex gsm;
ui32 CGameHandler::QID = 1;

CondSh<bool> battleMadeAction;
CondSh<BattleResult *> battleResult(NULL);

std::map<ui32, CFunctionList<void(ui32)> > callbacks; //question id => callback function - for selection dialogs

class CMP_stack
{
public:
	bool operator ()(const CStack* a, const CStack* b)
	{
		return (a->creature->speed)>(b->creature->speed);
	}
} cmpst ;

double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
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
void CGameHandler::handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script)
{
	std::vector<int> tempv = script->yourObjects();
	for (unsigned i=0;i<tempv.size();i++)
	{
		(*mapa)[tempv[i]]=script;
	}
	cppscripts.insert(script);
}
template <typename T>
void callWith(std::vector<T> args, boost::function<void(T)> fun, ui32 which)
{
	fun(args[which]);
}

void CGameHandler::changeSecSkill(int ID, ui16 which, int val, bool abs)
{
	SetSecSkill sps;
	sps.id = ID;
	sps.which = which;
	sps.abs = abs;
	sps.val = val;
	sendAndApply(&sps);
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
			std::cout << hero->name <<" got level "<<hero->level<<std::endl;
			int r = rand()%100, pom=0, x=0;
			int std::pair<int,int>::*g  =  (hero->level>9) ? (&std::pair<int,int>::second) : (&std::pair<int,int>::first);
			for(;x<PRIMARY_SKILLS;x++)
			{
				pom += hero->type->heroClass->primChance[x].*g;
				if(r<pom)
					break;
			}
			std::cout << "Bohater dostaje umiejetnosc pierwszorzedna " << x << " (wynik losowania "<<r<<")"<<std::endl; 
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
				if(hero->secSkills[i].second < 2)
					basicAndAdv.insert(hero->secSkills[i].first);
				else
					expert.insert(hero->secSkills[i].first);
				none.erase(hero->secSkills[i].first);
			}
			if(hero->secSkills.size() < hero->type->heroClass->skillLimit) //free skill slot
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(none)); //new skill
			}
			else
			{
				int s = hero->type->heroClass->chooseSecSkill(basicAndAdv);
				hlu.skills.push_back(s);
				basicAndAdv.erase(s);
			}
			if(basicAndAdv.size())
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(basicAndAdv)); //new skill
			}
			else if(hero->secSkills.size() < hero->type->heroClass->skillLimit)
			{
				hlu.skills.push_back(hero->type->heroClass->chooseSecSkill(none)); //new skill
			}
			boost::function<void(ui32)> callback = boost::function<void(ui32)>(boost::bind(callWith<ui16>,hlu.skills,boost::function<void(ui16)>(boost::bind(&CGameHandler::changeSecSkill,this,ID,_1,1,0)),_1));
			applyAndAsk(&hlu,hero->tempOwner,callback); //call changeSecSkill with appropriate args when client responds
		}
	}
}

void CGameHandler::startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2)
{
	BattleInfo *curB = new BattleInfo;
	setupBattle(curB, tile, army1, army2, hero1, hero2); //battle start
	NEW_ROUND;


	//tactic round
	{
		NEW_ROUND;
		if( (hero1 && hero1->getSecSkillLevel(19)>=0) || 
			( hero2 && hero2->getSecSkillLevel(19)>=0)  )//someone has tactics
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
		for(unsigned i=0;i<stacks.size() && !battleResult.get();i++)
		{
			if(!stacks[i]->alive) continue;//indicates imposiibility of making action for this dead unit
			BattleSetActiveStack sas;
			sas.stack = stacks[i]->ID;
			sendAndApply(&sas);

			//wait for response about battle action

			boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
			while(!battleMadeAction.data)
				battleMadeAction.cond.wait(lock);
			battleMadeAction.data = false;
		}
	}

	//unblock engaged players
	if(hero1->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero1->tempOwner,&PlayerStatus::engagedIntoBattle,false);
	if(hero2 && hero2->tempOwner<PLAYER_LIMIT)
		states.setFlag(hero2->tempOwner,&PlayerStatus::engagedIntoBattle,false);

	//end battle, remove all info, free memory
	sendAndApply(battleResult.data);
	delete battleResult.data;
	//for(int i=0;i<stacks.size();i++)
	//	delete stacks[i];
	//delete curB;
	//curB = NULL;
}
void prepareAttack(BattleAttack &bat, CStack *att, CStack *def)
{
	bat.stackAttacking = att->ID;
	bat.bsa.stackAttacked = def->ID;
	bat.bsa.damageAmount = BattleInfo::calculateDmg(att, def);//counting dealt damage

	//applying damages
	bat.bsa.killedAmount = bat.bsa.damageAmount / def->creature->hitPoints;
	unsigned damageFirst = bat.bsa.damageAmount % def->creature->hitPoints;

	if( def->firstHPleft <= damageFirst )
	{
		bat.bsa.killedAmount++;
		bat.bsa.newHP = def->firstHPleft + def->creature->hitPoints - damageFirst;
	}
	else
	{
		bat.bsa.newHP = def->firstHPleft - damageFirst;
	}

	if(def->amount <= bat.bsa.killedAmount) //stack killed
	{
		bat.bsa.newAmount = 0;
		bat.bsa.flags |= 1;
	}
	else
	{
		bat.bsa.newAmount = def->amount - bat.bsa.killedAmount;
	}
}
void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	try
	{
		ui16 pom;
		while(!end2)
		{
			c >> pom;
			bool blockvis = false;
			switch(pom)
			{
			case 100: //my interface ended its turn
				{
					states.setFlag(gs->currentPlayer,&PlayerStatus::makingTurn,false);
					break;
				}
			case 500:
				{
					si32 id;
					c >> id;
					RemoveObject rh(id);
					sendAndApply(&rh);
					break;
				}
			case 501://interface wants to move hero
				{
					int3 start, end;
					si32 id;
					c >> id >> start >> end;
					int3 hmpos = end + int3(-1,0,0);
					TerrainTile t = gs->map->terrain[hmpos.x][hmpos.y][hmpos.z];
					CGHeroInstance *h = static_cast<CGHeroInstance *>(gs->map->objects[id]);
					int cost = (int)((double)h->getTileCost(t.tertype,t.malle,t.nuine) * distance(start,end));

					TryMoveHero tmh;
					tmh.id = id;
					tmh.start = start;
					tmh.end = end;
					tmh.result = 0;
					tmh.movePoints = h->movement;

					if((h->getOwner() != gs->currentPlayer) || //not turn of that hero
						(distance(start,end)>=1.5) || //tiles are not neighouring
						(h->movement < cost) || //lack of movement points
						(t.tertype == rock) || //rock
						(!h->canWalkOnSea() && t.tertype == water) ||
						(t.blocked && !t.visitable) ) //tile is blocked andnot visitable
						goto fail;

					
					//check if there is blocking visitable object
					blockvis = false;
					tmh.movePoints = h->movement = (h->movement-cost); //take move points
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
						sendAndApply(&tmh); //failed to move to that tile but we visit object
						BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
						{
							if (obj->blockVisit)
							{
								//if(gs->checkFunc(obj->ID,"heroVisit")) //script function
								//	gs->objscr[obj->ID]["heroVisit"]->onHeroVisit(obj,h->subID);
								if(obj->state) //hard-coded function
									obj->state->onHeroVisit(obj->id,h->id);
							}
						}
						break;
					}
					else //normal move
					{
						tmh.result = 1;

						BOOST_FOREACH(CGObjectInstance *obj, gs->map->terrain[start.x-1][start.y][start.z].visitableObjects)
						{
							//TODO: allow to handle this in script-languages
							if(obj->state) //hard-coded function
								obj->state->onHeroLeave(obj->id,h->id);
						}

						//reveal fog of war
						int heroSight = h->getSightDistance();
						int xbeg = start.x - heroSight - 2;
						if(xbeg < 0)
							xbeg = 0;
						int xend = start.x + heroSight + 2;
						if(xend >= gs->map->width)
							xend = gs->map->width;
						int ybeg = start.y - heroSight - 2;
						if(ybeg < 0)
							ybeg = 0;
						int yend = start.y + heroSight + 2;
						if(yend >= gs->map->height)
							yend = gs->map->height;
						for(int xd=xbeg; xd<xend; ++xd) //revealing part of map around heroes
						{
							for(int yd=ybeg; yd<yend; ++yd)
							{
								int deltaX = (hmpos.x-xd)*(hmpos.x-xd);
								int deltaY = (hmpos.y-yd)*(hmpos.y-yd);
								if(deltaX+deltaY<h->getSightDistance()*h->getSightDistance())
								{
									if(gs->players[h->getOwner()].fogOfWarMap[xd][yd][hmpos.z] == 0)
									{
										tmh.fowRevealed.insert(int3(xd,yd,hmpos.z));
									}
								}
							}
						}

						sendAndApply(&tmh);

						BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)//call objects if they are visited
						{
							//if(gs->checkFunc(obj->ID,"heroVisit")) //script function
							//	gs->objscr[obj->ID]["heroVisit"]->onHeroVisit(obj,h->subID);
							if(obj->state) //hard-coded function
								obj->state->onHeroVisit(obj->id,h->id);
						}
					}
					break;
				fail:
					sendAndApply(&tmh);
					break;
				}
			case 502: //swap creatures in garrison
				{
					ui8 what, p1, p2; si32 id1, id2;
					c >> what >> id1 >> p1 >> id2 >> p2;
					CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id1]),
						*s2 = static_cast<CArmedInstance*>(gs->map->objects[id2]);
					CCreatureSet *S1 = &s1->army, *S2 = &s2->army;
					
					if(what==1) //swap
					{
						int pom = S2->slots[p2].first;
						S2->slots[p2].first = S1->slots[p1].first;
						S1->slots[p1].first = pom;
						int pom2 = S2->slots[p2].second;
						S2->slots[p2].second = S1->slots[p1].second;
						S1->slots[p1].second = pom2;

						if(!S1->slots[p1].second)
							S1->slots.erase(p1);
						if(!S2->slots[p2].second)
							S2->slots.erase(p2);
					}
					else if(what==2)//merge
					{
						if(S1->slots[p1].first != S2->slots[p2].first) break; //not same creature
						S2->slots[p2].second += S1->slots[p1].second;
						S1->slots[p1].first = NULL;
						S1->slots[p1].second = 0;
						S1->slots.erase(p1);
					}
					else if(what==3) //split
					{
						si32 val;
						c >> val;
						if(S2->slots.find(p2) != S2->slots.end()) break; //slot not free
						S2->slots[p2].first = S1->slots[p1].first;
						S2->slots[p2].second = val;
						S1->slots[p1].second -= val;
						if(!S1->slots[p1].second) //if we've moved all creatures
							S1->slots.erase(p1); 
					}
					if((s1->ID==34 && !S1->slots.size()) //it's not allowed to take last stack from hero army!
						|| (s2->ID==34 && !S2->slots.size()))
					{
						break;
					}
					SetGarrisons sg;
					sg.garrs[id1] = *S1;
					if(s1 != s2)
						sg.garrs[id2] = *S2;
					sendAndApply(&sg);
					break;
				}
			case 503:
				{
					si32 id;
					ui8 pos;
					c >> id >> pos;
					CArmedInstance *s1 = static_cast<CArmedInstance*>(gs->map->objects[id]);
					s1->army.slots.erase(pos);
					SetGarrisons sg;
					sg.garrs[id] = s1->army;
					sendAndApply(&sg);
					break;
				}
			case 504:
				{
					si32 tid, bid;
					c >> tid >> bid;
					CGTownInstance * t = static_cast<CGTownInstance*>(gs->map->objects[tid]);
					CBuilding * b = VLC->buildh->buildings[t->subID][bid];
					for(int i=0;i<RESOURCE_QUANTITY;i++)
						if(b->resources[i] > gs->players[t->tempOwner].resources[i])
							break; //no res
					//TODO: check requirements
					//TODO: check if building isn't forbidden

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
					sr.res = gs->players[t->tempOwner].resources;
					for(int i=0;i<7;i++)
						sr.res[i]-=b->resources[i];
					sendAndApply(&sr);

					break;
				}
			case 506: //recruit creature
				{
					si32 objid, ser=-1; //ser - used dwelling level
					ui32 crid, cram; //recruited creature id and amount
					c >> objid >> crid >> cram;

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
						cram > VLC->creh->creatures[crid].maxAmount(gs->players[t->tempOwner].resources) ||  //lack of resources
						cram<=0	||
						slot<0	) 
						break;

					//recruit
					SetResources sr;
					sr.player = t->tempOwner;
					for(int i=0;i<RESOURCE_QUANTITY;i++)
						sr.res[i]  =  gs->players[t->tempOwner].resources[i] - (VLC->creh->creatures[crid].cost[i] * cram);

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
					break;
				}
			case 507://upgrade creature
				{
					ui32 objid, upgID;
					ui8 pos;
					c >> objid >> pos >> upgID;
					CArmedInstance *obj = static_cast<CArmedInstance*>(gs->map->objects[objid]);
					UpgradeInfo ui = gs->getUpgradeInfo(obj,pos);
					int player = obj->tempOwner;
					int crQuantity = obj->army.slots[pos].second;

					//check if upgrade is possible
					if(ui.oldID<0 || !vstd::contains(ui.newID,upgID)) 
						break;

					//check if player has enough resources
					for(int i=0;i<ui.cost.size();i++)
					{
						for (std::set<std::pair<int,int> >::iterator j=ui.cost[i].begin(); j!=ui.cost[i].end(); j++)
						{
							if(gs->players[player].resources[j->first] < j->second*crQuantity)
								goto upgend;
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
							sr.val = gs->players[player].resources[j->first] - j->second*crQuantity;
							sendAndApply(&sr);
						}
					}

					{
						//upgrade creature
						SetGarrisons sg;
						sg.garrs[objid] = obj->army;
						sg.garrs[objid].slots[pos].first = upgID;
						sendAndApply(&sg);	
					}		
upgend:
					break;
				}
			case 508: //garrison swap
				{
					si32 tid;
					c >> tid;
					CGTownInstance *town = gs->getTown(tid);
					if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies
					{
						CCreatureSet csn = town->visitingHero->army, cso = town->army;
						while(!cso.slots.empty())//while there are unmoved creatures
						{
							int pos = csn.getSlotFor(cso.slots.begin()->second.first);
							if(pos<0)
								goto handleConEnd;
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
						//sg.garrs[town->visitingHero->id] = town->visitingHero->army;

						SetHeroesInTown intown;
						intown.tid = tid;
						intown.garrison = town->visitingHero->id;
						intown.visiting =  town->garrisonHero->id;
						sendAndApply(&intown);
						sendAndApply(&sg);
					}
					else
					{
						std::cout << "Warning, wrong garrison swap command for " << tid << std::endl;
					}
					break;
				}
			case 509: //swap artifacts
				{
					si32 hid1, hid2;
					ui16 slot1, slot2;
					c >> hid1 >> slot1 >> hid2 >> slot2;
					CGHeroInstance *h1 = gs->getHero(hid1), *h2 = gs->getHero(hid2);
					if((distance(h1->pos,h2->pos) > 1.0)   ||   (h1->tempOwner != h2->tempOwner))
						break;
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
					break;
				}
			case 510: //buy artifact
				{
					ui32 hid;
					si32 aid, bid;
					c >> hid >> aid;
					CGHeroInstance *hero = gs->getHero(hid);
					CGTownInstance *town = hero->visitedTown;
					if(aid==0)
					{
						if(!vstd::contains(town->builtBuildings,si32(0)))
							break;
						SetResource sr;
						sr.player = hero->tempOwner;
						sr.resid = 6;
						sr.val = gs->players[hero->tempOwner].resources[6] - 500;
						sendAndApply(&sr);

						SetHeroArtifacts sha;
						sha.hid = hid;
						sha.artifacts = hero->artifacts;
						sha.artifWorn = hero->artifWorn;
						sha.artifWorn[17] = 0;
						sendAndApply(&sha);
					}
					else if(aid < 7  &&  aid > 3) //war machine
					{
						int price = VLC->arth->artifacts[aid].price;
						if(vstd::contains(hero->artifWorn,ui16(9+aid))  //hero already has this machine
							|| !vstd::contains(town->builtBuildings,si32(16)) //no blackismith
							|| gs->players[hero->tempOwner].resources[6] < price //no gold
							|| town->town->warMachine!= aid ) //this machine is not available here (//TODO: support ballista yard in stronghold)
						{
							break;
						}
						SetResource sr;
						sr.player = hero->tempOwner;
						sr.resid = 6;
						sr.val = gs->players[hero->tempOwner].resources[6] - price;
						sendAndApply(&sr);

						SetHeroArtifacts sha;
						sha.hid = hid;
						sha.artifacts = hero->artifacts;
						sha.artifWorn = hero->artifWorn;
						sha.artifWorn[9+aid] = aid;
						sendAndApply(&sha);
					}
					break;
				}
			case 511: //trade at marketplace
				{
					ui8 player;
					ui32 mode, id1, id2, val;
					c >> player >> mode >> id1 >> id2 >> val;
					val = std::min(si32(val),gs->players[player].resources[id1]);
					double uzysk = (double)gs->resVals[id1] * val * gs->getMarketEfficiency(player);
					uzysk /= gs->resVals[id2];
					SetResource sr;
					sr.player = player;
					sr.resid = id1;
					sr.val = gs->players[player].resources[id1] - val;
					sendAndApply(&sr);

					sr.resid = id2;
					sr.val = gs->players[player].resources[id2] + (int)uzysk;
					sendAndApply(&sr);

					break;
				}
			case 2001:
				{
					ui32 qid, answer;
					c >> qid >> answer;
					gsm.lock();
					CFunctionList<void(ui32)> callb = callbacks[qid];
					callbacks.erase(qid);
					gsm.unlock();
					callb(answer);
					break;
				}
			case 3002:
				{			
					BattleAction ba;
					c >> ba;
					switch(ba.actionType)
					{
					case 2: //walk
						{
							moveStack(ba.stackNumber,ba.destinationTile);
							break;
						}
					case 3: //defend
						{
							break;
						}
					case 4: //retreat/flee
						{
							//TODO: check if fleeing is possible (e.g. enemy may have Shackles of War)
							//TODO: calculate casualties
							//TODO: remove retreating hero from map and place it in recrutation list
							BattleResult *br = new BattleResult;
							br->result = 1;
							br->winner = !ba.side; //fleeing side loses
							br->s1 = gs->curB->cas[0]; //setting casualities
							br->s2 = gs->curB->cas[1]; //as above - second side ;]
							battleResult.set(br);
							break;
						}
					case 6: //walk or attack
						{
							moveStack(ba.stackNumber,ba.destinationTile);
							CStack *curStack = gs->curB->getStack(ba.stackNumber),
								*stackAtEnd = gs->curB->getStackT(ba.additionalInfo);

							if((curStack->position != ba.destinationTile) || //we wasn't able to reach destination tile
								(BattleInfo::mutualPosition(ba.destinationTile,ba.additionalInfo)<0) ) //destination tile is not neighbouring with enemy stack
								return;

							BattleAttack bat;
							prepareAttack(bat,curStack,stackAtEnd);
							sendAndApply(&bat);
							break;
						}
					case 7: //shoot
						{
							CStack *curStack = gs->curB->getStack(ba.stackNumber),
								*destStack= gs->curB->getStackT(ba.destinationTile);

							BattleAttack bat;
							prepareAttack(bat,curStack,destStack);
							bat.flags |= 1;

							sendAndApply(&bat);
							break;
						}
					}
					battleMadeAction.setn(true);
					//sprawdzic czy po tej akcji ktoras strona nie wygrala bitwy
					break;
				}
			default:
#ifndef __GNUC__
				throw std::exception("Not supported client message!");
#else
				throw std::exception();
#endif
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		end2 = true;
	}
	catch (const std::exception * e)
	{
		std::cerr << e->what()<< std::endl;	
		end2 = true;
		delete e;
	}
	catch(...)
	{
		end2 = true;
	}
handleConEnd:
	;
}
void CGameHandler::moveStack(int stack, int dest)
{							
	CStack *curStack = gs->curB->getStack(stack),
		*stackAtEnd = gs->curB->getStackT(dest);

	//initing necessary tables
	bool accessibility[187];
	if(curStack->creature->isDoubleWide())
		gs->curB->getAccessibilityMapForTwoHex(accessibility,curStack->attackerOwned,curStack->ID);
	else 
		gs->curB->getAccessibilityMap(accessibility,curStack->ID);

	if((stackAtEnd && stackAtEnd!=curStack && stackAtEnd->alive) || !accessibility[dest])
		return;

	//if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
	//	return false;

	std::vector<int> path = gs->curB->getPath(curStack->position,dest,accessibility);
	int tilesToMove = std::max((int)path.size()-curStack->creature->speed, 0);
	for(int v=path.size()-1; v>=tilesToMove; --v)
	{
		//inform clients about move
		BattleStackMoved sm;
		sm.stack = curStack->ID;
		sm.tile = path[v];
		if(v==path.size()-1) //move start - set flag
			sm.flags |= 1;
		if(v==0) //move end - set flag
			sm.flags |= 2;
		sendAndApply(&sm);
	}
}
CGameHandler::CGameHandler(void)
{
	gs = NULL;
}

CGameHandler::~CGameHandler(void)
{
	delete gs;
}
void CGameHandler::init(StartInfo *si, int Seed)
{
	Mapa *map = new Mapa(si->mapname);
	std::cout << "Map loaded!" << std::endl;
	gs = new CGameState();
	std::cout << "Gamestate created!" << std::endl;
	gs->init(si,map,Seed);	
	std::cout << "Gamestate initialized!" << std::endl;
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
}
int lowestSpeed(CGHeroInstance * chi)
{
	std::map<si32,std::pair<ui32,si32> >::iterator i = chi->army.slots.begin();
	int ret = VLC->creh->creatures[(*i++).second.first].speed;
	for (;i!=chi->army.slots.end();i++)
	{
		ret = std::min(ret,VLC->creh->creatures[(*i).second.first].speed);
	}
	return ret;
}
int valMovePoints(CGHeroInstance * chi)
{
	int ret = 1270+70*lowestSpeed(chi);
	if (ret>2000) 
		ret=2000;
	
	//TODO: additional bonuses (but they aren't currently stored in chi)

	return ret;
}
void CGameHandler::newTurn()
{
	NewTurn n;
	n.day = gs->day + 1;
	n.resetBuilded = true;

	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->first>=PLAYER_LIMIT) continue;
		SetResources r;
		r.player = i->first;
		for(int j=0;j<RESOURCE_QUANTITY;j++)
			r.res[j] = i->second.resources[j];
		
		for (unsigned j=0;j<(*i).second.heroes.size();j++) //handle heroes
		{
			NewTurn::Hero h;
			h.id = (*i).second.heroes[j]->id;
			h.move = valMovePoints((*i).second.heroes[j]);
			h.mana = (*i).second.heroes[j]->mana;
			n.heroes.insert(h);
		}
		for(std::vector<CGTownInstance *>::iterator j=i->second.towns.begin();j!=i->second.towns.end();j++)//handle towns
		{
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
	for (std::set<CCPPObjectScript *>::iterator i=cppscripts.begin();i!=cppscripts.end();i++)
	{
		(*i)->newTurn();
	}
}
void CGameHandler::run()
{	
	BOOST_FOREACH(CConnection *cc, conns)
	{//init conn.
		ui8 quantity, pom;
		//ui32 seed;
		(*cc) << gs->scenarioOps->mapname << gs->map->checksum << gs->seed;
		(*cc) >> quantity;
		for(int i=0;i<quantity;i++)
		{
			(*cc) >> pom;
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
	std::map<int,CCPPObjectScript*> scripts;
	CScriptCallback * csc = new CScriptCallback();
	csc->gh = this;
	handleCPPObjS(&scripts,new CVisitableOPH(csc));
	handleCPPObjS(&scripts,new CVisitableOPW(csc));
	handleCPPObjS(&scripts,new CPickable(csc));
	handleCPPObjS(&scripts,new CMines(csc));
	handleCPPObjS(&scripts,new CTownScript(csc));
	handleCPPObjS(&scripts,new CHeroScript(csc));
	handleCPPObjS(&scripts,new CMonsterS(csc));
	handleCPPObjS(&scripts,new CCreatureGen(csc));

	/****************************INITIALIZING OBJECT SCRIPTS************************************************/
	//std::string temps("newObject");
	for (unsigned i=0; i<gs->map->objects.size(); i++)
	{
		//c++ scripts
		if (scripts.find(gs->map->objects[i]->ID) != scripts.end())
		{
			gs->map->objects[i]->state = scripts[gs->map->objects[i]->ID];
			gs->map->objects[i]->state->newObject(gs->map->objects[i]->id);
		}
		else 
		{
			gs->map->objects[i]->state = NULL;
		}

		//// lua scripts
		//if(checkFunc(map->objects[i]->ID,temps))
		//	(*skrypty)[map->objects[i]->ID][temps]->newObject(map->objects[i]);
	}
	for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		states.addPlayer(i->first);

	while (!end2)
	{
		newTurn();
		for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		{
			if((i->second.towns.size()==0 && i->second.heroes.size()==0)  || i->second.color<0 || i->first>=PLAYER_LIMIT  ) continue; //players has not towns/castle - loser
			states.setFlag(i->first,&PlayerStatus::makingTurn,true);
			gs->currentPlayer = i->first;
			*connections[i->first] << ui16(100) << i->first;    

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
		stacks.push_back(new CStack(&VLC->creh->creatures[i->second.first],i->second.second,hero1->tempOwner, stacks.size(), true));
		stacks[stacks.size()-1]->ID = stacks.size()-1;
	}
	//initialization of positions
	switch(army1.slots.size()) //for attacker
	{
	case 0:
		break;
	case 1:
		stacks[0]->position = 86; //6
		break;
	case 2:
		stacks[0]->position = 35; //3
		stacks[1]->position = 137; //9
		break;
	case 3:
		stacks[0]->position = 35; //3
		stacks[1]->position = 86; //6
		stacks[2]->position = 137; //9
		break;
	case 4:
		stacks[0]->position = 1; //1
		stacks[1]->position = 69; //5
		stacks[2]->position = 103; //7
		stacks[3]->position = 171; //11
		break;
	case 5:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 86; //6
		stacks[3]->position = 137; //9
		stacks[4]->position = 171; //11
		break;
	case 6:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 103; //7
		stacks[4]->position = 137; //9
		stacks[5]->position = 171; //11
		break;
	case 7:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 86; //6
		stacks[4]->position = 103; //7
		stacks[5]->position = 137; //9
		stacks[6]->position = 171; //11
		break;
	default: //fault
		break;
	}
	for(std::map<si32,std::pair<ui32,si32> >::iterator i = army2.slots.begin(); i!=army2.slots.end(); i++)
		stacks.push_back(new CStack(&VLC->creh->creatures[i->second.first],i->second.second,hero2 ? hero2->tempOwner : 255, stacks.size(), false));
	switch(army2.slots.size()) //for defender
	{
	case 0:
		break;
	case 1:
		stacks[0+army1.slots.size()]->position = 100; //6
		break;
	case 2:
		stacks[0+army1.slots.size()]->position = 49; //3
		stacks[1+army1.slots.size()]->position = 151; //9
		break;
	case 3:
		stacks[0+army1.slots.size()]->position = 49; //3
		stacks[1+army1.slots.size()]->position = 100; //6
		stacks[2+army1.slots.size()]->position = 151; //9
		break;
	case 4:
		stacks[0+army1.slots.size()]->position = 15; //1
		stacks[1+army1.slots.size()]->position = 83; //5
		stacks[2+army1.slots.size()]->position = 117; //7
		stacks[3+army1.slots.size()]->position = 185; //11
		break;
	case 5:
		stacks[0+army1.slots.size()]->position = 15; //1
		stacks[1+army1.slots.size()]->position = 49; //3
		stacks[2+army1.slots.size()]->position = 100; //6
		stacks[3+army1.slots.size()]->position = 151; //9
		stacks[4+army1.slots.size()]->position = 185; //11
		break;
	case 6:
		stacks[0+army1.slots.size()]->position = 15; //1
		stacks[1+army1.slots.size()]->position = 49; //3
		stacks[2+army1.slots.size()]->position = 83; //5
		stacks[3+army1.slots.size()]->position = 117; //7
		stacks[4+army1.slots.size()]->position = 151; //9
		stacks[5+army1.slots.size()]->position = 185; //11
		break;
	case 7:
		stacks[0+army1.slots.size()]->position = 15; //1
		stacks[1+army1.slots.size()]->position = 49; //3
		stacks[2+army1.slots.size()]->position = 83; //5
		stacks[3+army1.slots.size()]->position = 100; //6
		stacks[4+army1.slots.size()]->position = 117; //7
		stacks[5+army1.slots.size()]->position = 151; //9
		stacks[6+army1.slots.size()]->position = 185; //11
		break;
	default: //fault
		break;
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
	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

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
