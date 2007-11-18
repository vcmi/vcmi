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
#include "CCallback.h"
#include "hch/CGeneralTextHandler.h"
#include <sstream>
bool getGlobalFunc(lua_State * L, std::string fname)
{
	unsigned int hash = lua_calchash(fname.c_str(), fname.size());
	lua_pushhstring(L, hash, fname.c_str(), fname.size());
	lua_gettable(L, LUA_GLOBALSINDEX);
	return lua_isfunction(L, -1);
}

CObjectScript::CObjectScript()
{
	language == ESLan::UNDEF;
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
		std::string temp = "Cannot open script ";
		temp += initpath;
		throw std::exception(temp.c_str());
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
		throw new std::exception((fname + ": function not defined").c_str()); // the call is not defined
	}
}
#undef LST

CLuaObjectScript::CLuaObjectScript(std::string filename)
{
	language == ESLan::LUA;
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
		throw new  std::exception(("Failed to call "+genFN("newObject",os->ID)+" function in lua script.").c_str());
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
		throw new  std::exception(("Failed to call "+genFN("heroVisit",os->ID)+" function in lua script.").c_str());
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
		throw new  std::exception(("Failed to call "+genFN("hoverText",os->ID)+" function in lua script.").c_str());
	}
	std::string ret = lua_tostring(is,1);
	lua_settop(is, 0);
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
		throw new std::exception("Skrypt nie zainicjalizowal instancji tego obiektu. :(");
	}
};
void CVisitableOPH::onNAHeroVisit(CGObjectInstance *os, int heroID, bool alreadyVisited)
{
	if (!alreadyVisited)
	{
		switch (os->ID)
		{
		case 51:
		case 23:
		case 61:
		case 32:
			{
				int w=0, ot=0;
				switch(os->ID)
				{
				case 51:
					w=0;
					ot=80;
					break;
				case 23:
					w=1;
					ot=29;
					break;
				case 61:
					w=2;
					ot=100;
					break;
				case 32:
					w=3;
					ot=59;
					break;
				}
				cb->changePrimSkill(heroID,w,1);
				std::vector<SComponent*> weko;
				weko.push_back(new SComponent(SComponent::primskill,1,1));
				cb->showInfoDialog(cb->getHeroOwner(heroID),CGI->objh->advobtxt[ot],&weko);
				for (int ii=0; ii<weko.size();ii++)
					delete weko[ii];
				break;
			}
		}
	}
}

std::vector<int> CVisitableOPH::yourObjects()
{
	std::vector<int> ret(4);
	ret.push_back(51);
	ret.push_back(23);
	ret.push_back(61);
	ret.push_back(32);
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
	default:
		throw new std::exception("Unsupported ID in CVisitableOPH::hoverText");
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


//std::string SComponent::getSubtitle()
//{
//	std::string ret;
//
//
//	return ret;
//}
//void SComponent::getDescription(Etype Type, int Subtype)
//{
//}
SComponent::SComponent(Etype Type, int Subtype, int Val)
{
	switch (Type)
	{
	case primskill:
		description = CGI->generaltexth->arraytxt[2+Subtype];
		std::ostringstream oss;
		oss << ((Val>0)?("+"):("-")) << Val << " " << CGI->heroh->pskillsn[Subtype];
		subtitle = oss.str();
		break;
	}
	type = Type;
	subtype = Subtype;
	val = Val;
}

SDL_Surface * SComponent::getImg()
{
	switch (type)
	{
	case primskill:
		return CGI->heroh->pskillsb[subtype].ourImages[0].bitmap;
		break;
	}
	return NULL;
}