#pragma once
#include "../global.h"
#include <vector>
#include <boost/function.hpp>
class CVCMIServer;
class CGameHandler;
class SComponent;
class CSelectableComponent;
class IChosen;
class CCreatureSet;
class CGHeroInstance;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class CGameState;
struct lua_State;
struct MetaString;
struct InfoWindow;
struct ShowInInfobox;
struct SelectionDialog;
class CScriptCallback
{
	CScriptCallback(void);
public:
	~CScriptCallback(void);
	CGameHandler *gh;

	//get info
	static int3 getPos(CGObjectInstance * ob);
	int getHeroOwner(int heroID);
	int getSelectedHero();
	int getDate(int mode=0);
	const CGObjectInstance* getObj(int objid);
	const CGHeroInstance* getHero(int objid);
	const CGTownInstance* getTown(int objid);

	//do sth
	void removeObject(int objid);
	void setBlockVis(int objid, bool bv);
	void setOwner(int objid, ui8 owner);
	void setHoverName(int objid, MetaString * name);
	void changePrimSkill(int ID, int which, int val, bool abs=false);
	void showInfoDialog(InfoWindow *iw);
	void showSelectionDialog(SelectionDialog *iw, boost::function<void(ui32),std::allocator<void> > &callback); //returns question id
	void giveResource(int player, int which, int val);
	void showCompInfo(ShowInInfobox * comp);
	void heroVisitCastle(int obj, int heroID);
	void stopHeroVisitCastle(int obj, int heroID);
	void giveHeroArtifact(int artid, int hid, int position); //pos==-1 - first free slot in backpack
	void startBattle(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2); //use hero=NULL for no hero
	void startBattle(int heroID, CCreatureSet army, int3 tile); //for hero<=>neutral army

	//friends
	friend class CGameHandler;
};
class CLuaCallback : public CScriptCallback
{
private:

	static void registerFuncs(lua_State * L);
	static int getPos(lua_State * L);//(CGObjectInstance * object);
	static int changePrimSkill(lua_State * L);//(int ID, int which, int val);
	static int getGnrlText(lua_State * L);//(int ID, int which, int val);
	static int getSelectedHero(lua_State * L);//()

	friend class CGameHandler;
};