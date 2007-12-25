#include "global.h"
#include "lstate.h"
#include <set>
#include <map>
class CLua;
struct SDL_Surface;
class CGObjectInstance;
class CGameInfo;
class CGHeroInstance;
class CScriptCallback;
class SComponent;
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
	virtual void newObject(CGObjectInstance *os){};
	virtual void onHeroVisit(CGObjectInstance *os, int heroID){};
	virtual std::string hoverText(CGObjectInstance *os){return "";};
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
class IChosen
{
public:
	virtual void chosen(int which)=0;
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

	
	friend void initGameState(CGameInfo * cgi);
};

class CLuaObjectScript : public CLua, public CObjectScript
{
public:
	CLuaObjectScript(std::string filename);
	virtual ~CLuaObjectScript();
	static std::string genFN(std::string base, int ID);

	void init();
	void newObject(CGObjectInstance *os);
	void onHeroVisit(CGObjectInstance *os, int heroID);
	std::string hoverText(CGObjectInstance *os);

	friend void initGameState(CGameInfo * cgi);
};
class CCPPObjectScript: public CObjectScript
{
protected:
	CScriptCallback * cb;
	CCPPObjectScript(CScriptCallback * CB){cb=CB;};
public:
	virtual std::vector<int> yourObjects()=0; //returns IDs of objects which are handled by script
	virtual std::string hoverText(CGObjectInstance *os);
};
class CVisitableOPH : public CCPPObjectScript  //once per hero
{
	CVisitableOPH(CScriptCallback * CB):CCPPObjectScript(CB){};
	std::map<CGObjectInstance*,std::set<int> > visitors;
	
	void onNAHeroVisit(CGObjectInstance *os, int heroID, bool alreadyVisited);
	void newObject(CGObjectInstance *os);
	void onHeroVisit(CGObjectInstance *os, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	std::string hoverText(CGObjectInstance *os);

	friend void initGameState(CGameInfo * cgi);
};

class CVisitableOPW : public CCPPObjectScript  //once per week
{
	CVisitableOPW(CScriptCallback * CB):CCPPObjectScript(CB){};
	std::map<CGObjectInstance*,bool> visited;
	void onNAHeroVisit(CGObjectInstance *os, int heroID, bool alreadyVisited);
	void newObject(CGObjectInstance *os);
	void onHeroVisit(CGObjectInstance *os, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	std::string hoverText(CGObjectInstance *os);
	void newTurn (); 

	friend void initGameState(CGameInfo * cgi);
};

class CMines : public CCPPObjectScript  //flaggable, and giving resource at each day
{
	CMines(CScriptCallback * CB):CCPPObjectScript(CB){};

	std::vector<CGObjectInstance*> ourObjs;

	void newObject(CGObjectInstance *os);
	void onHeroVisit(CGObjectInstance *os, int heroID);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script
	std::string hoverText(CGObjectInstance *os);
	void newTurn (); 

	friend void initGameState(CGameInfo * cgi);
};

class CPickable : public CCPPObjectScript, public IChosen  //pickable - resources, artifacts, etc
{
	std::vector<SComponent*> tempStore;
	int player;

	CPickable(CScriptCallback * CB):CCPPObjectScript(CB){};
	void chosen(int which);
	void newObject(CGObjectInstance *os);
	void onHeroVisit(CGObjectInstance *os, int heroID);
	std::string hoverText(CGObjectInstance *os);
	std::vector<int> yourObjects(); //returns IDs of objects which are handled by script

	friend void initGameState(CGameInfo * cgi);
};