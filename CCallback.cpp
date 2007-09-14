#include "stdafx.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "hch\CHeroHandler.h"
#include "CGameInfo.h"
#include "hch\CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "CGameInterface.h"

bool CCallback::moveHero(int ID, int3 destPoint)
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
		stpos = int3(ourPath->nodes[i].x, ourPath->nodes[i].y, CGI->heroh->heroInstances[ID]->pos.z);
		endpos = int3(ourPath->nodes[i-1].x, ourPath->nodes[i-1].y, CGI->heroh->heroInstances[ID]->pos.z);
		HeroMoveDetails curd;
		curd.src = stpos;
		curd.dst = endpos;
		curd.heroID = ID;
		curd.owner = CGI->heroh->heroInstances[ID]->owner;
		//if(CGI->heroh->heroInstances[ID]->movement>=CGI->mh->getCost(stpos, endpos, CGI->heroh->heroInstances[ID]))
		{ //performing move
			for(int j=0; j<CGI->state->players.size(); ++j)
			{
				if(CGI->state->players[j].fogOfWarMap[stpos.x][stpos.y][stpos.z] || CGI->state->players[j].fogOfWarMap[endpos.x][endpos.y][endpos.z])
				{ //player should be notified
					CGI->playerint[j]->heroMoved(curd);
				}
			}
		}
		//else
			return true; //move ended - no more movement points
	}
	return true;
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