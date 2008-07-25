#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/bind.hpp>
#include "CGameHandler.h"
#include "../CGameState.h"
#include "../StartInfo.h"
#include "../map.h"
#include "../lib/NetPacks.h"
#include "../lib/Connection.h"
#include "../CLua.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"

bool makingTurn;
boost::condition_variable cTurn;
boost::mutex mTurn;
boost::shared_mutex gsm;

void CGameHandler::handleConnection(std::set<int> players, CConnection &c)
{
	ui16 pom;
	while(1)
	{
		c >> pom;
		switch(pom)
		{
		case 100: //my interface end its turn
			mTurn.lock();
			makingTurn = false;
			mTurn.unlock();
			cTurn.notify_all();
			break;
		default:
			throw std::exception("Not supported client message!");
			break;
		}
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
}
int lowestSpeed(CGHeroInstance * chi)
{
	std::map<int,std::pair<CCreature*,int> >::iterator i = chi->army.slots.begin();
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
	//std::map<int, PlayerState>::iterator i = gs->players.begin() ;
	gs->day++;
	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		//handle heroes/////////////////////////////
		for (unsigned j=0;j<(*i).second.heroes.size();j++)
		{
			(*i).second.heroes[j]->movement = valMovePoints((*i).second.heroes[j]);
		}


		//handle towns/////////////////////////////
		for(unsigned j=0;j<i->second.towns.size();j++)
		{
			i->second.towns[j]->builded=0;
			if(gs->getDate(1)==1) //first day of week
			{
				for(int k=0;k<CREATURES_PER_TOWN;k++) //creature growths
				{
					if(i->second.towns[j]->creatureDwelling(k))//there is dwelling (k-level)
						i->second.towns[j]->strInfo.creatures[k]+=i->second.towns[j]->creatureGrowth(k);
				}
			}
			if((gs->day>1) && i->first<PLAYER_LIMIT)
				i->second.resources[6]+=i->second.towns[j]->dailyIncome();
		}
	}	
	for (std::set<CCPPObjectScript *>::iterator i=gs->cppscripts.begin();i!=gs->cppscripts.end();i++)
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
		(*cc) << gs->scenarioOps->mapname;// << gs->map->checksum << seed;
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
	while (1)
	{
		for(std::map<ui8,PlayerState>::iterator i = gs->players.begin(); i != gs->players.end(); i++)
		{
			if((i->second.towns.size()==0 && i->second.heroes.size()==0)  || i->second.color<0) continue; //players has not towns/castle - loser
			makingTurn = true;
			*connections[i->first] << ui16(100) << i->first;    
			//wait till turn is done
			boost::unique_lock<boost::mutex> lock(mTurn);
			while(makingTurn)
			{
				cTurn.wait(lock);
			}

		}
	}
}