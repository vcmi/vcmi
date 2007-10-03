#include "stdafx.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "hch\CHeroHandler.h"
#include "hch\CTownHandler.h"
#include "CGameInfo.h"
#include "hch\CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "CGameInterface.h"
int CCallback::lowestSpeed(CHeroInstance * chi)
{
	int min = 150;
	for (  std::map<int,std::pair<CCreature*,int> >::iterator i = chi->army.slots.begin(); 
		   i!=chi->army.slots.end();		 i++													)
	{
		if (min>(*i).second.first->speed)
			min = (*i).second.first->speed;
	}
	return min;
}
int CCallback::valMovePoints(CHeroInstance * chi)
{
	int ret = 1270+70*lowestSpeed(chi);
	if (ret>2000) 
		ret=2000;
	
	//TODO: additional bonuses (but they aren't currently stored in chi)

	return ret;
}
void CCallback::newTurn()
{
	//std::map<int, PlayerState>::iterator i = gs->players.begin() ;
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (int j=0;j<(*i).second.heroes.size();j++)
		{
			(*i).second.heroes[j]->movement = valMovePoints((*i).second.heroes[j]);
		}
	}
}
bool CCallback::moveHero(int ID, int3 destPoint, int idtype)
{
	if(ID<0 || ID>CGI->heroh->heroInstances.size())
		return false;
	if(destPoint.x<0 || destPoint.x>CGI->ac->map.width)
		return false;
	if(destPoint.y<0 || destPoint.y>CGI->ac->map.height)
		return false;
	if(destPoint.z<0 || destPoint.z>CGI->mh->ttiles[0][0].size()-1)
		return false;
	CPath * ourPath = CGI->pathf->getPath(CGI->heroh->heroInstances[ID]->pos, destPoint, CGI->heroh->heroInstances[ID]);
	if(!ourPath)
		return false;
	for(int i=ourPath->nodes.size()-1; i>0; i--)
	{
		int3 stpos, endpos;
		stpos = int3(ourPath->nodes[i].coord.x, ourPath->nodes[i].coord.y, CGI->heroh->heroInstances[ID]->pos.z);
		endpos = int3(ourPath->nodes[i-1].coord.x, ourPath->nodes[i-1].coord.y, CGI->heroh->heroInstances[ID]->pos.z);
		HeroMoveDetails curd;
		curd.src = stpos;
		curd.dst = endpos;
		curd.heroID = ID;
		curd.owner = CGI->heroh->heroInstances[ID]->owner;
		//if(CGI->heroh->heroInstances[ID]->movement>=CGI->mh->getCost(stpos, endpos, CGI->heroh->heroInstances[ID]))
		{ //performing move
			int nn=0; //number of interfece of currently browsed player
			for(std::map<int, PlayerState>::iterator j=CGI->state->players.begin(); j!=CGI->state->players.end(); ++j)//CGI->state->players.size(); ++j) //for testing
			{
				if(j->second.fogOfWarMap[stpos.x][stpos.y][stpos.z] || j->second.fogOfWarMap[endpos.x][endpos.y][endpos.z])
				{ //player should be notified
					CGI->playerint[nn]->heroMoved(curd);
				}
				++nn;
				break; //for testing only
			}
		}
		//else
			//return true; //move ended - no more movement points
	}
	return true;
}


int CCallback::howManyTowns()
{
	return gs->players[gs->currentPlayer].towns.size();
}
const CTownInstance * CCallback::getTownInfo(int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	if (!mode)
		return gs->players[gs->currentPlayer].towns[val];
	else 
	{
		//TODO: add some smart ID to the CTownInstance


		//for (int i=0; i<gs->players[gs->currentPlayer].towns.size();i++)
		//{
		//	if (gs->players[gs->currentPlayer].towns[i]->someID==val)
		//		return gs->players[gs->currentPlayer].towns[i];
		//}
		return NULL;
	}
	return NULL;
}
int CCallback::howManyHeroes(int player)
{
	if (gs->currentPlayer!=player) //TODO: checking if we are allowed to give that info
		return -1;
	return gs->players[player].heroes.size();
}
const CHeroInstance * CCallback::getHeroInfo(int player, int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	if (gs->currentPlayer!=player) //TODO: checking if we are allowed to give that info
		return NULL;
	if (!mode)
		return gs->players[player].heroes[val];
	else 
	{
		for (int i=0; i<gs->players[player].heroes.size();i++)
		{
			if (gs->players[player].heroes[i]->type->ID==val)
				return gs->players[player].heroes[i];
		}
	}
	return NULL;
}

int CCallback::getResourceAmount(int type)
{
	return gs->players[gs->currentPlayer].resources[type];
}

int CCallback::getDate(int mode)
{
	int temp;
	switch (mode)
	{
	case 0:
		return gs->day;
	case 1:
		temp = (gs->day)%7;
		if (temp)
			return temp;
		else return 7;
	case 2:
		temp = ((gs->day-1)/7)+1;
		if (temp%4)
			return temp;
		else return 4;
	case 3:
		return ((gs->day-1)/28)+1;
	}
}