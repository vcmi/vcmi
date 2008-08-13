#include "CScriptCallback.h"
#include "../lib/Connection.h"
#include "CVCMIServer.h"
#include "CGameHandler.h"
#include "../CGameState.h"
#include "../map.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>


CScriptCallback::CScriptCallback(void)
{
}

CScriptCallback::~CScriptCallback(void)
{
}
void CScriptCallback::setBlockVis(int objid, bool bv)
{
	SetObjectProperty sop(objid,2,bv);
	gh->sendAndApply(&sop);
}
void CScriptCallback::removeObject(int objid)
{
	RemoveObject ro;
	ro.id = objid;
	gh->sendAndApply(&ro);
}

void CScriptCallback::setOwner(int objid, ui8 owner)
{
	SetObjectProperty sop(objid,1,owner);
	gh->sendAndApply(&sop);
}
const CGObjectInstance* CScriptCallback::getObj(int objid)
{
	return gh->gs->map->objects[objid];
}
const CGHeroInstance* CScriptCallback::getHero(int objid)
{
	return static_cast<const CGHeroInstance*>(gh->gs->map->objects[objid]);
}
const CGTownInstance* CScriptCallback::getTown(int objid)
{
	return static_cast<const CGTownInstance*>(gh->gs->map->objects[objid]);
}
void CScriptCallback::setHoverName(int objid, MetaString* name)
{
	SetHoverName shn(objid, *name);
	gh->sendAndApply(&shn);
}
int3 CScriptCallback::getPos(CGObjectInstance * ob)
{
	return ob->pos;
}
void CScriptCallback::changePrimSkill(int ID, int which, int val, bool abs)
{
	gh->changePrimSkill(ID, which, val, abs);
}

int CScriptCallback::getHeroOwner(int heroID)
{
	return gh->gs->map->objects[heroID]->tempOwner;
}
void CScriptCallback::showInfoDialog(InfoWindow *iw)
{
	gh->sendToAllClients(iw);
}

void CScriptCallback::showSelectionDialog(SelectionDialog *iw, boost::function<void(ui32),std::allocator<void> > &callback)
{
	gh->ask(iw,iw->player,callback);
}

int CScriptCallback::getSelectedHero()
{	
	//int ret;
	//if (LOCPLINT->adventureInt->selection.type == HEROI_TYPE)
	//	ret = ((CGHeroInstance*)(LOCPLINT->adventureInt->selection.selected))->subID;
	//else 
	//	ret = -1;;
	return -1;
}
int CScriptCallback::getDate(int mode)
{
	return gh->gs->getDate(mode);
}
void CScriptCallback::giveResource(int player, int which, int val)
{
	SetResource sr;
	sr.player = player;
	sr.resid = which;
	sr.val = (gh->gs->players[player].resources[which]+val);
	gh->sendAndApply(&sr);
}
void CScriptCallback::showCompInfo(ShowInInfobox * comp)
{
	gh->sendToAllClients(comp);
}
void CScriptCallback::heroVisitCastle(int obj, int heroID)
{
	HeroVisitCastle vc;
	vc.hid = heroID;
	vc.tid = obj;
	vc.flags |= 1;
	gh->sendAndApply(&vc);
}

void CScriptCallback::stopHeroVisitCastle(int obj, int heroID)
{
	HeroVisitCastle vc;
	vc.hid = heroID;
	vc.tid = obj;
	gh->sendAndApply(&vc);
}
void CScriptCallback::giveHeroArtifact(int artid, int hid, int position) //pos==-1 - first free slot in backpack
{
	CGHeroInstance* h = gh->gs->map->getHero(hid,0);
	if(position<0)
	{
		for(unsigned i=0;i<h->artifacts.size();i++)
		{
			if(!h->artifacts[i])
			{
				h->artifacts[i] = artid;
				return;
			}
		}
		h->artifacts.push_back(artid);
		return;
	}
	else
	{
		if(h->artifWorn[position]) //slot is occupied
		{
			giveHeroArtifact(h->artifWorn[position],hid,-1);
		}
		h->artifWorn[position] = artid;
	}
}

void CScriptCallback::startBattle(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2) //use hero=NULL for no hero
{
	boost::thread(boost::bind(&CGameHandler::startBattle,gh,*(CCreatureSet *)army1,*(CCreatureSet *)army2,tile,(CGHeroInstance *)hero1,(CGHeroInstance *)hero2));
}
void CScriptCallback::startBattle(int heroID, CCreatureSet army, int3 tile) //for hero<=>neutral army
{
	CGHeroInstance* h = const_cast<CGHeroInstance*>(getHero(heroID));
	startBattle(&h->army,&army,tile,h,NULL);
	//gh->gs->battle(&h->army,army,tile,h,NULL);
}
void CLuaCallback::registerFuncs(lua_State * L)
{
//	lua_newtable(L);
//
//#define REGISTER_C_FUNC(x) \
//	lua_pushstring(L, #x);      \
//	lua_pushcfunction(L, x);    \
//	lua_rawset(L, -3)
//
//	REGISTER_C_FUNC(getPos);
//	REGISTER_C_FUNC(changePrimSkill);
//	REGISTER_C_FUNC(getGnrlText);
//	REGISTER_C_FUNC(getSelectedHero);
//
//	lua_setglobal(L, "vcmi");
//	#undef REGISTER_C_FUNC
}
int CLuaCallback::getPos(lua_State * L)//(CGObjectInstance * object);
{	
	//const int args = lua_gettop(L); // number of arguments
	//if ((args < 1) || !lua_isnumber(L, 1) )
	//	luaL_error(L,
	//		"Incorrect arguments to getPos([Object address])");
	//CGObjectInstance * object = (CGObjectInstance *)(lua_tointeger(L, 1));
	//lua_pushinteger(L,object->pos.x);
	//lua_pushinteger(L,object->pos.y);
	//lua_pushinteger(L,object->pos.z);
	return 3;
}
int CLuaCallback::changePrimSkill(lua_State * L)//(int ID, int which, int val);
{	
	//const int args = lua_gettop(L); // number of arguments
	//if ((args < 1) || !lua_isnumber(L, 1) ||
	//    ((args >= 2) && !lua_isnumber(L, 2)) ||
	//    ((args >= 3) && !lua_isnumber(L, 3))		)
	//{
	//	luaL_error(L,
	//		"Incorrect arguments to changePrimSkill([Hero ID], [Which Primary skill], [Change by])");
	//}
	//int ID = lua_tointeger(L, 1),
	//	which = lua_tointeger(L, 2),
	//	val = lua_tointeger(L, 3);

	//CScriptCallback::changePrimSkill(ID,which,val);

	return 0;
}
int CLuaCallback::getGnrlText(lua_State * L) //(int which),returns string
{
	//const int args = lua_gettop(L); // number of arguments
	//if ((args < 1) || !lua_isnumber(L, 1) )
	//	luaL_error(L,
	//		"Incorrect arguments to getGnrlText([Text ID])");
	//int which = lua_tointeger(L,1);
	//lua_pushstring(L,CGI->generaltexth->allTexts[which].c_str());
	return 1;
}
int CLuaCallback::getSelectedHero(lua_State * L) //(),returns int (ID of hero, -1 if no hero is seleceted)
{
	//int ret;
	//if (LOCPLINT->adventureInt->selection.type == HEROI_TYPE)
	//	ret = ((CGHeroInstance*)(LOCPLINT->adventureInt->selection.selected))->subID;
	//else 
	//	ret = -1;
	//lua_pushinteger(L,ret);
	return 1;
}
