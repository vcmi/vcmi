#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/bind.hpp>
#include "CGameHandler.h"
#include "../CLua.h"
#include "../CGameState.h"
#include "../StartInfo.h"
#include "../map.h"
#include "../lib/NetPacks.h"
#include "../lib/Connection.h"
#include "../CLua.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"
#include "boost/date_time/posix_time/posix_time_types.hpp" //no i/o just types
extern bool end;
bool makingTurn;
boost::condition_variable cTurn;
boost::mutex mTurn;
boost::shared_mutex gsm;

double distance(int3 a, int3 b)
{
	return std::sqrt( (double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}

void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	try
	{
		ui16 pom;
		while(!end)
		{
			c >> pom;
			switch(pom)
			{
			case 100: //my interface ended its turn
				{
					mTurn.lock();
					makingTurn = false;
					mTurn.unlock();
					cTurn.notify_all();
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
					int cost = (double)h->getTileCost(t.tertype,t.malle,t.nuine) * distance(start,end);

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
					bool blockvis = false;
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
						gs->apply(&tmh);
						sendToAllClients(&tmh); //failed to move to that tile but we visit object
						BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
						{
							if (obj->blockVisit)
							{
								if(gs->checkFunc(obj->ID,"heroVisit")) //script function
									gs->objscr[obj->ID]["heroVisit"]->onHeroVisit(obj,h->subID);
								if(obj->state) //hard-coded function
									obj->state->onHeroVisit(obj,h->subID);
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
								obj->state->onHeroLeave(obj,h->subID);
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

						gs->apply(&tmh);
						sendToAllClients(&tmh);

						//call objects if they arevisited
						BOOST_FOREACH(CGObjectInstance *obj, t.visitableObjects)
						{
							if(gs->checkFunc(obj->ID,"heroVisit")) //script function
								gs->objscr[obj->ID]["heroVisit"]->onHeroVisit(obj,h->subID);
							if(obj->state) //hard-coded function
								obj->state->onHeroVisit(obj,h->subID);
						}
					}
					break;
				fail:
					gs->apply(&tmh);
					sendToAllClients(&tmh);
					break;
				}
			default:
				throw std::exception("Not supported client message!");
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		end = true;
	}
	catch (const std::exception * e)
	{
		std::cerr << e->what()<< std::endl;	
		end = true;
		delete e;
	}
	catch(...)
	{
		end = true;
	}
}
template <typename T>void CGameHandler::sendToAllClients(CPack<T> * info)
{
	BOOST_FOREACH(CConnection* c, conns)
		*c << info->getType() << *info->This();
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
}
int lowestSpeed(CGHeroInstance * chi)
{
	std::map<si32,std::pair<CCreature*,si32> >::iterator i = chi->army.slots.begin();
	int ret = (*i++).second.first->speed;
	for (;i!=chi->army.slots.end();i++)
	{
		ret = min(ret,(*i).second.first->speed);
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
	gs->apply(&n);
	sendToAllClients(&n);
	//for (std::set<CCPPObjectScript *>::iterator i=gs->cppscripts.begin();i!=gs->cppscripts.end();i++)
	//{
	//	(*i)->newTurn();
	//}
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
	while (!end)
	{
		newTurn();
		for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		{
			if((i->second.towns.size()==0 && i->second.heroes.size()==0)  || i->second.color<0) continue; //players has not towns/castle - loser
			makingTurn = true;
			gs->currentPlayer = i->first;
			*connections[i->first] << ui16(100) << i->first;    
			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(mTurn);
			while(makingTurn && !end)
			{
				boost::posix_time::time_duration p;
				p= boost::posix_time::seconds(1);
				cTurn.timed_wait(lock,p);
			}

		}
	}
}