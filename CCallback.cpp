#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "CCallback.h"
#include "CGameInfo.h"
#include "CGameState.h"
#include "CPathfinder.h"
#include "CPlayerInterface.h"
#include "CPlayerInterface.h"
#include "client/Client.h"
#include "hch/CAmbarCendamo.h"
#include "hch/CBuildingHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CTownHandler.h"
#include "lib/Connection.h"
#include "lib/NetPacks.h"
#include "mapHandler.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
extern CSharedCond<std::set<IPack*> > mess;

int gcd(int x, int y)
{
	int temp;
	if (y > x)
		swap(x,y);
	while (y != 0)
	{
		temp = y;
		y = x-y;
		x = temp;
		if (y > x)
			swap(x,y);
	}
	return x;
}
HeroMoveDetails::HeroMoveDetails(int3 Src, int3 Dst, CGHeroInstance*Ho)
	:src(Src),dst(Dst),ho(Ho)
{
	owner = ho->getOwner();
};
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

		for(std::map<ui8, PlayerState>::iterator j=gs->players.begin(); j!=gs->players.end(); ++j)
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
#ifndef __GNUC__
		throw std::exception("Unknown path format");
#else
		throw std::exception();
#endif

	CPath * ourPath = path; 
	if(!ourPath)
		return false;
	for(int i=ourPath->nodes.size()-1; i>0; i--)
	{
		int3 stpos(ourPath->nodes[i].coord.x, ourPath->nodes[i].coord.y, hero->pos.z), 
			endpos(ourPath->nodes[i-1].coord.x, ourPath->nodes[i-1].coord.y, hero->pos.z);
		HeroMoveDetails curd(stpos,endpos,hero);
		*cl->serv << ui16(501) << hero->id << stpos << endpos;
		{//wait till there is server answer
			boost::unique_lock<boost::mutex> lock(*mess.mx);
			while(std::find_if(mess.res->begin(),mess.res->end(),IPack::isType<501>) == mess.res->end())
				mess.cv->wait(lock);
			std::set<IPack*>::iterator itr = std::find_if(mess.res->begin(),mess.res->end(),IPack::isType<501>);
			TryMoveHero tmh = *static_cast<TryMoveHero*>(*itr);
			mess.res->erase(itr);
			if(!tmh.result)
				return false;
		}
	}
	return true;
}

void CCallback::selectionMade(int selection, int asker)
{
	*cl->serv << ui16(2001) << ui32(asker) << ui32(selection);
}
void CCallback::recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount)
{
	if(player!=obj->tempOwner) return;
	*cl->serv << ui16(506) << obj->id << ID << amount;
}


bool CCallback::dismissCreature(const CArmedInstance *obj, int stackPos)
{
	if(((player>=0)  &&  obj->tempOwner != player) || obj->army.slots.size()<2)
		return false;
	*cl->serv << ui16(503) << obj->id <<  ui8(stackPos);
	return true;
}
bool CCallback::upgradeCreature(const CArmedInstance *obj, int stackPos, int newID)
{
	*cl->serv << ui16(507) << obj->id <<  ui8(stackPos) << ui32(newID);
	return false;
}
void CCallback::endTurn()
{
	std::cout << "Player "<<(unsigned)player<<" end his turn."<<std::endl;
	cl->serv->wmx->lock();
	*cl->serv << ui16(100); //report that we ended turn
	cl->serv->wmx->unlock();
}
UpgradeInfo CCallback::getUpgradeInfo(const CArmedInstance *obj, int stackPos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getUpgradeInfo(const_cast<CArmedInstance*>(obj),stackPos);
}

const StartInfo * CCallback::getStartInfo()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->scenarioOps;
}

int CCallback::howManyTowns()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].towns.size();
}
const CGTownInstance * CCallback::getTownInfo(int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
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
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].heroes.size();
}
const CGHeroInstance * CCallback::getHeroInfo(int val, int mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	//if (gs->currentPlayer!=player) //TODO: checking if we are allowed to give that info
	//	return NULL;
	if (!mode) //esrial id
		if(val<gs->players[player].heroes.size())
			return gs->players[player].heroes[val];
		else return NULL;
	else if(mode==1) //it's hero type id
	{
		for (int i=0; i<gs->players[player].heroes.size();i++)
		{
			if (gs->players[player].heroes[i]->type->ID==val)
				return gs->players[player].heroes[i];
		}
	}
	else //object id
	{
		return static_cast<CGHeroInstance*>(gs->map->objects[val]);
	}
	return NULL;
}

int CCallback::getResourceAmount(int type)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].resources[type];
}
std::vector<si32> CCallback::getResourceAmount()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].resources;
}
int CCallback::getDate(int mode)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getDate(mode);
}
std::vector < std::string > CCallback::getObjDescriptions(int3 pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector<std::string> ret;
	if(!isVisible(pos,player))
		return ret;
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].blockingObjects)
		ret.push_back(obj->hoverName);
	return ret;
}
bool CCallback::verifyPath(CPath * path, bool blockSea)
{
	for (int i=0;i<path->nodes.size();i++)
	{
		if ( CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->blocked 
			&& (! (CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->visitable)))
			return false; //path is wrong - one of the tiles is blocked

		if (blockSea)
		{
			if (i==0)
				continue;

			if (
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype==water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype!=water))
				  ||
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype!=water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==water))
				  ||
				  (CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==rock)
					
				)
				return false;
		}


	}
	return true;
}

std::vector< std::vector< std::vector<unsigned char> > > & CCallback::getVisibilityMap()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].fogOfWarMap;
}


bool CCallback::isVisible(int3 pos, int Player)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[Player].fogOfWarMap[pos.x][pos.y][pos.z];
}

std::vector < const CGTownInstance *> CCallback::getTownsInfo(bool onlyOur)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGTownInstance *> ret = std::vector < const CGTownInstance *>();
	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (int j=0;j<(*i).second.towns.size();j++)
		{
			if ( ( isVisible((*i).second.towns[j],player) ) || (*i).first==player)
			{
				ret.push_back((*i).second.towns[j]);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CCallback::getHeroesInfo(bool onlyOur)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGHeroInstance *> ret;
	for(int i=0;i<gs->map->heroes.size();i++)
	{
		if(	 (gs->map->heroes[i]->tempOwner==player) ||
		   (isVisible(gs->map->heroes[i]->getPosition(false),player) && !onlyOur)	)
		{
			ret.push_back(gs->map->heroes[i]);
		}
	}
	return ret;
}

bool CCallback::isVisible(int3 pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return isVisible(pos,player);
}

bool CCallback::isVisible( CGObjectInstance *obj, int Player )
{
	//object is visible when at least one blocked tile is visible
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int3 pos = obj->pos + int3(fx-7,fy-5,0);
			if(gs->map->isInTheMap(pos) 
				&& !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1) 
				&& isVisible(pos,Player)  )
				return true;
		}
	}
	return false;
}
int CCallback::getMyColor()
{
	return player;
}
int CCallback::getHeroSerial(const CGHeroInstance * hero)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for (int i=0; i<gs->players[player].heroes.size();i++)
	{
		if (gs->players[player].heroes[i]==hero)
			return i;
	}
	return -1;
}
const CCreatureSet* CCallback::getGarrison(const CGObjectInstance *obj)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!obj)
		return NULL;
	if(obj->ID == 34)
		return &(dynamic_cast<const CGHeroInstance*>(obj))->army;
	else if(obj->ID == 98)
		return &(dynamic_cast<const CGTownInstance*>(obj)->army);
	else return NULL;
}

int CCallback::swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{
	if(s1->tempOwner != player   ||   s2->tempOwner != player)
		return -1;
	*cl->serv << ui16(502) << ui8(1) << s1->id << ui8(p1) << s2->id << ui8(p2);
	return 0;
}

int CCallback::mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{	
	if ((s1->tempOwner!= player  ||  s2->tempOwner!=player))
	{
		return -1;
	}
	*cl->serv << ui16(502) << ui8(2) << s1->id << ui8(p1) << s2->id << ui8(p2);
	return 0;
}
int CCallback::splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)
{
	if (s1->tempOwner!= player  ||  s2->tempOwner!=player || (!val))
	{
		return -1;
	}
	*cl->serv << ui16(502) << ui8(3) << s1->id << ui8(p1) << s2->id << ui8(p2) << si32(val);
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(player!=hero->tempOwner) return false;
	*cl->serv << ui16(500) << hero->id;
	return true;
}

int CCallback::getMySerial()
{	
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].serial;
}

bool CCallback::swapArifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)
{
	if(player!=hero1->tempOwner || player!=hero2->tempOwner)
		return false;
	*cl->serv << ui16(509) << hero1->id << pos1 << hero2->id << pos2;
	return true;
}

bool CCallback::buildBuilding(const CGTownInstance *town, si32 buildingID)
{
	CGTownInstance * t = const_cast<CGTownInstance *>(town);

	if(town->tempOwner!=player)
		return false;
	CBuilding *b = CGI->buildh->buildings[t->subID][buildingID];
	for(int i=0;i<7;i++)
		if(b->resources[i] > gs->players[player].resources[i])
			return false; //lack of resources

	*cl->serv << ui16(504) << town->id << buildingID;
	return true;
}

int CCallback::battleGetBattlefieldType()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return CGI->mh->ttiles[gs->curB->tile.x][gs->curB->tile.y][gs->curB->tile.z].tileInfo->tertype;
}

int CCallback::battleGetObstaclesAtTile(int tile) //returns bitfield 
{
	//TODO - write
	return -1;
}
int CCallback::battleGetStack(int pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->battleGetStack(pos);
}

CStack* CCallback::battleGetStackByID(int ID)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB) return NULL;
	return gs->curB->getStack(ID);
}

CStack* CCallback::battleGetStackByPos(int pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return battleGetStackByID(battleGetStack(pos));
}

int CCallback::battleGetPos(int stack)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for(int g=0; g<gs->curB->stacks.size(); ++g)
	{
		if(gs->curB->stacks[g]->ID == stack)
			return gs->curB->stacks[g]->position;
	}
	return -1;
}

std::map<int, CStack> CCallback::battleGetStacks()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::map<int, CStack> ret;
	if(!gs->curB) //there is no battle
	{
		return ret;
	}

	for(int g=0; g<gs->curB->stacks.size(); ++g)
	{
		ret[gs->curB->stacks[g]->ID] = *(gs->curB->stacks[g]);
	}
	return ret;
}

CCreature CCallback::battleGetCreature(int number)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for(int h=0; h<gs->curB->stacks.size(); ++h)
	{
		if(gs->curB->stacks[h]->ID == number) //creature found
			return *(gs->curB->stacks[h]->creature);
	}
#ifndef __GNUC__
	throw new std::exception("Cannot find the creature");
#else
	throw new std::exception();
#endif
}

std::vector<int> CCallback::battleGetAvailableHexes(int ID)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->curB->getAccessibility(ID);
	//return gs->battleGetRange(ID);
}

bool CCallback::battleIsStackMine(int ID)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for(int h=0; h<gs->curB->stacks.size(); ++h)
	{
		if(gs->curB->stacks[h]->ID == ID) //creature found
			return gs->curB->stacks[h]->owner == player;
	}
	return false;
}
bool CCallback::battleCanShoot(int ID, int dest) //TODO: finish
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(battleGetStackByID(ID)->creature->isShooting() 
		&& battleGetStack(dest) != -1 
		&& battleGetStackByPos(dest)->owner != battleGetStackByID(ID)->owner
		&& battleGetStackByPos(dest)->alive())
		return true;
	return false;
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner != player) return;
	*cl->serv << ui16(508) << si32(town->id);
}

void CCallback::buyArtifact(const CGHeroInstance *hero, int aid)
{
	if(hero->tempOwner != player) return;
	*cl->serv << ui16(510) << hero->id << ui32(aid);
}

std::vector < const CGObjectInstance * > CCallback::getBlockingObjs( int3 pos )
{
	std::vector<const CGObjectInstance *> ret;
	if(!gs->map->isInTheMap(pos))
		return ret;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].blockingObjects)
		ret.push_back(obj);
	return ret;
}

std::vector < const CGObjectInstance * > CCallback::getVisitableObjs( int3 pos )
{
	std::vector<const CGObjectInstance *> ret;
	if(!gs->map->isInTheMap(pos))
		return ret;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].visitableObjects)
		ret.push_back(obj);
	return ret;
}

void CCallback::getMarketOffer( int t1, int t2, int &give, int &rec, int mode/*=0*/ )
{
	if(mode) return; //TODO - support
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	//if(gs->resVals[t1] >= gs->resVals[t2])
	float r = gs->resVals[t1],
		g = gs->resVals[t2] / gs->getMarketEfficiency(player,mode);
	if(r>g)
	{
		rec = ceil(r / g);
		give = 1;
	}
	else
	{
		give = ceil(g / r);
		rec = 1;
	}
}

void CCallback::trade( int mode, int id1, int id2, int val1 )
{
	int p1, p2;
	getMarketOffer(id1,id2,p1,p2,mode);
	*cl->serv << ui16(511) << ui8(player) << ui32(mode)  << ui32(id1) << ui32(id2) << ui32(val1);
}