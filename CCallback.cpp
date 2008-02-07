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
		curd.owner = hero->getOwner();
		/*if(player!=-1)
		{
			hero->pos = endpos;
		}*/
		if((hero->movement>=CGI->mh->getCost(stpos, endpos, hero))  || player==-1)
		{ //performing move
			hero->movement-=CGI->mh->getCost(stpos, endpos, hero);
			
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
		return &(dynamic_cast<const CGTownInstance*>(obj)->garrison);
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
	//if(S1->slots[p1].first)
	{
		//if(s2->slots[p2].first)
		{
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
				CGI->playerint[s1->tempOwner]->garrisonChanged(s1);
			if((s2->tempOwner<PLAYER_LIMIT) && (s2 != s1))
				CGI->playerint[s2->tempOwner]->garrisonChanged(s2);
			return 0;
		}
	}
	return -1;
}

int CCallback::mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{
	return -1;
}
int CCallback::splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)
{
	return -1;
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
		std::cout << "Bohater o ID " << ID <<" (" <<CGI->heroh->heroes[ID]->name <<") dostaje "<<val<<" expa, ale nic z tym nie umiem zrobic :("<<std::endl;
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
		CGI->playerint[getHeroOwner(heroID)]->heroVisitsTown(CGI->state->getHero(heroID,0),n);
	}
	else
		return;
}

void CScriptCallback::stopHeroVisitCastle(CGObjectInstance * ob, int heroID)
{
	CGTownInstance * n;
	if(n = dynamic_cast<CGTownInstance*>(ob))
	{
		if(n->visitingHero->type->ID == heroID)
			n->visitingHero = NULL;
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