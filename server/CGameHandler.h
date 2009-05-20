#ifndef __CGAMEHANDLER_H__
#define __CGAMEHANDLER_H__

#include "../global.h"
#include <set>
#include "../client/FunctionList.h"
#include "../lib/CGameState.h"
#include "../lib/Connection.h"
#include "../lib/IGameCallback.h"
#include "../lib/BattleAction.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>

/*
 * CGameHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CVCMIServer;
class CGameState;
struct StartInfo;
class CCPPObjectScript;
class CScriptCallback;
struct BattleResult;
struct BattleAttack;
struct BattleStackAttacked;
struct CPack;
struct Query;
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
public:
	CVCMIServer *s;
	std::map<int,CConnection*> connections; //player color -> connection to client with interface of that player
	PlayerStatuses states; //player color -> player state
	std::set<CConnection*> conns;

	//queries stuff
	boost::recursive_mutex gsm;
	ui32 QID;
	std::map<ui32, CFunctionList<void(ui32)> > callbacks; //query id => callback function - for selection and yes/no dialogs
	std::map<ui32, boost::function<void()> > garrisonCallbacks; //query id => callback - for garrison dialogs
	std::map<ui32, std::pair<si32,si32> > allowedExchanges;

	bool isAllowedExchange(int id1, int id2);
	void giveSpells(const CGTownInstance *t, const CGHeroInstance *h);
	void moveStack(int stack, int dest);
	void startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb); //use hero=NULL for no hero
	void prepareAttack(BattleAttack &bat, CStack *att, CStack *def); //if last parameter is true, attack is by shooting, if false it's a melee attack
	void prepareAttacked(BattleStackAttacked &bsa, CStack *def);
	void checkForBattleEnd( std::vector<CStack*> &stacks );
	void setupBattle( BattleInfo * curB, int3 tile, CCreatureSet &army1, CCreatureSet &army2, CGHeroInstance * hero1, CGHeroInstance * hero2 );


	CGameHandler(void);
	~CGameHandler(void);

	//////////////////////////////////////////////////////////////////////////
	//from IGameCallback
	//get info
	int getCurrentPlayer();
	int getSelectedHero();


	//do sth
	void changeSpells(int hid, bool give, const std::set<ui32> &spells);
	bool removeObject(int objid);
	void setBlockVis(int objid, bool bv);
	void setOwner(int objid, ui8 owner);
	void setHoverName(int objid, MetaString * name);
	void setObjProperty(int objid, int prop, int val);
	void changePrimSkill(int ID, int which, int val, bool abs=false);
	void changeSecSkill(int ID, int which, int val, bool abs=false); 
	void showInfoDialog(InfoWindow *iw);
	void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback);
	ui32 showBlockingDialog(BlockingDialog *iw); //synchronous version of above
	void showGarrisonDialog(int upobj, int hid, const boost::function<void()> &cb);
	void giveResource(int player, int which, int val);
	void showCompInfo(ShowInInfobox * comp);
	void heroVisitCastle(int obj, int heroID);
	void stopHeroVisitCastle(int obj, int heroID);
	void giveHeroArtifact(int artid, int hid, int position); //pos==-1 - first free slot in backpack; pos==-2 - default if available or backpack
	void moveArtifact(int hid, int oldPosition, int destPos);
	void removeArtifact(int hid, int pos);
	void startBattleI(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, boost::function<void(BattleResult*)> cb); //use hero=NULL for no hero
	void startBattleI(int heroID, CCreatureSet army, int3 tile, boost::function<void(BattleResult*)> cb); //for hero<=>neutral army
	void setAmount(int objid, ui32 val);
	bool moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255);
	void giveHeroBonus(GiveBonus * bonus);
	void setMovePoints(SetMovePoints * smp);
	void setManaPoints(int hid, int val);
	void giveHero(int id, int player);
	void changeObjPos(int objid, int3 newPos, ui8 flags);
	//////////////////////////////////////////////////////////////////////////

	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	int getPlayerAt(CConnection *c) const;

	void playerMessage( ui8 player, const std::string &message);
	bool makeBattleAction(BattleAction &ba);
	bool makeCustomAction(BattleAction &ba);
	bool queryReply( ui32 qid, ui32 answer );
	bool hireHero( ui32 tid, ui8 hid );
	bool setFormation( si32 hid, ui8 formation );
	bool tradeResources( ui32 val, ui8 player, ui32 id1, ui32 id2 );
	bool buyArtifact( ui32 hid, si32 aid );
	bool swapArtifacts( si32 hid1, si32 hid2, ui16 slot1, ui16 slot2 );
	bool garrisonSwap(si32 tid);
	bool upgradeCreature( ui32 objid, ui8 pos, ui32 upgID );
	bool recruitCreatures(si32 objid, ui32 crid, ui32 cram);
	bool buildStructure(si32 tid, si32 bid);
	bool disbandCreature( si32 id, ui8 pos );
	bool arrangeStacks( si32 id1, si32 id2, ui8 what, ui8 p1, ui8 p2, si32 val );
	void save(const std::string &fname);
	void close();
	void handleTimeEvents();
	bool complain(const std::string &problem); //sends message to all clients, prints on the logs and return true

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & QID & states;
	}

	ui32 getQueryResult(ui8 player, int queryID);
	void sendMessageToAll(const std::string &message);
	void sendMessageTo(CConnection &c, const std::string &message);
	void applyAndAsk(Query * sel, ui8 player, boost::function<void(ui32)> &callback);
	void ask(Query * sel, ui8 player, const CFunctionList<void(ui32)> &callback);
	void sendToAllClients(CPackForClient * info);
	void sendAndApply(CPackForClient * info);

	void run(bool resume);
	void newTurn();

	friend class CVCMIServer;
	friend class CScriptCallback;
};

#endif // __CGAMEHANDLER_H__
