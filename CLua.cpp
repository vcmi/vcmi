#include "stdafx.h"
#include "CLua.h"
#include "CLuaHandler.h"
#include "hch/CHeroHandler.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lobject.h"
#include "lgc.h"
#include "lapi.h"
#include "CGameInfo.h"
#include "CGameState.h"
#include <sstream>
#include "hch/CObjectHandler.h"
#include "hch/CTownHandler.h"
#include "CCallback.h"
#include "hch/CGeneralTextHandler.h"
#include <sstream>
#include "CPlayerInterface.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "hch/CDefObjInfoHandler.h"
#pragma warning (disable : 4311)
bool getGlobalFunc(lua_State * L, std::string fname)
{
	unsigned int hash = lua_calchash(fname.c_str(), fname.size());
	lua_pushhstring(L, hash, fname.c_str(), fname.size());
	lua_gettable(L, LUA_GLOBALSINDEX);
	return lua_isfunction(L, -1);
}

CObjectScript::CObjectScript()
{
	language = UNDEF;
	//std::cout << "Tworze obiekt objectscript "<<this<<std::endl;
}

CObjectScript::~CObjectScript()
{
	//std::cout << "Usuwam obiekt objectscript "<<this<<std::endl;
}

CScript::CScript()
{
	//std::cout << "Tworze obiekt CScript "<<this<<std::endl;
}
CScript::~CScript()
{
	//std::cout << "Usuwam obiekt CScript "<<this<<std::endl;
}

#define LST (is)
CLua::CLua(std::string initpath)
{
	opened=false;
	open(initpath);
}
CLua::CLua()
{
	//std::cout << "Tworze obiekt clua "<<this<<std::endl;
	opened=false;
}
void CLua::open(std::string initpath)
{
	LST = lua_open();
	opened = true;
	LUA_OPEN_LIB(LST, luaopen_base);
	LUA_OPEN_LIB(LST, luaopen_io);
	if ((luaL_loadfile (LST, initpath.c_str())) == 0)
	{
		lua_pcall (LST, 0, LUA_MULTRET, 0);
	}
	else
	{
#ifndef __GNUC__
		std::string temp = "Cannot open script ";
		temp += initpath;
		throw std::exception(temp.c_str());
#else
		throw std::exception();
#endif
	}
}
void CLua::registerCLuaCallback()
{
}

CLua::~CLua()
{
	//std::cout << "Usuwam obiekt clua "<<this<<std::endl;
	if (opened)
	{
		std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
		lua_close(LST);
	}
}

void CLua::findF(std::string fname)
{
	 lua_getfield(is, LUA_GLOBALSINDEX, fname.c_str()); /* function to be called */
}
void CLua::findF2(std::string fname)
{
	lua_pushstring (is, fname.c_str());
	lua_gettable (is, LUA_GLOBALSINDEX);
}
void CLua::findFS(std::string fname)
{
	lua_settop(is, 0);
	if (!getGlobalFunc(is,fname))
	{
		lua_settop(is, 0);
#ifndef __GNUC__
		throw new std::exception((fname + ": function not defined").c_str()); // the call is not defined
#else
		throw std::exception();
#endif
	}
}
#undef LST

CLuaObjectScript::CLuaObjectScript(std::string filename)
{
	language = LUA;
	open(filename);
	//binit = bnewobject = bonherovisit = brightext = false;
	//std::cout << "Tworze obiekt CLuaObjectScript "<<this<<std::endl;
}
CLuaObjectScript::~CLuaObjectScript()
{
	//std::cout << "Usuwam obiekt CLuaObjectScript "<<this<<std::endl;
}

void CLuaObjectScript::init()
{
}

std::string CLuaObjectScript::genFN(std::string base, int ID)
{
	std::stringstream sts;
	sts<<base<<"_"<<ID;
	return sts.str();
}

void CLuaObjectScript::newObject(CGObjectInstance *os)
{
	findF(genFN("newObject",os->ID));
	lua_pushinteger(is, (int)os);
	if (lua_pcall (is, 1, 0, 0))
	{
		lua_settop(is, 0);
#ifndef __GNUC__
		throw new  std::exception(("Failed to call "+genFN("newObject",os->ID)+" function in lua script.").c_str());
#else
		throw std::exception();
#endif
	}
	lua_settop(is, 0);
	return;
}
void CLuaObjectScript::onHeroVisit(CGObjectInstance *os, int heroID)
{
	findF(genFN("heroVisit",os->ID));
	lua_pushinteger(is, (int)os);
	lua_pushinteger(is, heroID);
	if (lua_pcall (is, 2, 0, 0))
	{
		lua_settop(is, 0);
#ifndef __GNUC__
		throw new  std::exception(("Failed to call "+genFN("heroVisit",os->ID)+" function in lua script.").c_str());
#else
		throw std::exception();
#endif
	}
	lua_settop(is, 0);
}
std::string CLuaObjectScript::hoverText(CGObjectInstance *os)
{
	findF(genFN("hoverText",os->ID));
	lua_pushinteger(is, (int)os);
	if (lua_pcall (is, 1, 1, 0))
	{
		lua_settop(is, 0);
#ifndef __GNUC__
		throw new  std::exception(("Failed to call "+genFN("hoverText",os->ID)+" function in lua script.").c_str());
#else
		throw std::exception();
#endif
	}
	std::string ret = lua_tostring(is,1);
	lua_settop(is, 0);
	return ret;
}

std::string CCPPObjectScript::hoverText(CGObjectInstance *os)
{
	return CGI->objh->objects[os->defInfo->id].name;
}

void CVisitableOPH::newObject(CGObjectInstance *os)
{
	visitors.insert
		(std::pair<CGObjectInstance*,std::set<int> >(os,std::set<int>()));
};

void CVisitableOPH::onHeroVisit(CGObjectInstance *os, int heroID)
{
	if (visitors.find(os)!=visitors.end())
	{
		if(visitors[os].find(heroID)==visitors[os].end())
		{
			onNAHeroVisit(os,heroID, false);
			visitors[os].insert(heroID);
		}
		else
		{
			onNAHeroVisit(os,heroID, true);
		}
	}
	else
	{
#ifndef __GNUC__
		throw new std::exception("Skrypt nie zainicjalizowal instancji tego obiektu. :(");
#else
		throw std::exception();
#endif
	}
};
void CVisitableOPH::onNAHeroVisit(CGObjectInstance *os, int heroID, bool alreadyVisited)
{
	int w=0, ot=0, vvv=1;
	switch(os->ID)
	{
	case 51:
		w=0;
		ot=80;
		break;
	case 23:
		w=1;
		ot=39;
		break;
	case 61:
		w=2;
		ot=100;
		break;
	case 32:
		w=3;
		ot=59;
		break;
	case 100:
		w=4;
		ot=143;
		vvv=1000;
		break;
	}
	if (!alreadyVisited)
	{
		switch (os->ID)
		{
		case 51:
		case 23:
		case 61:
		case 32:
			{
				cb->changePrimSkill(heroID,w,vvv);
				std::vector<SComponent*> weko;
				weko.push_back(new SComponent(SComponent::primskill,w,vvv));
				cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[ot],&weko);
				//for (int ii=0; ii<weko.size();ii++)
				//	delete weko[ii];
				break;
			}
		case 100:
			{
				cb->changePrimSkill(heroID,w,vvv);
				std::vector<SComponent*> weko;
				weko.push_back(new SComponent(SComponent::experience,0,vvv));
				cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[ot],&weko);
				//for (int ii=0; ii<weko.size();ii++)
				//	delete weko[ii];
				break;
			}
		}
	}
	else
	{
		ot++;
		cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[ot],&std::vector<SComponent*>());
	}
}

std::vector<int> CVisitableOPH::yourObjects()
{
	std::vector<int> ret(5);
	ret.push_back(51);
	ret.push_back(23);
	ret.push_back(61);
	ret.push_back(32);
	ret.push_back(100);
	return ret;
}

std::string CVisitableOPH::hoverText(CGObjectInstance *os)
{
	std::string add;
	int pom;
	switch(os->ID)
	{
	case 51:
		pom = 8;
		break;
	case 23:
		pom = 7;
		break;
	case 61:
		pom = 11;
		break;
	case 32:
		pom = 4;
		break;
	case 100:
		pom = 5;
		break;
	default:
#ifndef __GNUC__
		throw new std::exception("Unsupported ID in CVisitableOPH::hoverText");
#else
		throw std::exception();
#endif
	}
	add = " " + CGI->objh->xtrainfo[pom] + " ";
	int heroID = cb->getSelectedHero();
	if (heroID>=0)
	{
		add += ( (visitors[os].find(heroID) == visitors[os].end())
				?
			(CGI->generaltexth->allTexts[353])  //not visited
				:
			( CGI->generaltexth->allTexts[352]) ); //visited
	}
	return CGI->objh->objects[os->defInfo->id].name + add;
}

void CVisitableOPW::onNAHeroVisit(CGObjectInstance *os, int heroID, bool alreadyVisited)
{
	int mid;
	switch (os->ID)
	{
	case 55:
		mid = 92;
		break;
	case 112:
		mid = 170;
		break;
	case 109:
		mid = 164;
		break;
	}
	if (alreadyVisited)
	{
		if (os->ID!=112)
			mid++;
		else
			mid--;
		cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[mid],&std::vector<SComponent*>()); //TODO: maybe we have memory leak with these windows
	}
	else
	{
		int type, sub, val;
		type = SComponent::resource;
		switch (os->ID)
		{
		case 55:
			if (rand()%2)
			{
				sub = 5;
				val = 5;
			}
			else
			{
				sub = 6;
				val = 500;
			}
			break;
		case 112:
			mid = 170;
			sub = (rand() % 5) + 1;
			val = (rand() % 4) + 3;
			break;
		case 109:
			mid = 164;
			sub = 6;
			if(cb->getDate(2)<2)
				val = 500;
			else
				val = 1000;
		}
		SComponent * com = new SComponent((SComponent::Etype)type,sub,val);
		std::vector<SComponent*> weko;
		weko.push_back(com);
		cb->giveResource(cb->getHeroOwner(heroID),sub,val);
		cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[mid],&weko);
		visited[os] = true;
	}
}
void CVisitableOPW::newTurn ()
{
	if (cb->getDate(1)==1)
	{
		for (std::map<CGObjectInstance*,bool>::iterator i = visited.begin(); i != visited.end(); i++)
		{
			(*i).second = false;
		}
	}
}
void CVisitableOPW::newObject(CGObjectInstance *os)
{
	visited.insert(std::pair<CGObjectInstance*,bool>(os,false));
}

void CVisitableOPW::onHeroVisit(CGObjectInstance *os, int heroID)
{
	if(visited[os])
		onNAHeroVisit(os,heroID,true);
	else
		onNAHeroVisit(os,heroID,false);
}

std::vector<int> CVisitableOPW::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(3);
	ret.push_back(55); //mystical garden
	ret.push_back(112); //windmill
	ret.push_back(109); //water wheel
	return ret;
}

std::string CVisitableOPW::hoverText(CGObjectInstance *os)
{
	return CGI->objh->objects[os->defInfo->id].name + " " + ( (visited[os]) ? (CGI->generaltexth->allTexts[352]) : (CGI->generaltexth->allTexts[353]))  ;
}

void CMines::newObject(CGObjectInstance *os)
{
	ourObjs.push_back(os);
	os->tempOwner = NEUTRAL_PLAYER;
}
void CMines::onHeroVisit(CGObjectInstance *os, int heroID)
{
	int vv = 1;
	if (os->subID==0 || os->subID==2)
		vv++;
	else if (os->subID==6)
		vv = 1000;
	if (os->tempOwner == cb->getHeroOwner(heroID))
	{
		//TODO: garrison
	}
	else
	{
		if (os->subID==7)
			return; //TODO: support for abandoned mine
		os->tempOwner = cb->getHeroOwner(heroID);
		SComponent * com = new SComponent(SComponent::resource,os->subID,vv);
		com->subtitle+=CGI->generaltexth->allTexts[3].substr(2,CGI->generaltexth->allTexts[3].length()-2);
		std::vector<SComponent*> weko;
		weko.push_back(com);
		cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->mines[os->subID].second,&weko);
	}
}
std::vector<int> CMines::yourObjects()
{
	std::vector<int> ret(1);
	ret.push_back(53);
	return ret;
}
std::string CMines::hoverText(CGObjectInstance *os)
{
	if (os->tempOwner == NEUTRAL_PLAYER)
		return CGI->objh->mines[os->subID].first;
	else
		return CGI->objh->mines[os->subID].first + " " + CGI->generaltexth->arraytxt[23+os->tempOwner];

}
void CMines::newTurn ()
{
	for (int i=0;i<ourObjs.size();i++)
	{
		if (ourObjs[i]->tempOwner == NEUTRAL_PLAYER)
			continue;
		int vv = 1;
		if (ourObjs[i]->subID==0 || ourObjs[i]->subID==2)
			vv++;
		else if (ourObjs[i]->subID==6)
			vv = 1000;
		cb->giveResource(ourObjs[i]->tempOwner,ourObjs[i]->subID,vv);
	}
}


void CPickable::newObject(CGObjectInstance *os)
{
	os->blockVisit = true;
}
void CPickable::onHeroVisit(CGObjectInstance *os, int heroID)
{
	switch(os->ID)
	{
	case 5:
		{
			cb->giveHeroArtifact(os->subID,heroID,-1); //TODO: na pozycje
			break;
		}
	case 79:
		{
			//TODO: handle guards (when battles are finished)
			CResourceObjInfo * t2 = static_cast<CResourceObjInfo *>(os->info);
			int val;
			if(t2->amount)
				val = t2->amount;
			else
			{
				switch(os->subID)
				{
				case 6:
					val = 500 + (rand()%6)*100;
					break;
				case 0: case 2:
					val = 6 + (rand()%5);
					break;
				default:
					val = 3 + (rand()%3);
					break;
				}
			}
			if(t2->message.length())
				cb->showInfoDialog(cb->getHeroOwner(heroID),t2->message,&std::vector<SComponent*>());
			SComponent ccc(SComponent::resource,os->subID,val);
			ccc.description = CGI->objh->advobtxt[113];
			boost::algorithm::replace_first(ccc.description,"%s",CGI->objh->restypes[os->subID]);
			cb->giveResource(cb->getHeroOwner(heroID),os->subID,val);
			cb->showCompInfo(cb->getHeroOwner(heroID),&ccc);
			break;
		}
	case 101:
		{
			if (os->subID)
				break; //not OH3 treasure chest
			int wyn = rand()%100;
			if (wyn<32)
			{
				tempStore.push_back(new CSelectableComponent(SComponent::resource,6,1000));
				tempStore.push_back(new CSelectableComponent(SComponent::experience,0,500));
			}//1k/0.5k
			else if(wyn<64)
			{
				tempStore.push_back(new CSelectableComponent(SComponent::resource,6,1500));
				tempStore.push_back(new CSelectableComponent(SComponent::experience,0,1000));
			}//1.5k/1k
			else if(wyn<95)
			{
				tempStore.push_back(new CSelectableComponent(SComponent::resource,6,2000));
				tempStore.push_back(new CSelectableComponent(SComponent::experience,0,1500));
			}//2k/1.5k
			else
			{
				if (1/*TODO: backpack is full*/)
				{
					tempStore.push_back(new CSelectableComponent(SComponent::resource,6,1000));
					tempStore.push_back(new CSelectableComponent(SComponent::experience,0,500));
				}
				else
				{
					//TODO: give treasure artifact
					break;
				}
			}//random treasure artifact, or (if backapack is full) 1k/0.5k
			tempStore[1]->ID = heroID;
			player = cb->getHeroOwner(heroID);
			cb->showSelDialog(player,CGI->objh->advobtxt[146],&tempStore,this);
			break;
		}
	}
	CGI->mh->removeObject(os);
}
void CPickable::chosen(int which)
{
	switch(tempStore[which]->type)
	{
	case SComponent::resource:
		cb->giveResource(player,tempStore[which]->subtype,tempStore[which]->val);
		break;
	case SComponent::experience:
		cb->changePrimSkill(tempStore[which]->ID,4,tempStore[which]->val);
		break;
	default:
#ifndef __GNUC__
		throw new std::exception("Unhandled choice");
#else
		throw std::exception();
#endif

	}
	for (int i=0;i<tempStore.size();i++)
		delete tempStore[i];
	tempStore.clear();
}

std::string CPickable::hoverText(CGObjectInstance *os)
{
	switch (os->ID)
	{
	case 79:
		return CGI->objh->restypes[os->subID];
		break;
	case 5:
		return CGI->arth->artifacts[os->subID].name;
		break;
	default:
		return CGI->objh->objects[os->defInfo->id].name;
		break;
	}
}

std::vector<int> CPickable::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(3);
	ret.push_back(79); //resource
	ret.push_back(5); //artifact
	ret.push_back(101); //treasure chest / commander stone
	return ret;
}

void CTownScript::onHeroVisit(CGObjectInstance *os, int heroID)
{
	cb->heroVisitCastle(os,heroID);
}
void CTownScript::onHeroLeave(CGObjectInstance *os, int heroID)
{
	cb->stopHeroVisitCastle(os,heroID);
}
std::string CTownScript::hoverText(CGObjectInstance *os)
{
	CGTownInstance * n;
	if(n = dynamic_cast<CGTownInstance*>(os))
		return n->name + ", " + n->town->name;
	else return "";
}

std::vector<int> CTownScript::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(98); //town
	return ret;
}

void CHeroScript::newObject(CGObjectInstance *os)
{
	os->blockVisit = true;
	heroes.insert(std::pair<int,CGObjectInstance*>(os->subID,os));
}

void CHeroScript::onHeroVisit(CGObjectInstance *os, int heroID)
{
	//TODO: check for allies
	if(static_cast<CGHeroInstance*>(heroes[heroID])->tempOwner == static_cast<CGHeroInstance*>(os)->tempOwner) //one of allied cases
	{
		//exchange
	}
	else
	{
		cb->startBattle(
			&(static_cast<CGHeroInstance*>(heroes[heroID]))->army,
			&(static_cast<CGHeroInstance*>(os))->army,
			os->pos,
			static_cast<CGHeroInstance*>(heroes[heroID]),
			static_cast<CGHeroInstance*>(os));
	}
}
std::vector<int> CHeroScript::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(34); //hero
	return ret;
}
std::string CHeroScript::hoverText(CGObjectInstance *os)
{
	CGHeroInstance* h = static_cast<CGHeroInstance*>(os);
	std::string ret = CGI->generaltexth->allTexts[15];
	boost::algorithm::replace_first(ret,"%s",h->name);
	boost::algorithm::replace_first(ret,"%s",h->type->heroClass->name);
	return ret;
}
void CMonsterS::newObject(CGObjectInstance *os)
{
	//os->blockVisit = true;
	switch(CGI->creh->creatures[os->subID].level)
	{
	case 1:
		((CCreatureObjInfo*)os->info)->number = rand()%31+20;
		break;
	case 2:
		((CCreatureObjInfo*)os->info)->number = rand()%16+15;
		break;
	case 3:
		((CCreatureObjInfo*)os->info)->number = rand()%16+10;
		break;
	case 4:
		((CCreatureObjInfo*)os->info)->number = rand()%11+10;
		break;
	case 5:
		((CCreatureObjInfo*)os->info)->number = rand()%9+8;
		break;
	case 6:
		((CCreatureObjInfo*)os->info)->number = rand()%8+5;
		break;
	case 7:
		((CCreatureObjInfo*)os->info)->number = rand()%7+3;
		break;
	case 8:
		((CCreatureObjInfo*)os->info)->number = rand()%4+2;
		break;
	case 9:
		((CCreatureObjInfo*)os->info)->number = rand()%3+2;
		break;
	case 10:
		((CCreatureObjInfo*)os->info)->number = rand()%3+1;
		break;

	}

}
std::string CMonsterS::hoverText(CGObjectInstance *os)
{
	int pom = CCreature::getQuantityID(((CCreatureObjInfo*)os->info)->number);
	pom = 174 + 3*pom + 1;
	return CGI->generaltexth->arraytxt[pom] + " " + CGI->creh->creatures[os->subID].namePl;
}
void CMonsterS::onHeroVisit(CGObjectInstance *os, int heroID)
{
	CCreatureSet set;
	//TODO: zrobic secik w sposob wyrafinowany
	set.slots[0] = std::pair<CCreature*,int>(&CGI->creh->creatures[os->subID],((CCreatureObjInfo*)os->info)->number);
	cb->startBattle(heroID,&set,os->pos);
}
std::vector<int> CMonsterS::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(54); //monster
	return ret;
}


void CCreatureGen::newObject(CGObjectInstance *os)
{
	amount[os] = CGI->creh->creatures[CGI->objh->cregens[os->subID]].growth;
}
std::string CCreatureGen::hoverText(CGObjectInstance *os)
{
	return CGI->objh->creGens[os->subID];
}
void CCreatureGen::onHeroVisit(CGObjectInstance *os, int heroID)
{
}
std::vector<int> CCreatureGen::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(17); //cregen1
	return ret;
}
