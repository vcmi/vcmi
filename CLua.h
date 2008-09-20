#pragma once
#include "global.h"
#include "client/FunctionList.h"

#ifndef _MSC_VER
extern "C" {
#endif

//#include "lstate.h"

#ifndef _MSC_VER
}
#endif

#include <set>
#include <map>
class CLua;
struct SDL_Surface;
class CGObjectInstance;
class CGameInfo;
class CGHeroInstance;
class CScriptCallback;
class SComponent;
class CSelectableComponent;
class CGameState;
struct Mapa;
struct lua_State;
struct BattleResult;
enum ESLan{UNDEF=-1,CPP,ERM,LUA};
class CObjectScript
{
public:
	int owner, language;
	std::string filename;

	int getOwner(){return owner;} //255 - neutral /  254 - not flaggable
	CObjectScript();
	virtual ~CObjectScript();


	//functions to be called in script
	//virtual void init(){};
	virtual void newObject(int objid){};
	virtual void onHeroVisit(int objid, int heroID){};
	virtual void onHeroLeave(int objid, int heroID){};
	virtual std::string hoverText(int objid){return "";};
	virtual void newTurn (){};


	//TODO: implement functions below:
	virtual void equipArtefact(int HID, int AID, int slot, bool putOn){}; //putOn==0 means that artifact is taken off
	virtual void battleStart(int phase){}; //phase==0 - very start, before initialization of battle; phase==1 - just before battle starts
	virtual void battleNewTurn (int turn){}; //turn==-1 is for tactic stage
	//virtual void battleAction (int type,int destination, int stack, int owner, int){};
	//virtual void mouseClick (down,left,screen?, pos??){};
	virtual void heroLevelUp (int HID){}; //add possibility of changing available sec. skills

};
class CScript
{
public:
	CScript();
	virtual ~CScript();
};
class CLua :public CScript
{
protected:
	lua_State * is; /// tez niebezpieczne!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! (ale chwilowo okielznane)
	bool opened;
public:
	CLua(std::string initpath);
	void open(std::string initpath);
	void registerCLuaCallback();
	CLua();
	virtual ~CLua();

	void findF(std::string fname);
	void findF2(std::string fname);
	void findFS(std::string fname);


	friend class CGameState;
};

class CLuaObjectScript : public CLua, public CObjectScript
{
public:
	CLuaObjectScript(std::string filename);
	virtual ~CLuaObjectScript();
	static std::string genFN(std::string base, int ID);

	void init();
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
};
class CCPPObjectScript: public CObjectScript
{
public:
	CScriptCallback * cb;
	CCPPObjectScript(CScriptCallback * CB){cb=CB;};
	virtual std::vector<int> yourObjects()=0; //returns IDs of objects which are handled by script
};
class CVisitableOPH : public CCPPObjectScript  //once per hero
{
public:
	CVisitableOPH(CScriptCallback * CB):CCPPObjectScript(CB){};
	std::map<int, int> typeOfTree; //0 - level for free; 1 - 2000 gold; 2 - 10 gems
	std::map<int,std::set<int> > visitors;

	void onNAHeroVisit(int objid, int heroID, bool alreadyVisited);
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	void treeSelected(int objid, int heroID, int resType, int resVal, int expVal, ui32 result);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
};

class CVisitableOPW : public CCPPObjectScript  //once per week
{
public:
	CVisitableOPW(CScriptCallback * CB):CCPPObjectScript(CB){};
	std::map<int,bool> visited;
	void onNAHeroVisit(int objid, int heroID, bool alreadyVisited);
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	void newTurn ();
};

class CMines : public CCPPObjectScript  //flaggable, and giving resource at each day
{
public:
	CMines(CScriptCallback * CB):CCPPObjectScript(CB){};

	std::vector<int> ourObjs;

	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	void newTurn ();
};

class CPickable : public CCPPObjectScript //pickable - resources, artifacts, etc
{
public:
	CPickable(CScriptCallback * CB):CCPPObjectScript(CB){};
	void chosen(ui32 which, int heroid, int val); //val - value of treasure in gold
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
};

class CTownScript : public CCPPObjectScript  //pickable - resources, artifacts, etc
{
public:
	CTownScript(CScriptCallback * CB):CCPPObjectScript(CB){};
	void onHeroVisit(int objid, int heroID);
	void onHeroLeave(int objid, int heroID);
	void newObject(int objid);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
};

class CHeroScript : public CCPPObjectScript
{
public:
	CHeroScript(CScriptCallback * CB):CCPPObjectScript(CB){};
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
};

class CMonsterS : public CCPPObjectScript
{
public:
	CMonsterS(CScriptCallback * CB):CCPPObjectScript(CB){};
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	void endBattleWith(const CGObjectInstance *monster, BattleResult *result);
};

class CCreatureGen : public CCPPObjectScript
{
public:
	std::map<int, int> amount; //amount of creatures in each dwelling
	CCreatureGen(CScriptCallback * CB):CCPPObjectScript(CB){};
	void newObject(int objid);
	void onHeroVisit(int objid, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
};
