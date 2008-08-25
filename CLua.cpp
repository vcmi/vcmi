#include "stdafx.h"
#include <sstream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CTownHandler.h"
#include "hch/CArtHandler.h"
#include "hch/CDefObjInfoHandler.h"
//#include "lua.h"
//#include "lualib.h"
//#include "lauxlib.h"
//#include "lobject.h"
//#include "lgc.h"
//#include "lapi.h"
#include "CLua.h"
#include "CGameState.h"
#include "lib/VCMI_Lib.h"
#include "map.h"
#include "server/CScriptCallback.h"
#include "lib/NetPacks.h"
#pragma warning (disable : 4311)
#define DEFOS const CGObjectInstance *os = cb->getObj(objid)
bool getGlobalFunc(lua_State * L, std::string fname)
{
	//unsigned int hash = lua_calchash(fname.c_str(), fname.size());
	//lua_pushhstring(L, hash, fname.c_str(), fname.size());
	//lua_gettable(L, LUA_GLOBALSINDEX);
	//return lua_isfunction(L, -1);
	return false;
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
	//LST = lua_open();
	//opened = true;
	//LUA_OPEN_LIB(LST, luaopen_base);
	//LUA_OPEN_LIB(LST, luaopen_io);
	//if ((luaL_loadfile (LST, initpath.c_str())) == 0)
	//{
	//	lua_pcall (LST, 0, LUA_MULTRET, 0);
	//}
	//else
	//{
	//	std::string temp = "Cannot open script ";
	//	temp += initpath;
	//	throw std::exception(temp.c_str());
	//}
}
void CLua::registerCLuaCallback()
{
}

CLua::~CLua()
{
	////std::cout << "Usuwam obiekt clua "<<this<<std::endl;
	//if (opened)
	//{
	//	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
	//	lua_close(LST);
	//}
}

void CLua::findF(std::string fname)
{
	// lua_getfield(is, LUA_GLOBALSINDEX, fname.c_str()); /* function to be called */
}
void CLua::findF2(std::string fname)
{
	//lua_pushstring (is, fname.c_str());
	//lua_gettable (is, LUA_GLOBALSINDEX); 
}
void CLua::findFS(std::string fname)
{
	//lua_settop(is, 0);
	//if (!getGlobalFunc(is,fname)) 
	//{
	//	lua_settop(is, 0);
	//	throw new std::exception((fname + ": function not defined").c_str()); // the call is not defined
	//}
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

void CLuaObjectScript::newObject(int objid)
{
	//findF(genFN("newObject",os->ID));
	//lua_pushinteger(is, (int)os);
	//if (lua_pcall (is, 1, 0, 0))
	//{
	//	lua_settop(is, 0);
	//	throw new  std::exception(("Failed to call "+genFN("newObject",os->ID)+" function in lua script.").c_str());
	//}
	//lua_settop(is, 0);
	return;
}
void CLuaObjectScript::onHeroVisit(int objid, int heroID)
{
	//findF(genFN("heroVisit",os->ID));
	//lua_pushinteger(is, (int)os);
	//lua_pushinteger(is, heroID);
	//if (lua_pcall (is, 2, 0, 0))
	//{
	//	lua_settop(is, 0);
	//	throw new  std::exception(("Failed to call "+genFN("heroVisit",os->ID)+" function in lua script.").c_str());
	//}
	//lua_settop(is, 0);
}
//std::string CLuaObjectScript::hoverText(int objid)
//{
//	//findF(genFN("hoverText",os->ID));
//	//lua_pushinteger(is, (int)os);
//	//if (lua_pcall (is, 1, 1, 0))
//	//{
//	//	lua_settop(is, 0);
//	//	throw new  std::exception(("Failed to call "+genFN("hoverText",os->ID)+" function in lua script.").c_str());
//	//}
//	//std::string ret = lua_tostring(is,1);
//	//lua_settop(is, 0);
//	return "";
//}

void CVisitableOPH::newObject(int objid)
{
	visitors.insert
		(std::pair<int,std::set<int> >(objid,std::set<int>()));

	DEFOS;
	MetaString hovername;
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
	case 102:
		typeOfTree[objid] = rand()%3;
		return;
	default:
		std::cout << "Unsupported ID in CVisitableOPH::hoverText" << std::endl;
		return;
	}
	hovername << std::pair<ui8,ui32>(3,os->ID) << " " << std::pair<ui8,ui32>(2,pom);
	cb->setHoverName(objid,&hovername);

	//int heroID = cb->getSelectedHero();
	//if (heroID>=0)
	//{
		//add += ( (visitors[os].find(heroID) == visitors[os].end()) 
		//		? 
		//	(VLC->generaltexth->allTexts[353])  //not visited
		//		: 
		//	( VLC->generaltexth->allTexts[352]) ); //visited
	//}
};

void CVisitableOPH::onHeroVisit(int objid, int heroID)
{
	DEFOS;
	if (visitors.find(objid)!=visitors.end())
	{
		if(visitors[objid].find(heroID)==visitors[objid].end())
		{
			onNAHeroVisit(objid,heroID, false);
			if(os->ID != 102)
				visitors[objid].insert(heroID);
		}
		else
		{
			onNAHeroVisit(objid,heroID, true);
		}
	}
	else
	{
#ifndef __GNUC__
		throw new std::exception("Skrypt nie zainicjalizowal instancji tego obiektu. :(");
#else
		throw new std::exception();
#endif
	}
};
void CVisitableOPH::onNAHeroVisit(int objid, int heroID, bool alreadyVisited)
{
	const CGObjectInstance *os = cb->getObj(objid);
	int id=0, subid=0, ot=0, val=1;
	switch(os->ID)
	{
	case 51:
		subid=0;
		ot=80;
		break;
	case 23:
		subid=1;
		ot=39;
		break;
	case 61:
		subid=2;
		ot=100;
		break;
	case 32:
		subid=3;
		ot=59;
		break;
	case 100:
		id=5;
		ot=143;
		val=1000;
		break;
	case 102:
		id = 5;
		subid = 1;
		ot = 146;
		val = 1;
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
				cb->changePrimSkill(heroID,subid,val);
				InfoWindow iw;
				iw.components.push_back(Component(0,subid,val,0));
				iw.text << std::pair<ui8,ui32>(11,ot);
				iw.player = cb->getHeroOwner(heroID);
				cb->showInfoDialog(&iw);
				break;
			}
		case 100: //give exp
			{
				InfoWindow iw;
				iw.components.push_back(Component(id,subid,val,0));
				iw.player = cb->getHeroOwner(heroID);
				iw.text << std::pair<ui8,ui32>(11,ot);
				cb->showInfoDialog(&iw);
				cb->changePrimSkill(heroID,4,val);
				break;
			}
		case 102:
			{
				const CGHeroInstance *h = cb->getHero(heroID);
				val = VLC->heroh->reqExp(h->level) + VLC->heroh->reqExp(h->level+val);
				if(!typeOfTree[objid])
				{
					visitors[objid].insert(heroID);
					InfoWindow iw;
					iw.components.push_back(Component(id,subid,1,0));
					iw.player = cb->getHeroOwner(heroID);
					iw.text << std::pair<ui8,ui32>(11,148);
					cb->showInfoDialog(&iw);
					cb->changePrimSkill(heroID,4,val);
					break;
				}
				else
				{
					int res, resval;
					if(typeOfTree[objid]==1)
					{
						res = 6;
						resval = 2000;
						ot = 149;
					}
					else
					{
						res = 5;
						resval = 10;
						ot = 151;
					}

					if(cb->getResource(h->tempOwner,res) < resval) //not enough resources
					{
						ot++;
						InfoWindow iw;
						iw.player = h->tempOwner;
						iw.text << std::pair<ui8,ui32>(11,ot);
						cb->showInfoDialog(&iw);
						return;
					}

					YesNoDialog sd;
					sd.player = cb->getHeroOwner(heroID);
					sd.text << std::pair<ui8,ui32>(11,ot);
					sd.components.push_back(Component(id,subid,val,0));
					cb->showYesNoDialog(&sd,CFunctionList<void(ui32)>(boost::bind(&CVisitableOPH::treeSelected,this,objid,heroID,res,resval,val,_1)));
				}
				break;
			}
		}
	}
	else
	{
		ot++;
		InfoWindow iw;
		iw.player = cb->getHeroOwner(heroID);
		iw.text << std::pair<ui8,ui32>(11,ot);
		cb->showInfoDialog(&iw);
	}
}

std::vector<int> CVisitableOPH::yourObjects()
{
	std::vector<int> ret;
	ret.push_back(51);//camp 
	ret.push_back(23);//tower
	ret.push_back(61);//axis
	ret.push_back(32);//garden
	ret.push_back(100);//stone
	ret.push_back(102);//tree
	return ret;
}

void CVisitableOPH::treeSelected( int objid, int heroID, int resType, int resVal, int expVal, ui32 result )
{
	if(result==0) //player agreed to give res for exp
	{
		cb->giveResource(cb->getHeroOwner(heroID),resType,-resVal); //take resource
		cb->changePrimSkill(heroID,4,expVal); //give exp
		visitors[objid].insert(heroID); //set state to visited
	}
}
void CVisitableOPW::onNAHeroVisit(int objid, int heroID, bool alreadyVisited)
{
	DEFOS;
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

		InfoWindow iw;
		iw.player = cb->getHero(heroID)->tempOwner;
		iw.text << std::pair<ui8,ui32>(11,mid);
		cb->showInfoDialog(&iw);
	}
	else
	{
		int type, sub, val;
		type = 2;
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
		int player = cb->getHeroOwner(heroID);
		cb->giveResource(player,sub,val);
		InfoWindow iw;
		iw.player = player;
		iw.components.push_back(Component(type,sub,val,0));
		iw.text << std::pair<ui8,ui32>(11,mid);
		cb->showInfoDialog(&iw);
		visited[objid] = true;
		MetaString ms; //set text to "visited"
		ms << std::pair<ui8,ui32>(3,os->ID) << " " << std::pair<ui8,ui32>(1,352);
		cb->setHoverName(objid,&ms);
	}
}
void CVisitableOPW::newTurn ()
{
	if (cb->getDate(1)==1) //first day of week
	{
		for (std::map<int,bool>::iterator i = visited.begin(); i != visited.end(); i++)
		{
			(*i).second = false;
			MetaString ms; //set text to "not visited"
			ms << std::pair<ui8,ui32>(3,cb->getObj(i->first)->ID) << " " << std::pair<ui8,ui32>(1,353);
			cb->setHoverName(i->first,&ms);
		}

	}
} 
void CVisitableOPW::newObject(int objid)
{
	visited.insert(std::pair<int,bool>(objid,false));
	DEFOS;
	MetaString ms;
	ms << std::pair<ui8,ui32>(3,os->ID) << " " << std::pair<ui8,ui32>(1,visited[objid] ? 352 : 353);
	cb->setHoverName(objid,&ms);
}

void CVisitableOPW::onHeroVisit(int objid, int heroID)
{
	if(visited[objid])
		onNAHeroVisit(objid,heroID,true);
	else 
		onNAHeroVisit(objid,heroID,false);
}

std::vector<int> CVisitableOPW::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(3);
	ret.push_back(55); //mystical garden
	ret.push_back(112); //windmill
	ret.push_back(109); //water wheel
	return ret;
}

void CMines::newObject(int objid)
{
	ourObjs.push_back(objid);
	cb->setOwner(objid,NEUTRAL_PLAYER);
	DEFOS;
	MetaString ms;
	ms << std::pair<ui8,ui32>(3,os->ID);
	cb->setHoverName(objid,&ms);
}
void CMines::onHeroVisit(int objid, int heroID)
{
	//TODO: this is code for standard mines, no support for abandoned mine (subId==7)
	DEFOS;
	const CGHeroInstance *h = cb->getHero(heroID);
	if(h->tempOwner == os->tempOwner)
		return; //TODO: leaving garrison
	cb->setOwner(objid,h->tempOwner);
	MetaString ms;
	ms << std::pair<ui8,ui32>(9,os->subID) << " " << std::pair<ui8,ui32>(6,23+h->tempOwner);
	cb->setHoverName(objid,&ms);
	ms.clear();

	int vv=1; //amount of resource per turn	
	if (os->subID==0 || os->subID==2)
		vv++;
	else if (os->subID==6)
		vv = 1000;

	InfoWindow iw;
	iw.text << std::pair<ui8,ui32>(10,os->subID);
	iw.player = h->tempOwner;
	iw.components.push_back(Component(2,os->subID,vv,-1));
	cb->showInfoDialog(&iw);
}
std::vector<int> CMines::yourObjects()
{
	std::vector<int> ret;
	ret.push_back(53);
	return ret;
}
void CMines::newTurn ()
{
	const CGObjectInstance * obj;
	for (unsigned i=0;i<ourObjs.size();i++)
	{
		obj = cb->getObj(ourObjs[i]);
		if (obj->tempOwner == NEUTRAL_PLAYER)
			continue;
		int vv = 1;
		if (obj->subID==0 || obj->subID==2)
			vv++;
		else if (obj->subID==6)
			vv = 1000;
		cb->giveResource(obj->tempOwner,obj->subID,vv);
	}
}


void CPickable::newObject(int objid)
{
	cb->setBlockVis(objid,true);

	MetaString ms;
	DEFOS;
	switch (os->ID)
	{
	case 79:
		ms << std::pair<ui8,ui32>(4,os->subID);
		break;
	case 5:
		ms << std::pair<ui8,ui32>(5,os->subID);
		break;
	default:
		ms << std::pair<ui8,ui32>(3,os->ID);
		break;
	}

	cb->setHoverName(objid,&ms);
}
void CPickable::onHeroVisit(int objid, int heroID)
{
	DEFOS;
	switch(os->ID)
	{
	case 5: //artifact
		{
			cb->giveHeroArtifact(os->subID,heroID,-1); //TODO: na pozycje
			InfoWindow iw;
			iw.player = cb->getHeroOwner(heroID);
			iw.components.push_back(Component(4,os->subID,0,0));
			iw.text << std::pair<ui8,ui32>(12,os->subID);
			cb->showInfoDialog(&iw);
			break;
		}
	case 12: //campfire
		{
			int val = (rand()%3) + 4, //4 - 6
				res = rand()%6, 
				owner = cb->getHeroOwner(heroID);
			cb->giveResource(owner,res,val); //non-gold resource
			cb->giveResource(owner,6,val*100);//gold
			InfoWindow iw;
			iw.player = owner;
			iw.components.push_back(Component(2,6,val*100,0));
			iw.components.push_back(Component(2,res,val,0));
			iw.text << std::pair<ui8,ui32>(11,23);
			cb->showInfoDialog(&iw);
			break;
		}
	case 79: //resource
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
			{
				InfoWindow iw;
				iw.player = cb->getHero(heroID)->tempOwner;
				iw.text << t2->message;
				cb->showInfoDialog(&iw);
			}

			cb->giveResource(cb->getHeroOwner(heroID),os->subID,val);

			ShowInInfobox sii;
			sii.player = cb->getHeroOwner(heroID);
			sii.c = Component(2,os->subID,val,0);
			sii.text << std::pair<ui8,ui32>(11,113);
			sii.text.replacements.push_back(VLC->objh->restypes[os->subID]);
			cb->showCompInfo(&sii);
			break;
		}
	case 101: //treasure chest
		{
			if (os->subID) //not OH3 treasure chest
				break; 
			int wyn = rand()%100, val=0;
			if (wyn<32) //1k/0.5k
			{
				val = 1000;
			}
			else if(wyn<64) //1.5k/1k
			{
				val = 1500;
			}
			else if(wyn<95) //2k/1.5k
			{
				val = 2000;
			}
			else //random treasure artifact, or (if backapack is full) 1k/0.5k
			{
				if (1/*TODO: backpack is full*/)
				{
					val = 1000;
				}
				else
				{
					//TODO: give treasure artifact
					break;
				}
			}
			SelectionDialog sd;
			sd.player = cb->getHeroOwner(heroID);
			sd.text << std::pair<ui8,ui32>(11,146);
			sd.components.push_back(Component(2,6,val,0));
			sd.components.push_back(Component(5,0,val-500,0));
			boost::function<void(ui32)> fun = boost::bind(&CPickable::chosen,this,_1,heroID,val);
			cb->showSelectionDialog(&sd,fun);
			break;
		}
	}
	cb->removeObject(objid);
}
void CPickable::chosen(ui32 which, int heroid, int val)
{
	switch(which)
	{
	case 0: //player pick gold
		cb->giveResource(cb->getHeroOwner(heroid),6,val);
		break;
	case 1: //player pick exp
		cb->changePrimSkill(heroid, 4, val-500);
		break;
	default:
		throw std::string("Unhandled choice");
	}
}

std::vector<int> CPickable::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret;
	ret.push_back(79); //resource
	ret.push_back(5); //artifact
	ret.push_back(12); //resource
	ret.push_back(101); //treasure chest / commander stone
	return ret;
}

void CTownScript::onHeroVisit(int objid, int heroID)
{
	cb->heroVisitCastle(objid,heroID);
}

void CTownScript::newObject(int objid)
{
	MetaString ms;
	const CGTownInstance * n = cb->getTown(objid);
	ms << n->name << ", " << n->town->name;
	cb->setHoverName(objid,&ms);
}

void CTownScript::onHeroLeave(int objid, int heroID)
{
	cb->stopHeroVisitCastle(objid,heroID);
}

std::vector<int> CTownScript::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(98); //town
	return ret;
}

void CHeroScript::newObject(int objid)
{
	cb->setBlockVis(objid,true);
	MetaString ms;
	ms << std::pair<ui8,ui32>(1,15);
	ms.replacements.push_back(cb->getHero(objid)->name);
	ms.replacements.push_back(cb->getHero(objid)->type->heroClass->name);
	cb->setHoverName(objid,&ms);
}

void CHeroScript::onHeroVisit(int objid, int heroID)
{
	//TODO: check for allies
	const CGHeroInstance *my = cb->getHero(objid), 
		*vis = cb->getHero(objid);
	if(my->tempOwner == vis->tempOwner) //one of allied cases
	{
		//exchange
	}
	else
	{
		cb->startBattle(
			&my->army,
			&vis->army,
			my->pos,
			my,
			vis);
	}
}
std::vector<int> CHeroScript::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret;
	ret.push_back(34); //hero
	return ret;
}
void CMonsterS::newObject(int objid)
{
	//os->blockVisit = true;
	DEFOS;
	switch(VLC->creh->creatures[os->subID].level)
	{
	case 1:
		amounts[objid] = rand()%31+20;
		break;
	case 2:
		amounts[objid] = rand()%16+15;
		break;
	case 3:
		amounts[objid] = rand()%16+10;
		break;
	case 4:
		amounts[objid] = rand()%11+10;
		break;
	case 5:
		amounts[objid] = rand()%9+8;
		break;
	case 6:
		amounts[objid] = rand()%8+5;
		break;
	case 7:
		amounts[objid] = rand()%7+3;
		break;
	case 8:
		amounts[objid] = rand()%4+2;
		break;
	case 9:
		amounts[objid] = rand()%3+2;
		break;
	case 10:
		amounts[objid] = rand()%3+1;
		break;

	}

	MetaString ms;
	int pom = CCreature::getQuantityID(amounts[objid]);
	pom = 174 + 3*pom + 1;
	ms << std::pair<ui8,ui32>(6,pom) << " " << std::pair<ui8,ui32>(7,os->subID);
	cb->setHoverName(objid,&ms);
}
void CMonsterS::onHeroVisit(int objid, int heroID)
{
	DEFOS;
	CCreatureSet set;
	//TODO: zrobic secik w sposob wyrafinowany
	set.slots[0] = std::pair<ui32,si32>(os->subID,amounts[objid]);
	cb->startBattle(heroID,set,os->pos);
}
std::vector<int> CMonsterS::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret;
	ret.push_back(54); //monster
	return ret;
}


void CCreatureGen::newObject(int objid)
{
	DEFOS;
	amount[objid] = VLC->creh->creatures[VLC->objh->cregens[os->subID]].growth;
	MetaString ms;
	ms << std::pair<ui8,ui32>(8,os->subID);
	cb->setHoverName(objid,&ms);
}
void CCreatureGen::onHeroVisit(int objid, int heroID)
{
}
std::vector<int> CCreatureGen::yourObjects() //returns IDs of objects which are handled by script
{
	std::vector<int> ret(1);
	ret.push_back(17); //cregen1
	return ret;
}
