#include "stdafx.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "hch/CHeroHandler.h"
#include "hch/CTownHandler.h"
#include "CGameInfo.h"
#include "hch/CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "CPlayerInterface.h"
#include "hch/CGeneralTextHandler.h"
#include "CAdvmapInterface.h"
#include "CPlayerInterface.h"
#include "hch/CBuildingHandler.h"
#include "hch/CObjectHandler.h"
#include "lib/Connection.h"
#include "client/Client.h"
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include "lib/NetPacks.h"
#include <boost/thread/shared_mutex.hpp>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
extern CSharedCond<std::set<IPack*> > mess;

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
	//todo - jak bedzie multiplayer po sieci, to moze wymagac przerobek zaleznych od obranego modelu
	//IChosen * ask = (IChosen *)asker;
	//ask->chosen(selection);
}
void CCallback::recruitCreatures(const CGObjectInstance *obj, int ID, int amount)
{
	if(amount<=0) return;
	if(obj->ID==98) //recruiting from town
	{
		int ser=-1; //used dwelling level
		CGTownInstance *t = const_cast<CGTownInstance*>(static_cast<const CGTownInstance*>(obj));

		//verify
		bool found = false;
		typedef std::pair<const int,int> Parka;
		for(std::map<si32,ui32>::iterator av=t->strInfo.creatures.begin();av!=t->strInfo.creatures.end();av++)
		{
			if(	(   found  = (ID == t->town->basicCreatures[av->first])   ) //creature is available among basic cretures
				|| (found  = (ID == t->town->upgradedCreatures[av->first]))			)//creature is available among upgraded cretures
			{
				amount = std::min(amount,(int)av->second); //reduce recruited amount up to available amount
				ser = av->first;
				break;
			}
		}
		if(!found)	//no such creature
			return;

		if(amount > CGI->creh->creatures[ID].maxAmount(gs->players[player].resources))
			return; //not enough resources

		//for(int i=0;i<RESOURCE_QUANTITY;i++)
		//	if (gs->players[player].resources[i]  <  (CGI->creh->creatures[ID].cost[i] * amount))
		//		return; //not enough resources

		if(amount<=0) return;

		//recruit
		int slot = -1; //slot ID
		std::pair<si32,std::pair<ui32,si32> > parb;	

		for(int i=0;i<7;i++) //TODO: if there is already stack of same creatures it should be used always
		{
			if(((!t->army.slots[i].first) && (!t->army.slots[i].second)) || (t->army.slots[i].first == ID)) //slot is free or there is saem creature
			{
				slot = i;
				break;
			}
		}

		if(slot<0) //no free slot
			return;

		for(int i=0;i<RESOURCE_QUANTITY;i++)
			gs->players[player].resources[i]  -=  (CGI->creh->creatures[ID].cost[i] * amount);

		t->strInfo.creatures[ser] -= amount;
		if(t->army.slots[slot].first) //add new creatures to the existing stack
		{
			t->army.slots[slot].second += amount;
		}
		else //create new stack in the garrison
		{
			t->army.slots[slot].first = ID;
			t->army.slots[slot].second = amount;
		}
		cl->playerint[player]->garrisonChanged(obj);

	}
	//TODO: recruit from dwellings on the adventure map
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
	//TODO: write
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
	UpgradeInfo ret;
	CCreature *base = &CGI->creh->creatures[((CArmedInstance *)obj)->army.slots[stackPos].first];
	if((obj->ID == 98)  ||  ((obj->ID == 34) && static_cast<const CGHeroInstance*>(obj)->visitedTown))
	{
		CGTownInstance * t;
		if(obj->ID == 98)
			t = static_cast<CGTownInstance *>(const_cast<CArmedInstance *>(obj));
		else
			t = static_cast<const CGHeroInstance*>(obj)->visitedTown;
		for(std::set<si32>::iterator i=t->builtBuildings.begin();  i!=t->builtBuildings.end(); i++)
		{
			if( (*i) >= 37   &&   (*i) < 44 ) //upgraded creature dwelling
			{
				int nid = t->town->upgradedCreatures[(*i)-37]; //upgrade offered by that building
				if(base->upgrades.find(nid) != base->upgrades.end()) //possible upgrade
				{
					ret.newID.push_back(nid);
					ret.cost.push_back(std::set<std::pair<int,int> >());
					for(int j=0;j<RESOURCE_QUANTITY;j++)
					{
						int dif = CGI->creh->creatures[nid].cost[j] - base->cost[j];
						if(dif)
							ret.cost[ret.cost.size()-1].insert(std::make_pair(j,dif));
					}
				}
			}
		}//end for
	}
	//TODO: check if hero ability makes some upgrades possible

	if(ret.newID.size())
		ret.oldID = base->idNumber;

	return ret;
}

const StartInfo * CCallback::getStartInfo()
{
	return gs->scenarioOps;
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
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].heroes.size();
}
const CGHeroInstance * CCallback::getHeroInfo(int player, int val, bool mode) //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if (gs->currentPlayer!=player) //TODO: checking if we are allowed to give that info
		return NULL;
	if (!mode)
		if(val<gs->players[player].heroes.size())
			return gs->players[player].heroes[val];
		else return NULL;
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
			if ( ( isVisible((*i).second.towns[j]->pos,player) ) || (*i).first==player)
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
	return isVisible(pos,player);
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

bool CCallback::swapArifacts(const CGHeroInstance * hero1, bool worn1, int pos1, const CGHeroInstance * hero2, bool worn2, int pos2)
{
	if(!hero1 || !hero2) //incorrect data
		return false;
	CGHeroInstance * Uhero1 = const_cast<CGHeroInstance *>(hero1);
	CGHeroInstance * Uhero2 = const_cast<CGHeroInstance *>(hero2);

	if(worn1 && worn2)
	{
		std::swap(Uhero1->artifWorn[pos1], Uhero2->artifWorn[pos2]);
	}
	else if(worn1 && !worn2)
	{
		std::swap(Uhero1->artifWorn[pos1], Uhero2->artifacts[pos2]);
	}
	else if(!worn1 && worn2)
	{
		std::swap(Uhero1->artifacts[pos1], Uhero2->artifWorn[pos2]);
	}
	else
	{
		std::swap(Uhero1->artifacts[pos1], Uhero2->artifacts[pos2]);
	}
	
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
//TODO: check if we are allowed to build
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
		&& battleGetStackByPos(dest)->alive)
		return true;
	return false;
}
