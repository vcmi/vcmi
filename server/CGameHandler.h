#ifndef CGAMEHANDLER_H
#define CGAMEHANDLER_H

#include "../global.h"
#include <set>
#include "../client/FunctionList.h"
#include "../CGameState.h"
#include "../lib/Connection.h"
#include "../lib/IGameCallback.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>
class CVCMIServer;
class CGameState;
struct StartInfo;
class CCPPObjectScript;
class CScriptCallback;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
template <typename T> struct CPack;
template <typename T> struct Query;
class CGHeroInstance;
extern std::map<ui32, CFunctionList<void(ui32)> > callbacks; //question id => callback functions - for selection dialogs
extern boost::mutex gsm;

struct PlayerStatus
{
	bool makingTurn, engagedIntoBattle;
	std::set<ui32> queries;

	PlayerStatus():makingTurn(false),engagedIntoBattle(false){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & makingTurn & engagedIntoBattle & queries;
	}
};
class PlayerStatuses
{
public:
	std::map<ui8,PlayerStatus> players;
	boost::mutex mx;
	boost::condition_variable cv; //notifies when any changes are made

	void addPlayer(ui8 player);
	PlayerStatus operator[](ui8 player);
	bool hasQueries(ui8 player);
	bool checkFlag(ui8 player, bool PlayerStatus::*flag);
	void setFlag(ui8 player, bool PlayerStatus::*flag, bool val);
	void addQuery(ui8 player, ui32 id);
	void removeQuery(ui8 player, ui32 id);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & players;
	}
};

class CGameHandler : public IGameCallback
{
	static ui32 QID;
	CGameState *gs;
	//std::set<CCPPObjectScript *> cppscripts; //C++ scripts
	//std::map<int, std::map<std::string, CObjectScript*> > objscr; //non-C++ scripts 

	CVCMIServer *s;
	std::map<int,CConnection*> connections; //player color -> connection to clinet with interface of that player
	PlayerStatuses states; //player color -> player state
	std::set<CConnection*> conns;

	void changeSecSkill(int ID, ui16 which, int val, bool abs=false);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);
	void moveStack(int stack, int dest);
	void startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb); //use hero=NULL for no hero
	void prepareAttack(BattleAttack &bat, CStack *att, CStack *def); //if last parameter is true, attack is by shooting, if false it's a melee attack
	void prepareAttacked(BattleStackAttacked &bsa, CStack *def);
	void checkForBattleEnd( std::vector<CStack*> &stacks );
	void setupBattle( BattleInfo * curB, int3 tile, CCreatureSet &army1, CCreatureSet &army2, CGHeroInstance * hero1, CGHeroInstance * hero2 );

public:
	CGameHandler(void);
	~CGameHandler(void);

	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	//get info
	int getOwner(int heroID);
	int getResource(int player, int which);
	int getSelectedHero();
	int getDate(int mode=0);
	const CGObjectInstance* getObj(int objid);
	const CGHeroInstance* getHero(int objid);
	const CGTownInstance* getTown(int objid);
	const CGHeroInstance* getSelectedHero(int player); //NULL if no hero is selected
	int getCurrentPlayer();

	//do sth
	void changeSpells(int hid, bool give, const std::set<ui32> &spells);
	void removeObject(int objid);
	void setBlockVis(int objid, bool bv);
	void setOwner(int objid, ui8 owner);
	void setHoverName(int objid, MetaString * name);
	void changePrimSkill(int ID, int which, int val, bool abs=false);
	void showInfoDialog(InfoWindow *iw);
	void showYesNoDialog(YesNoDialog *iw, const CFunctionList<void(ui32)> &callback);
	void showSelectionDialog(SelectionDialog *iw, const CFunctionList<void(ui32)> &callback); //returns question id
	void giveResource(int player, int which, int val);
	void showCompInfo(ShowInInfobox * comp);
	void heroVisitCastle(int obj, int heroID);
	void stopHeroVisitCastle(int obj, int heroID);
	void giveHeroArtifact(int artid, int hid, int position); //pos==-1 - first free slot in backpack; pos==-2 - default if available or backpack
	void startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb); //use hero=NULL for no hero
	void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb); //for hero<=>neutral army
	void setAmount(int objid, ui32 val);
	void moveHero(int hid, int3 pos, bool instant);
	//////////////////////////////////////////////////////////////////////////

	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & QID & gs & states;
	}
	template <typename T> void applyAndAsk(Query<T> * sel, ui8 player, boost::function<void(ui32)> &callback)
	{
		gsm.lock();
		sel->id = QID;
		callbacks[QID] = callback;
		states.addQuery(player,QID);
		QID++; 
		sendAndApply(sel);
		gsm.unlock();
	}
	template <typename T> void ask(Query<T> * sel, ui8 player, const CFunctionList<void(ui32)> &callback)
	{
		gsm.lock();
		sel->id = QID;
		callbacks[QID] = callback;
		states.addQuery(player,QID);
		sendToAllClients(sel);
		QID++; 
		gsm.unlock();
	}

	template <typename T>void sendDataToClients(const T & data)
	{
		for(std::set<CConnection*>::iterator i=conns.begin(); i!=conns.end();i++)
		{
			(*i)->wmx->lock();
			**i << data;
			(*i)->wmx->unlock();
		}
	}
	template <typename T>void sendToAllClients(CPack<T> * info)
	{
		for(std::set<CConnection*>::iterator i=conns.begin(); i!=conns.end();i++)
		{
			(*i)->wmx->lock();
			**i << info->getType() << *info->This();
			(*i)->wmx->unlock();
		}
	}
	template <typename T>void sendAndApply(CPack<T> * info)
	{
		gs->apply(info);
		sendToAllClients(info);
	}
	void run();
	void newTurn();

	friend class CVCMIServer;
	friend class CScriptCallback;
};
#endif //CGAMEHANDLER_H