#include "stdafx.h"
#include "CCallback.h"
#include "CPathfinder.h"
#include "hch\CHeroHandler.h"
#include "hch\CTownHandler.h"
#include "CGameInfo.h"
#include "hch\CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "CPlayerInterface.h"
#include "CLua.h"
#include "hch/CGeneralTextHandler.h"
#include "CAdvmapInterface.h"
#include "CPlayerInterface.h"
#include "hch/CBuildingHandler.h"
LUALIB_API int (luaL_error) (lua_State *L, const char *fmt, ...);

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
	for (std::set<CCPPObjectScript *>::iterator i=gs->cppscripts.begin();i!=gs->cppscripts.end();i++)
	{
		(*i)->newTurn();
	}
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		//handle heroes/////////////////////////////
		for (int j=0;j<(*i).second.heroes.size();j++)
		{
			(*i).second.heroes[j]->movement = valMovePoints((*i).second.heroes[j]);
		}


		//handle towns/////////////////////////////
		for(int j=0;j<i->second.towns.size();j++)
		{
			i->second.towns[j]->builded=0;
			if(getDate(1)==1) //first day of week
			{
				for(int k=0;k<CREATURES_PER_TOWN;k++)
				{
					if(i->second.towns[j]->creatureDwelling(k))//there is dwelling
						i->second.towns[j]->strInfo.creatures[k]+=i->second.towns[j]->creatureGrowth(k);
				}
			}
			if((gs->day>1) && i->first<PLAYER_LIMIT)
				i->second.resources[6]+=i->second.towns[j]->dailyIncome();
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
		curd.owner = hero->getOwner();
		/*if(player!=-1)
		{
			hero->pos = endpos;
		}*/
		if(hero->movement >= (ourPath->nodes.size()>=2 ?  (*(ourPath->nodes.end()-2)).dist : 0) - ourPath->nodes[i].dist  || player==-1)
		{ //performing move
			hero->movement -= (ourPath->nodes.size()>=2 ?  (*(ourPath->nodes.end()-2)).dist : 0) - ourPath->nodes[i].dist;
			ourPath->nodes.pop_back();
			
			std::vector< CGObjectInstance * > vis = CGI->mh->getVisitableObjs(CGHeroInstance::convertPosition(curd.dst,false));
			bool blockvis = false;
			for (int pit = 0; pit<vis.size();pit++)
				if (vis[pit]->blockVisit)
					blockvis = true;

			if (!blockvis)
			{
				curd.successful = true;
				hero->pos = curd.dst;

				//inform leaved objects
				std::vector< CGObjectInstance * > leave = CGI->mh->getVisitableObjs(CGHeroInstance::convertPosition(curd.src,false));
				for (int iii=0; iii<leave.size(); iii++) //if object is visitable we call onHeroVisit
				{
					//TODO: allow to handle this in LUA
					if(leave[iii]->state) //hard-coded function
						leave[iii]->state->onHeroLeave(leave[iii],curd.ho->subID);
				}


				//reveal fog of war
				int heroSight = hero->getSightDistance();
				int xbeg = stpos.x - heroSight - 2;
				if(xbeg < 0)
					xbeg = 0;
				int xend = stpos.x + heroSight + 2;
				if(xend >= CGI->ac->map.width)
					xend = CGI->ac->map.width;
				int ybeg = stpos.y - heroSight - 2;
				if(ybeg < 0)
					ybeg = 0;
				int yend = stpos.y + heroSight + 2;
				if(yend >= CGI->ac->map.height)
					yend = CGI->ac->map.height;
				for(int xd=xbeg; xd<xend; ++xd) //revealing part of map around heroes
				{
					for(int yd=ybeg; yd<yend; ++yd)
					{
						int deltaX = (hero->getPosition(false).x-xd)*(hero->getPosition(false).x-xd);
						int deltaY = (hero->getPosition(false).y-yd)*(hero->getPosition(false).y-yd);
						if(deltaX+deltaY<hero->getSightDistance()*hero->getSightDistance())
						{
							if(gs->players[player].fogOfWarMap[xd][yd][hero->getPosition(false).z] == 0)
							{
								CGI->playerint[gs->players[player].serial]->tileRevealed(int3(xd, yd, hero->getPosition(false).z));
							}
							gs->players[player].fogOfWarMap[xd][yd][hero->getPosition(false).z] = 1;
						}
					}
				}


				//notify interfacesabout move
				int nn=0; //number of interfece of currently browsed player
				for(std::map<int, PlayerState>::iterator j=CGI->state->players.begin(); j!=CGI->state->players.end(); ++j)//CGI->state->players.size(); ++j) //for testing
				{
					if (j->first > PLAYER_LIMIT)
						break;
					if(j->second.fogOfWarMap[stpos.x-1][stpos.y][stpos.z] || j->second.fogOfWarMap[endpos.x-1][endpos.y][endpos.z])
					{ //player should be notified
						CGI->playerint[j->second.serial]->heroMoved(curd);
					}
					++nn;
				}


				//call objects if they arevisited
				for (int iii=0; iii<vis.size(); iii++) //if object is visitable we call onHeroVisit
				{
					if(gs->checkFunc(vis[iii]->ID,"heroVisit")) //script function
						gs->objscr[vis[iii]->ID]["heroVisit"]->onHeroVisit(vis[iii],curd.ho->subID);
					if(vis[iii]->state) //hard-coded function
						vis[iii]->state->onHeroVisit(vis[iii],curd.ho->subID);
				}
			}
			else //interaction with blocking object (like resources)
			{
				curd.successful = false;
				CGI->playerint[gs->players[hero->getOwner()].serial]->heroMoved(curd);
				for (int iii=0; iii<vis.size(); iii++) //if object is visitable we call onHeroVisit
				{
					if (vis[iii]->blockVisit)
					{
						if(gs->checkFunc(vis[iii]->ID,"heroVisit")) //script function
							gs->objscr[vis[iii]->ID]["heroVisit"]->onHeroVisit(vis[iii],curd.ho->subID);
						if(vis[iii]->state) //hard-coded function
							vis[iii]->state->onHeroVisit(vis[iii],curd.ho->subID);
					}
				}
				return false;
			}

		}
		else
			return true; //move ended - no more movement points
	}
	return true;
}

void CCallback::selectionMade(int selection, int asker)
{
	//todo - jak bedzie multiplayer po sieci, to moze wymagac przerobek zaleznych od obranego modelu
	IChosen * ask = (IChosen *)asker;
	ask->chosen(selection);
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
		for(std::map<int,int>::iterator av=t->strInfo.creatures.begin();av!=t->strInfo.creatures.end();av++)
		{
			if(	(   found  = (ID == t->town->basicCreatures[av->first])   ) //creature is available among basic cretures
				|| (found  = (ID == t->town->upgradedCreatures[av->first]))			)//creature is available among upgraded cretures
			{
				amount = std::min(amount,av->second); //reduce recruited amount up to available amount
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
		std::pair<int,std::pair<CCreature*,int> > parb;	

		for(int i=0;i<7;i++) //TODO: if there is already stack of same creatures it should be used always
		{
			if((!t->army.slots[i].first) || (t->army.slots[i].first->idNumber == ID)) //slot is free or there is saem creature
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
			t->army.slots[slot].first = &CGI->creh->creatures[ID];
			t->army.slots[slot].second = amount;
		}
		CGI->playerint[gs->players[player].serial]->garrisonChanged(obj);

	}
	//TODO: recruit from dwellings on the adventure map
}


bool CCallback::dismissCreature(const CArmedInstance *obj, int stackPos)
{
	if((player>=0)  &&  obj->tempOwner != player)
		return false;
	CArmedInstance *ob = const_cast<CArmedInstance*>(obj);
	ob->army.slots.erase(stackPos);
	CGI->playerint[gs->players[player].serial]->garrisonChanged(obj);
	return true;
}
bool CCallback::upgradeCreature(const CArmedInstance *obj, int stackPos, int newID)
{
	//TODO: write
	return false;
}
UpgradeInfo CCallback::getUpgradeInfo(const CArmedInstance *obj, int stackPos)
{
	UpgradeInfo ret;
	CCreature *base = ((CArmedInstance*)obj)->army.slots[stackPos].first;
	if((obj->ID == 98)  ||  ((obj->ID == 34) && static_cast<const CGHeroInstance*>(obj)->visitedTown))
	{
		CGTownInstance * t;
		if(obj->ID == 98)
			t = static_cast<CGTownInstance *>(const_cast<CArmedInstance *>(obj));
		else
			t = static_cast<const CGHeroInstance*>(obj)->visitedTown;
		for(std::set<int>::iterator i=t->builtBuildings.begin();  i!=t->builtBuildings.end(); i++)
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
	return gs->players[player].resources[type];
}
std::vector<int> CCallback::getResourceAmount()
{
	return gs->players[player].resources;
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

std::vector < const CGTownInstance *> CCallback::getTownsInfo(bool onlyOur)
{
	std::vector < const CGTownInstance *> ret = std::vector < const CGTownInstance *>();
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
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
	std::vector < const CGHeroInstance *> ret = std::vector < const CGHeroInstance *>();
	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (int j=0;j<(*i).second.heroes.size();j++)
		{
			if ( ( isVisible((*i).second.heroes[j]->getPosition(false),player) ) || (*i).first==player)
			{
				ret.push_back((*i).second.heroes[j]);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
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
	CCreatureSet *S1 = const_cast<CCreatureSet*>(getGarrison(s1)), *S2 = const_cast<CCreatureSet*>(getGarrison(s2));
	if (false)
	{
		//TODO: check if we are allowed to swap these creatures
		return -1;
	}

	CCreature * pom = S2->slots[p2].first;
	S2->slots[p2].first = S1->slots[p1].first;
	S1->slots[p1].first = pom;
	int pom2 = S2->slots[p2].second;
	S2->slots[p2].second = S1->slots[p1].second;
	S1->slots[p1].second = pom2;

	if(!S1->slots[p1].first)
		S1->slots.erase(p1);
	if(!S2->slots[p2].first)
		S2->slots.erase(p2);

	if(s1->tempOwner<PLAYER_LIMIT)
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s1->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s1);
				break;
			}
		}
	}
	if((s2->tempOwner<PLAYER_LIMIT) && (s2 != s1))
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s2->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s2);
				break;
			}
		}
	}
	return 0;
}

int CCallback::mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{	
	CCreatureSet *S1 = const_cast<CCreatureSet*>(getGarrison(s1)), *S2 = const_cast<CCreatureSet*>(getGarrison(s2));
	if ((S1->slots[p1].first != S2->slots[p2].first) && (true /*we are allowed to*/))
	{
		return -1;
	}


	S2->slots[p2].second += S1->slots[p1].second;
	S1->slots[p1].first = NULL;
	S1->slots[p1].second = 0;

	S1->slots.erase(p1);

	if(s1->tempOwner<PLAYER_LIMIT)
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s1->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s1);
				break;
			}
		}
	}
	if((s2->tempOwner<PLAYER_LIMIT) && (s2 != s1))
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s2->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s2);
				break;
			}
		}
	}
	return 0;
}
int CCallback::splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)
{
	if(!val)
		return -1;
	CCreatureSet *S1 = const_cast<CCreatureSet*>(getGarrison(s1)), *S2 = const_cast<CCreatureSet*>(getGarrison(s2));
	if ((S1->slots[p1].second<val) && (true /*we are allowed to*/))
	{
		return -1;
	}

	S2->slots[p2].first = S1->slots[p1].first;
	S2->slots[p2].second = val;
	S1->slots[p1].second -= val;
	if(!S1->slots[p1].second) //if we've moved all creatures
		S1->slots.erase(p1); 


	if(s1->tempOwner<PLAYER_LIMIT)
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s1->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s1);
				break;
			}
		}
	}
	if((s2->tempOwner<PLAYER_LIMIT) && (s2 != s1))
	{
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == s2->tempOwner)
			{
				CGI->playerint[b]->garrisonChanged(s2);
				break;
			}
		}
	}
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	CGHeroInstance * Vhero = const_cast<CGHeroInstance *>(hero);
	CGI->mh->removeObject(Vhero);
	std::vector<CGHeroInstance*>::iterator nitr = find(CGI->state->players[player].heroes.begin(), CGI->state->players[player].heroes.end(), Vhero);
	CGI->state->players[player].heroes.erase(nitr);
	LOCPLINT->adventureInt->heroList.updateHList();
	return false;
}

int CCallback::getMySerial()
{	
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

bool CCallback::buildBuilding(const CGTownInstance *town, int buildingID)
{
	CGTownInstance * t = const_cast<CGTownInstance *>(town);
	CBuilding *b = CGI->buildh->buildings[t->subID][buildingID];

	if(0/*not allowed*/)//TODO: check if we are allowed to build
		return false;

	if(buildingID>36) //upg dwelling
	{
		if(t->getHordeLevel(0) == (buildingID-37))
			t->builtBuildings.insert(19);
		else if(t->getHordeLevel(1) == (buildingID-37))
			t->builtBuildings.insert(25);
	}
	else if(buildingID >= 30) //bas. dwelling
	{
		t->strInfo.creatures[buildingID-30] = CGI->creh->creatures[t->town->basicCreatures[buildingID-30]].growth;
	}

	t->builtBuildings.insert(buildingID);
	for(int i=0;i<7;i++)
		gs->players[player].resources[i]-=b->resources[i];
	t->builded++;
	CGI->playerint[CGI->state->players[player].serial]->buildChanged(town,buildingID,1);
	return true;
}

int CCallback::battleGetBattlefieldType()
{
	return CGI->mh->ttiles[CGI->state->curB->tile.x][CGI->state->curB->tile.y][CGI->state->curB->tile.z].terType;
}

int CCallback::battleGetObstaclesAtTile(int tile) //returns bitfield 
{
	//TODO - write
	return -1;
}
int CCallback::battleGetStack(int pos)
{
	for(int g=0; g<CGI->state->curB->stacks.size(); ++g)
	{
		if(CGI->state->curB->stacks[g]->position == pos ||
				( CGI->state->curB->stacks[g]->creature->isDoubleWide() && 
					( (CGI->state->curB->stacks[g]->attackerOwned && CGI->state->curB->stacks[g]->position-1 == pos) || 
						(!CGI->state->curB->stacks[g]->attackerOwned && CGI->state->curB->stacks[g]->position+1 == pos)
					) 
				)
			)
			return CGI->state->curB->stacks[g]->ID;
	}
	return -1;
}

CStack CCallback::battleGetStackByID(int ID)
{
	for(int g=0; g<CGI->state->curB->stacks.size(); ++g)
	{
		if(CGI->state->curB->stacks[g]->ID == ID)
			return *(CGI->state->curB->stacks[g]);
	}
	return CStack();
}

CStack CCallback::battleGetStackByPos(int pos)
{
	return battleGetStackByID(battleGetStack(pos));
}

int CCallback::battleGetPos(int stack)
{
	for(int g=0; g<CGI->state->curB->stacks.size(); ++g)
	{
		if(CGI->state->curB->stacks[g]->ID == stack)
			return CGI->state->curB->stacks[g]->position;
	}
	return -1;
}

std::map<int, CStack> CCallback::battleGetStacks()
{
	std::map<int, CStack> ret;
	for(int g=0; g<CGI->state->curB->stacks.size(); ++g)
	{
		ret[CGI->state->curB->stacks[g]->ID] = *(CGI->state->curB->stacks[g]);
	}
	return ret;
}

CCreature CCallback::battleGetCreature(int number)
{
	for(int h=0; h<CGI->state->curB->stacks.size(); ++h)
	{
		if(CGI->state->curB->stacks[h]->ID == number) //creature found
			return *(CGI->state->curB->stacks[h]->creature);
	}
	throw new std::exception("Cannot find the creature");
}

std::vector<int> CCallback::battleGetAvailableHexes(int ID)
{
	return CGI->state->battleGetRange(ID);
}

bool CCallback::battleIsStackMine(int ID)
{
	for(int h=0; h<CGI->state->curB->stacks.size(); ++h)
	{
		if(CGI->state->curB->stacks[h]->ID == ID) //creature found
			return CGI->state->curB->stacks[h]->owner == player;
	}
	return false;
}

int3 CScriptCallback::getPos(CGObjectInstance * ob)
{
	return ob->pos;
}
void CScriptCallback::changePrimSkill(int ID, int which, int val)
{	
	CGHeroInstance * hero = CGI->state->getHero(ID,0);
	if (which<PRIMARY_SKILLS)
	{
		hero->primSkills[which]+=val;
		for (int i=0; i<CGI->playerint.size(); i++)
		{
			if (CGI->playerint[i]->playerID == hero->getOwner())
			{
				CGI->playerint[i]->heroPrimarySkillChanged(hero, which, val);
				break;
			}
		}
	}
	else if (which==4)
	{
		hero->exp+=val;
		if(hero->exp >= CGI->heroh->reqExp(hero->level+1)) //new level
		{
			hero->level++;
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
			hero->primSkills[x]++;

			//TODO: dac dwie umiejetnosci 2-rzedne to wyboru

		}
		//TODO - powiadomic interfejsy, sprawdzic czy nie ma awansu itp
	}
}

int CScriptCallback::getHeroOwner(int heroID)
{
	CGHeroInstance * hero = CGI->state->getHero(heroID,0);
	return hero->getOwner();
}
void CScriptCallback::showInfoDialog(int player, std::string text, std::vector<SComponent*> * components)
{
	//TODO: upewniac sie ze mozemy to zrzutowac (przy customowych interfejsach cos moze sie kopnac)
	if (player>=0)
	{
		CGameInterface * temp = CGI->playerint[CGI->state->players[player].serial];
		if (temp->human)
			((CPlayerInterface*)(temp))->showInfoDialog(text,*components);
		return;
	}
	else
	{
		for (int i=0; i<CGI->playerint.size();i++)
		{
			if (CGI->playerint[i]->human)
				((CPlayerInterface*)(CGI->playerint[i]))->showInfoDialog(text,*components);
		}
	}
}

void CScriptCallback::showSelDialog(int player, std::string text, std::vector<CSelectableComponent*>*components, IChosen * asker)
{
	CGameInterface * temp = CGI->playerint[CGI->state->players[player].serial];
	if (temp->human)
		((CPlayerInterface*)(temp))->showSelDialog(text,*components,(int)asker);
	return;
}
int CScriptCallback::getSelectedHero()
{	
	int ret;
	if (LOCPLINT->adventureInt->selection.type == HEROI_TYPE)
		ret = ((CGHeroInstance*)(LOCPLINT->adventureInt->selection.selected))->subID;
	else 
		ret = -1;;
	return ret;
}
int CScriptCallback::getDate(int mode)
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
void CScriptCallback::giveResource(int player, int which, int val)
{
	gs->players[player].resources[which]+=val;
	CGI->playerint[gs->players[player].serial]->receivedResource(which,val);
}
void CScriptCallback::showCompInfo(int player, SComponent * comp)
{
	CPlayerInterface * i = dynamic_cast<CPlayerInterface*>(CGI->playerint[gs->players[player].serial]);
	if(i)
		i->showComp(*comp);
}
void CScriptCallback::heroVisitCastle(CGObjectInstance * ob, int heroID)
{
	CGTownInstance * n;
	if(n = dynamic_cast<CGTownInstance*>(ob))
	{
		n->visitingHero = CGI->state->getHero(heroID,0);
		CGI->state->getHero(heroID,0)->visitedTown = n;
		for(int b=0; b<CGI->playerint.size(); ++b)
		{
			if(CGI->playerint[b]->playerID == getHeroOwner(heroID))
			{
				CGI->playerint[b]->heroVisitsTown(CGI->state->getHero(heroID,0),n);
				break;
			}
		}
	}
	else
		return;
}

void CScriptCallback::stopHeroVisitCastle(CGObjectInstance * ob, int heroID)
{
	CGTownInstance * n;
	if(n = dynamic_cast<CGTownInstance*>(ob))
	{
		CGI->state->getHero(heroID,0)->visitedTown = NULL;
		if(n->visitingHero && n->visitingHero->type->ID == heroID)
			n->visitingHero = NULL;
		return;
	}
	else
		return;
}
void CScriptCallback::giveHeroArtifact(int artid, int hid, int position) //pos==-1 - first free slot in backpack
{
	CGHeroInstance* h = gs->getHero(hid,0);
	if(position<0)
	{
		for(int i=0;i<h->artifacts.size();i++)
		{
			if(!h->artifacts[i])
			{
				h->artifacts[i] = &CGI->arth->artifacts[artid];
				return;
			}
		}
		h->artifacts.push_back(&CGI->arth->artifacts[artid]);
		return;
	}
	else
	{
		if(h->artifWorn[position]) //slot is occupied
		{
			giveHeroArtifact(h->artifWorn[position]->id,hid,-1);
		}
		h->artifWorn[position] = &CGI->arth->artifacts[artid];
	}
}

void CScriptCallback::startBattle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2) //use hero=NULL for no hero
{
	gs->battle(army1,army2,tile,hero1,hero2);
}
void CScriptCallback::startBattle(int heroID, CCreatureSet * army, int3 tile) //for hero<=>neutral army
{
	CGHeroInstance* h = gs->getHero(heroID,0);
	gs->battle(&h->army,army,tile,h,NULL);
}
void CLuaCallback::registerFuncs(lua_State * L)
{
	lua_newtable(L);

#define REGISTER_C_FUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_C_FUNC(getPos);
	REGISTER_C_FUNC(changePrimSkill);
	REGISTER_C_FUNC(getGnrlText);
	REGISTER_C_FUNC(getSelectedHero);

	/*
	REGISTER_C_FUNC(changePrimSkill);
	REGISTER_C_FUNC(getGnrlText);
	REGISTER_C_FUNC(changePrimSkill);
	REGISTER_C_FUNC(getGnrlText);
	REGISTER_C_FUNC(changePrimSkill);
	REGISTER_C_FUNC(getGnrlText);*/
	

	lua_setglobal(L, "vcmi");
	#undef REGISTER_C_FUNC
}
int CLuaCallback::getPos(lua_State * L)//(CGObjectInstance * object);
{	
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1) )
		luaL_error(L,
			"Incorrect arguments to getPos([Object address])");
	CGObjectInstance * object = (CGObjectInstance *)(lua_tointeger(L, 1));
	lua_pushinteger(L,object->pos.x);
	lua_pushinteger(L,object->pos.y);
	lua_pushinteger(L,object->pos.z);
	return 3;
}
int CLuaCallback::changePrimSkill(lua_State * L)//(int ID, int which, int val);
{	
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1) ||
	    ((args >= 2) && !lua_isnumber(L, 2)) ||
	    ((args >= 3) && !lua_isnumber(L, 3))		)
	{
		luaL_error(L,
			"Incorrect arguments to changePrimSkill([Hero ID], [Which Primary skill], [Change by])");
	}
	int ID = lua_tointeger(L, 1),
		which = lua_tointeger(L, 2),
		val = lua_tointeger(L, 3);

	CScriptCallback::changePrimSkill(ID,which,val);

	return 0;
}
int CLuaCallback::getGnrlText(lua_State * L) //(int which),returns string
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1) )
		luaL_error(L,
			"Incorrect arguments to getGnrlText([Text ID])");
	int which = lua_tointeger(L,1);
	lua_pushstring(L,CGI->generaltexth->allTexts[which].c_str());
	return 1;
}
int CLuaCallback::getSelectedHero(lua_State * L) //(),returns int (ID of hero, -1 if no hero is seleceted)
{
	int ret;
	if (LOCPLINT->adventureInt->selection.type == HEROI_TYPE)
		ret = ((CGHeroInstance*)(LOCPLINT->adventureInt->selection.selected))->subID;
	else 
		ret = -1;
	lua_pushinteger(L,ret);
	return 1;
}
