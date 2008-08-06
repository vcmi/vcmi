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
#include "../CLua.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CHeroHandler.h"
#include "boost/date_time/posix_time/posix_time_types.hpp" //no i/o just types
#include "../lib/VCMI_Lib.h"
#include "../lib/CondSh.h"
#include <boost/thread/xtime.hpp>
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
boost::condition_variable cTurn;
boost::mutex mTurn;
boost::shared_mutex gsm;


CondSh<bool> battleMadeAction;
CondSh<BattleResult *> battleResult(NULL);

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

void CGameHandler::handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script)
{
	std::vector<int> tempv = script->yourObjects();
	for (unsigned i=0;i<tempv.size();i++)
	{
		(*mapa)[tempv[i]]=script;
	}
	cppscripts.insert(script);
}

void CGameHandler::changePrimSkill(int ID, int which, int val, bool abs)
{
	SetPrimSkill sps;
	sps.id = ID;
	sps.which = which;
	sps.abs = abs;
	sps.val = val;
	sendAndApply(&sps);
	if(which==4)
	{
		CGHeroInstance *hero = static_cast<CGHeroInstance *>(gs->map->objects[ID]);
		if(hero->exp >= VLC->heroh->reqExp(hero->level+1)) //new level
		{
			//hero->level++;

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
			hero->primSkills[x]++;

			std::set<ui16> choice;

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
				choice.insert(hero->type->heroClass->chooseSecSkill(none)); //new skill
			}
			else
			{
				int s = hero->type->heroClass->chooseSecSkill(basicAndAdv);
				choice.insert(s);
				basicAndAdv.erase(s);
			}
			if(basicAndAdv.size())
			{
				choice.insert(hero->type->heroClass->chooseSecSkill(basicAndAdv)); //new skill
			}
			else if(hero->secSkills.size() < hero->type->heroClass->skillLimit)
			{
				choice.insert(hero->type->heroClass->chooseSecSkill(none)); //new skill
			}

		}
		//TODO - powiadomic interfejsy, sprawdzic czy nie ma awansu itp
	}
}

void CGameHandler::startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2)
{

	BattleInfo *curB = new BattleInfo;
	//battle start
	{
		battleResult.set(NULL);
		std::vector<CStack*> & stacks = (curB->stacks);

		curB->army1=army1;
		curB->army2=army2;
		curB->hero1=(hero1)?(hero1->id):(-1);
		curB->hero2=(hero1)?(hero1->id):(-1);
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
			states[hero1->tempOwner] += 10;
		if(hero2 && hero2->tempOwner<PLAYER_LIMIT)
			states[hero2->tempOwner] += 10;

		//send info about battles
		BattleStart bs;
		bs.info = curB;
		sendAndApply(&bs);

		NEW_ROUND;
	}

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

	//end battle, remove all info, free memory
	sendAndApply(battleResult.data);
	delete battleResult.data;
	//for(int i=0;i<stacks.size();i++)
	//	delete stacks[i];
	//delete curB;
	//curB = NULL;
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
					mTurn.lock();
					states[gs->currentPlayer] = 0;
					mTurn.unlock();
					cTurn.notify_all();
					break;
				}
			case 500:
				{
					si32 id;
					c >> id;
					RemoveHero rh(id);
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
									obj->state->onHeroVisit(obj->id,h->subID);
							}
						}
						break;
					}
					else //normal move
					{
						tmh.result = 1;

						BOOST_FOREACH(CGObjectInstance *obj, gs->map->terrain[start.x][start.y][start.z].visitableObjects)
						{
							//TODO: allow to handle this in script-languages
							if(obj->state) //hard-coded function
								obj->state->onHeroLeave(obj->id,h->subID);
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

						if(!S1->slots[p1].first)
							S1->slots.erase(p1);
						if(!S2->slots[p2].first)
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
						SetStrInfo ssi;
						ssi.tid = tid;
						ssi.cres = t->strInfo.creatures;
						ssi.cres[bid-30] = VLC->creh->creatures[t->town->basicCreatures[bid-30]].growth;
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
			case 3002:
				{			
					BattleAction ba;
					c >> ba;
					switch(ba.actionType)
					{
					case 2: //walk
						{
							CStack *curStack = gs->curB->getStack(ba.stackNumber),
								*stackAtEnd = gs->curB->getStackT(ba.destinationTile);

							//initing necessary tables
							bool accessibility[187];
							if(curStack->creature->isDoubleWide())
							{
								gs->curB->getAccessibilityMapForTwoHex(accessibility,curStack->attackerOwned,curStack->ID);
								//accessibility[curStack->attackerOwned ? curStack->position+1 : curStack->position-1]=true;//OUR second tile is for US accessible
							}
							else 
								gs->curB->getAccessibilityMap(accessibility,curStack->ID);
							//accessibility[curStack->position] = true; //OUR tile is for US accessible

							//if(!stackAtEnd && !accessibility[dest])
							//	return false;

							//if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
							//	return false;

							std::vector<int> path = gs->curB->getPath(curStack->position,ba.destinationTile,accessibility);
							int tilesToMove = std::max((int)path.size()-curStack->creature->speed, 0);
							for(int v=path.size()-1; v>=tilesToMove; --v)
							{
								if(v!=0 || !stackAtEnd) //it's not the last step or the last tile is free
								{
									//inform clients about move
									BattleStackMoved sm;
									sm.stack = curStack->ID;
									sm.tile = path[v];
									if(v==path.size()-1)//move start - set flag
										sm.flags |= 1;
									if(v==0 || (stackAtEnd && v==1)) //move end - set flag
										sm.flags |= 2;
									sendAndApply(&sm);
								}
								else //if it's last step and we should attack unit at the end
								{
									//LOCPLINT->battleStackAttacking(ID, path[v]);
									////counting dealt damage
									//int finalDmg = calculateDmg(curStack, curB->stacks[numberOfStackAtEnd]);

									////applying damages
									//int cresKilled = finalDmg / curB->stacks[numberOfStackAtEnd]->creature->hitPoints;
									//int damageFirst = finalDmg % curB->stacks[numberOfStackAtEnd]->creature->hitPoints;

									//if( curB->stacks[numberOfStackAtEnd]->firstHPleft <= damageFirst )
									//{
									//	curB->stacks[numberOfStackAtEnd]->amount -= 1;
									//	curB->stacks[numberOfStackAtEnd]->firstHPleft += curB->stacks[numberOfStackAtEnd]->creature->hitPoints - damageFirst;
									//}
									//else
									//{
									//	curB->stacks[numberOfStackAtEnd]->firstHPleft -= damageFirst;
									//}

									//int cresInstackBefore = curB->stacks[numberOfStackAtEnd]->amount; 
									//curB->stacks[numberOfStackAtEnd]->amount -= cresKilled;
									//if(curB->stacks[numberOfStackAtEnd]->amount<=0) //stack killed
									//{
									//	curB->stacks[numberOfStackAtEnd]->amount = 0;
									//	LOCPLINT->battleStackKilled(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore) , ID, false);
									//	curB->stacks[numberOfStackAtEnd]->alive = false;
									//}
									//else
									//{
									//	LOCPLINT->battleStackIsAttacked(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore), ID, false);
									//}

									//damage applied
								}
							}
							//curB->stackActionPerformed = true;
							//LOCPLINT->actionFinished(BattleAction());
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
							battleResult.set(br);
							break;
						}
					case 6: //walk or attack
						{
							//battleMoveCreatureStack(ba.stackNumber, ba.destinationTile);
							//battleAttackCreatureStack(ba.stackNumber, ba.destinationTile);
							break;
						}
					case 7: //shoot
						{
							//battleShootCreatureStack(ba.stackNumber, ba.destinationTile);
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
	gs = new CGameState();
	gs->init(si,map,Seed);	

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

	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->first>=PLAYER_LIMIT) continue;
		NewTurn::Resources r;
		r.player = i->first;
		for(int j=0;j<RESOURCE_QUANTITY;j++)
			r.resources[j] = i->second.resources[j];
		
		for (unsigned j=0;j<(*i).second.heroes.size();j++) //handle heroes
		{
			NewTurn::Hero h;
			h.id = (*i).second.heroes[j]->id;
			h.move = valMovePoints((*i).second.heroes[j]);
			h.mana = (*i).second.heroes[j]->mana;
			n.heroes.insert(h);
		}
		for(unsigned j=0;j<i->second.towns.size();j++)//handle towns
		{
			i->second.towns[j]->builded=0;
			//if(gs->getDate(1)==1) //first day of week
			//{
			//	for(int k=0;k<CREATURES_PER_TOWN;k++) //creature growths
			//	{
			//		if(i->second.towns[j]->creatureDwelling(k))//there is dwelling (k-level)
			//			i->second.towns[j]->strInfo.creatures[k]+=i->second.towns[j]->creatureGrowth(k);
			//	}
			//}
			if((gs->day) && i->first<PLAYER_LIMIT)//not the first day and town not neutral
				r.resources[6] += i->second.towns[j]->dailyIncome();
		}
		n.res.insert(r);
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

	while (!end2)
	{
		newTurn();
		for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		{
			if((i->second.towns.size()==0 && i->second.heroes.size()==0)  || i->second.color<0 || i->first>=PLAYER_LIMIT  ) continue; //players has not towns/castle - loser
			states[i->first] = 1;
			gs->currentPlayer = i->first;
			*connections[i->first] << ui16(100) << i->first;    
			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(mTurn);
			while(states[i->first] && !end2)
			{
				boost::posix_time::time_duration p;
				p= boost::posix_time::seconds(1);
				boost::xtime time={0,0};
				time.sec = static_cast<boost::xtime::xtime_sec_t>(p.total_seconds());
				cTurn.wait(lock);
				//cTurn.timed_wait(lock,time);
			}
		}
	}
}
