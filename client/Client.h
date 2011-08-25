#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "../global.h"
#include <boost/thread.hpp>
#include "../lib/IGameCallback.h"
#include "../lib/CondSh.h"
#include <queue>

/*
 * Client.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class IGameEventsReceiver;
class IBattleEventsReceiver;
class CBattleGameInterface;
struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;
struct BattleAction;
struct SharedMem;
class CClient;
class CScriptingModule;
struct CPathsInfo;
namespace boost { class thread; }

void processCommand(const std::string &message, CClient *&client);

/// structure to handle running server and connecting to it
class CServerHandler
{
private:
	void callServer(); //calls server via system(), should be called as thread
public:
	timeHandler th;
	boost::thread *serverThread; //thread that called system to run server
	SharedMem *shared; //interprocess memory (for waiting for server)
	bool verbose; //whether to print log msgs
	std::string port; //port number in text form

	//functions setting up local server
	void startServer(); //creates a thread with callServer
	void waitForServer(); //waits till server is ready
	CConnection * connectToServer(); //connects to server

	//////////////////////////////////////////////////////////////////////////
	static CConnection * justConnectToServer(const std::string &host = "", const std::string &port = ""); //connects to given host without taking any other actions (like setting up server)

	CServerHandler(bool runServer = false);
	~CServerHandler();
};

/// Class which handles client - server logic
class CClient : public IGameCallback
{
public:
	CCallback *cb;
	std::set<CCallback*> callbacks; //callbacks given to player interfaces
	std::vector<IGameEventsReceiver*> privilagedGameEventReceivers; //scripting modules, spectator interfaces
	std::vector<IBattleEventsReceiver*> privilagedBattleEventReceivers; //scripting modules, spectator interfaces
	std::map<ui8,CGameInterface *> playerint;
	std::map<ui8,CBattleGameInterface *> battleints;
	bool hotSeat;
	CConnection *serv;
	BattleAction *curbaction;

	CPathsInfo *pathInfo;
	boost::mutex pathMx; //protects the variable above

	CScriptingModule *erm;

	CondSh<int> waitingRequest;

	std::queue<CPack *> packs;
	boost::mutex packsM;

	void waitForMoveAndSend(int color);
	//void sendRequest(const CPackForServer *request, bool waitForRealization);
	CClient(void);
	CClient(CConnection *con, StartInfo *si);
	~CClient(void);

	void init();
	void newGame(CConnection *con, StartInfo *si); //con - connection to server

	void loadNeutralBattleAI();
	void endGame(bool closeConnection = true);
	void stopConnection();
	void save(const std::string & fname);
	void loadGame(const std::string & fname);
	void run();
	void finishCampaign( CCampaignState * camp );
	void proposeNextMission( CCampaignState * camp );
	void calculatePaths(const CGHeroInstance *h);
	void updatePaths(); //calls calculatePaths for same hero for which we previously calculated paths

	bool terminate;	// tell to terminate
	boost::thread *connectionHandler; //thread running run() method

	//////////////////////////////////////////////////////////////////////////
	virtual int getLocalPlayer() const OVERRIDE;

	//////////////////////////////////////////////////////////////////////////
	//not working yet, will be implement somewhen later with support for local-sim-based gameplay
	void changeSpells(int hid, bool give, const std::set<ui32> &spells) OVERRIDE {};
	bool removeObject(int objid) OVERRIDE {return false;};
	void setBlockVis(int objid, bool bv) OVERRIDE {};
	void setOwner(int objid, ui8 owner) OVERRIDE {};
	void setHoverName(int objid, MetaString * name) OVERRIDE {};
	void changePrimSkill(int ID, int which, si64 val, bool abs=false) OVERRIDE {};
	void changeSecSkill(int ID, int which, int val, bool abs=false) OVERRIDE {}; 
	void showBlockingDialog(BlockingDialog *iw, const CFunctionList<void(ui32)> &callback) OVERRIDE {};
	ui32 showBlockingDialog(BlockingDialog *iw) OVERRIDE {return 0;}; //synchronous version of above
	void showGarrisonDialog(int upobj, int hid, bool removableUnits, const boost::function<void()> &cb) OVERRIDE {};
	void showThievesGuildWindow(int requestingObjId) OVERRIDE {};
	void giveResource(int player, int which, int val) OVERRIDE {};

	void giveCreatures(const CArmedInstance * objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) OVERRIDE {};
	void takeCreatures(int objid, const std::vector<CStackBasicDescriptor> &creatures) OVERRIDE {};
	bool changeStackType(const StackLocation &sl, CCreature *c) OVERRIDE {return false;};
	bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) OVERRIDE {return false;};
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count) OVERRIDE {return false;};
	bool eraseStack(const StackLocation &sl, bool forceRemoval = false){return false;};
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) OVERRIDE {return false;}
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) OVERRIDE {return false;}
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) OVERRIDE {}
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count = -1) OVERRIDE {return false;}

	void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, int pos) OVERRIDE {};
	void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, int pos) OVERRIDE {};
	void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) OVERRIDE {}; 
	void removeArtifact(const ArtifactLocation &al) OVERRIDE {};
	void moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) OVERRIDE {};

	void showCompInfo(ShowInInfobox * comp) OVERRIDE {};
	void heroVisitCastle(int obj, int heroID) OVERRIDE {};
	void stopHeroVisitCastle(int obj, int heroID) OVERRIDE {};
	//void giveHeroArtifact(int artid, int hid, int position){}; 
	//void giveNewArtifact(int hid, int position){};
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, boost::function<void(BattleResult*)> cb = 0, const CGTownInstance *town = NULL) OVERRIDE {}; //use hero=NULL for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false) OVERRIDE {}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, boost::function<void(BattleResult*)> cb = 0, bool creatureBank = false) OVERRIDE {}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	void setAmount(int objid, ui32 val) OVERRIDE {};
	bool moveHero(si32 hid, int3 dst, ui8 instant, ui8 asker = 255) OVERRIDE {return false;};
	void giveHeroBonus(GiveBonus * bonus) OVERRIDE {};
	void setMovePoints(SetMovePoints * smp) OVERRIDE {};
	void setManaPoints(int hid, int val) OVERRIDE {};
	void giveHero(int id, int player) OVERRIDE {};
	void changeObjPos(int objid, int3 newPos, ui8 flags) OVERRIDE {};
	void sendAndApply(CPackForClient * info) OVERRIDE {};
	void heroExchange(si32 hero1, si32 hero2) OVERRIDE {};

	//////////////////////////////////////////////////////////////////////////
	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions
	friend void processCommand(const std::string &message, CClient *&client); //handling console
	
	void handlePack( CPack * pack ); //applies the given pack and deletes it
	void battleStarted(const BattleInfo * info);
	void commenceTacticPhaseForInt(CBattleGameInterface *battleInt); //will be called as separate thread

	void commitPackage(CPackForClient *pack) OVERRIDE;

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serialize(Handler &h, const int version);
};

#endif // __CLIENT_H__
