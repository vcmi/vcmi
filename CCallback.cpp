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
#include "CLua.h"
int CCallback::lowestSpeed(CGHeroInstance * chi)
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
int CCallback::valMovePoints(CGHeroInstance * chi)
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
	gs->day++;
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (int j=0;j<(*i).second.heroes.size();j++)
		{
			(*i).second.heroes[j]->movement = valMovePoints((*i).second.heroes[j]);
		}
	}
}
bool CCallback::moveHero(int ID, CPath * path, int idtype, int pathType)
{
	CGHeroInstance * hero = NULL;

	if (idtype==0)
	{
		if (player==-1)
			hero=gs->players[player+1].heroes[ID];
		else
			hero=gs->players[player].heroes[ID];
	}
	else if (idtype==1 && player>=0) //looking for it in local area
	{
		for (int i=0; i<gs->players[player].heroes.size();i++)
		{
			if (gs->players[player].heroes[i]->type->ID == ID)
				hero = gs->players[player].heroes[i];
		}
	}
	else //idtype==1; player<0
	{

		for(std::map<int, PlayerState>::iterator j=CGI->state->players.begin(); j!=CGI->state->players.end(); ++j)
		{
			for (int i=0; i<(*j).second.heroes.size();i++)
			{
				if ((*j).second.heroes[i]->type->ID == ID)
				{
					hero = (*j).second.heroes[i];
				}
			}
		}
	}

	if (!hero)
		return false; //can't find hero
	if(!verifyPath(path,!hero->canWalkOnSea()))//TODO: not check sea, if hero has flying or walking on water
		return false; //invalid path

	//check path format
	if (pathType==0)
		CPathfinder::convertPath(path,pathType);
	if (pathType>1)
		throw std::exception("Unknown path format");

	CPath * ourPath = path; 
	if(!ourPath)
		return false;
	for(int i=ourPath->nodes.size()-1; i>0; i--)
	{
		int3 stpos, endpos;
		stpos = int3(ourPath->nodes[i].coord.x, ourPath->nodes[i].coord.y, hero->pos.z);
		endpos = int3(ourPath->nodes[i-1].coord.x, ourPath->nodes[i-1].coord.y, hero->pos.z);
		HeroMoveDetails curd;
		curd.src = stpos;
		curd.dst = endpos;
		curd.ho = hero;
		curd.owner = hero->state->owner;
		/*if(player!=-1)
		{
			hero->pos = endpos;
		}*/
		if((hero->movement>=CGI->mh->getCost(stpos, endpos, hero))  || player==-1)
		{ //performing move
			hero->movement-=CGI->mh->getCost(stpos, endpos, hero);
			
			int heroSight = hero->getSightDistance();

			int xbeg = stpos.x - heroSight - 2;
			if(xbeg < 0)
				xbeg = 0;

			int xend = stpos.x + heroSight + 2;
			if(xend >= CGI->ac->map.width)
				xend = CGI->ac->map.width - 1;

			int ybeg = stpos.y - heroSight - 2;
			if(ybeg < 0)
				ybeg = 0;

			int yend = stpos.y + heroSight + 2;
			if(yend >= CGI->ac->map.height)
				yend = CGI->ac->map.height - 1;

			for(int xd=xbeg; xd<xend; ++xd) //revealing part of map around heroes
			{
				for(int yd=ybeg; yd<yend; ++yd)
				{
					int deltaX = (hero->getPosition(false).x-xd)*(hero->getPosition(false).x-xd);
					int deltaY = (hero->getPosition(false).y-yd)*(hero->getPosition(false).y-yd);
					if(deltaX+deltaY<hero->getSightDistance()*hero->getSightDistance())
						gs->players[player].fogOfWarMap[xd][yd][hero->getPosition(false).z] = 1;
				}
			}

			hero->pos = curd.dst;
			int nn=0; //number of interfece of currently browsed player
			for(std::map<int, PlayerState>::iterator j=CGI->state->players.begin(); j!=CGI->state->players.end(); ++j)//CGI->state->players.size(); ++j) //for testing
			{
				if (j->first > PLAYER_LIMIT)
					break;
				if(j->second.fogOfWarMap[stpos.x-1][stpos.y][stpos.z] || j->second.fogOfWarMap[endpos.x-1][endpos.y][endpos.z])
				{ //player should be notified
					CGI->playerint[j->first]->heroMoved(curd);
				}
				++nn;
			}

			std::vector< CGObjectInstance * > vis = CGI->mh->getVisitableObjs(hero->getPosition(false));
			for (int iii=0; iii<vis.size(); iii++)
				std::cout<< CGI->objh->objects[vis[iii]->ID].name<<std::endl;

		}
		else
			return true; //move ended - no more movement points
		//hero->pos = curd.dst;
	}
	return true;
}


int CCallback::howManyTowns()
{
	return gs->players[gs->currentPlayer].towns.size();
}
const CGTownInstance * CCallback::getTownInfo(int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
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
int CCallback::howManyHeroes()
{
	return gs->players[player].heroes.size();
}
const CGHeroInstance * CCallback::getHeroInfo(int player, int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
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
		break;
	case 1:
		temp = (gs->day)%7;
		if (temp)
			return temp;
		else return 7;
		break;
	case 2:
		temp = ((gs->day-1)/7)+1;
		if (!(temp%4))
			return 4;
		else 
			return (temp%4);
		break;
	case 3:
		return ((gs->day-1)/28)+1;
		break;
	}
	return 0;
}
bool CCallback::verifyPath(CPath * path, bool blockSea)
{
	for (int i=0;i<path->nodes.size();i++)
	{
		if ( CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].blocked 
			&& (! (CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].visitable)))
			return false; //path is wrong - one of the tiles is blocked

		if (blockSea)
		{
			if (i==0)
				continue;

			if (
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].terType==EterrainType::water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].terType!=EterrainType::water))
				  ||
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].terType!=EterrainType::water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].terType==EterrainType::water))
				  ||
				  (CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].terType==EterrainType::rock)
					
				)
				return false;
		}


	}
	return true;
}

std::vector < std::string > CCallback::getObjDescriptions(int3 pos)
{
	if(gs->players[player].fogOfWarMap[pos.x][pos.y][pos.z])
		return CGI->mh->getObjDescriptions(pos);
	else return std::vector< std::string > ();
}

PseudoV< PseudoV< PseudoV<unsigned char> > > & CCallback::getVisibilityMap()
{
	return gs->players[player].fogOfWarMap;
}


bool CCallback::isVisible(int3 pos, int Player)
{
	return gs->players[Player].fogOfWarMap[pos.x][pos.y][pos.z];
}

std::vector < const CGHeroInstance *> * CCallback::getHeroesInfo(bool onlyOur)
{
	std::vector < const CGHeroInstance *> * ret = new std::vector < const CGHeroInstance *>();
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (int j=0;j<(*i).second.heroes.size();j++)
		{
			if ( ( isVisible((*i).second.heroes[j]->getPosition(false),player) ) || (*i).first==player)
			{
				ret->push_back((*i).second.heroes[j]);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	return ret;
}

bool CCallback::isVisible(int3 pos)
{
	return isVisible(pos,player);
}
