#pragma once


#include "../lib/IGameCallback.h"
#include "../lib/CondSh.h"
#include "../lib/CStopWatch.h"

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
	CStopWatch th;
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

template<typename T>
class ThreadSafeVector
{
	typedef std::vector<T> TVector;
	typedef boost::unique_lock<boost::mutex> TLock;
	TVector items;
	boost::mutex mx;
	boost::condition_variable cond;

public:

	void pushBack(const T &item)
	{
		TLock lock(mx);
		items.push_back(item);
		cond.notify_all();
	}

// 	//to access list, caller must present a lock used to lock mx
// 	TVector &getList(TLock &lockedLock)
// 	{
// 		assert(lockedLock.owns_lock() && lockedLock.mutex() == &mx);
// 		return items;
// 	}

	TLock getLock()
	{
		return TLock(mx);
	}

	void waitWhileContains(const T &item)
	{
		auto lock = getLock();
		while(vstd::contains(items, item))
			cond.wait(lock);
	}

	bool tryRemovingElement(const T&item) //returns false if element was not present
	{
		auto lock = getLock();
		auto itr = vstd::find(items, item);
		if(itr == items.end()) //not in container
		{
			return false;
		}

		items.erase(itr);
		cond.notify_all();
		return true;
	}
};

/// Class which handles client - server logic
class CClient : public IGameCallback
{
public:
	CCallback *cb;
	std::map<PlayerColor,shared_ptr<CCallback> > callbacks; //callbacks given to player interfaces
	std::map<PlayerColor,shared_ptr<CBattleCallback> > battleCallbacks; //callbacks given to player interfaces
	std::vector<IGameEventsReceiver*> privilagedGameEventReceivers; //scripting modules, spectator interfaces
	std::vector<IBattleEventsReceiver*> privilagedBattleEventReceivers; //scripting modules, spectator interfaces
	std::map<PlayerColor,CGameInterface *> playerint;
	std::map<PlayerColor,CBattleGameInterface *> battleints;
	bool hotSeat;
	CConnection *serv;

	boost::optional<BattleAction> curbaction;

	unique_ptr<CPathsInfo> pathInfo;
	boost::mutex pathMx; //protects the variable above

	CScriptingModule *erm;

	ThreadSafeVector<int> waitingRequest;

	void waitForMoveAndSend(PlayerColor color);
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
	void campaignMapFinished( shared_ptr<CCampaignState> camp );
	void finishCampaign( shared_ptr<CCampaignState> camp );
	void proposeNextMission(shared_ptr<CCampaignState> camp);
	void invalidatePaths(const CGHeroInstance *h = NULL); //invalidates paths for hero h or for any hero if h is NULL => they'll got recalculated when the next query comes
	void calculatePaths(const CGHeroInstance *h);
	void updatePaths(); //calls calculatePaths for same hero for which we previously calculated paths

	bool terminate;	// tell to terminate
	boost::thread *connectionHandler; //thread running run() method

	//////////////////////////////////////////////////////////////////////////
	virtual PlayerColor getLocalPlayer() const OVERRIDE;

	//////////////////////////////////////////////////////////////////////////
	//not working yet, will be implement somewhen later with support for local-sim-based gameplay
	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) OVERRIDE {};
	bool removeObject(const CGObjectInstance * obj) OVERRIDE {return false;};
	void setBlockVis(ObjectInstanceID objid, bool bv) OVERRIDE {};
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) OVERRIDE {};
	void setHoverName(const CGObjectInstance * obj, MetaString * name) OVERRIDE {};
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs=false) OVERRIDE {};
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false) OVERRIDE {}; 

	void showBlockingDialog(BlockingDialog *iw) OVERRIDE {}; 
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits, const boost::function<void()> &cb) OVERRIDE {};
	void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) OVERRIDE {};
	void giveResource(PlayerColor player, Res::ERes which, int val) OVERRIDE {};
	virtual void giveResources(PlayerColor player, TResources resources) OVERRIDE {};

	void giveCreatures(const CArmedInstance * objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) OVERRIDE {};
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures) OVERRIDE {};
	bool changeStackType(const StackLocation &sl, CCreature *c) OVERRIDE {return false;};
	bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) OVERRIDE {return false;};
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count) OVERRIDE {return false;};
	bool eraseStack(const StackLocation &sl, bool forceRemoval = false){return false;};
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) OVERRIDE {return false;}
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) OVERRIDE {return false;}
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) OVERRIDE {}
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count = -1) OVERRIDE {return false;}

	void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, ArtifactPosition pos) OVERRIDE {};
	void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, ArtifactPosition pos) OVERRIDE {};
	void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) OVERRIDE {}; 
	void removeArtifact(const ArtifactLocation &al) OVERRIDE {};
	bool moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) OVERRIDE {return false;};
	void synchronizeArtifactHandlerLists() OVERRIDE {};

	void showCompInfo(ShowInInfobox * comp) OVERRIDE {};
	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) OVERRIDE {};
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) OVERRIDE {};
	//void giveHeroArtifact(int artid, int hid, int position){}; 
	//void giveNewArtifact(int hid, int position){};
	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = NULL) OVERRIDE {}; //use hero=NULL for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false) OVERRIDE {}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false) OVERRIDE {}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	void setAmount(ObjectInstanceID objid, ui32 val) OVERRIDE {};
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, PlayerColor asker = PlayerColor::NEUTRAL) OVERRIDE {return false;};
	void giveHeroBonus(GiveBonus * bonus) OVERRIDE {};
	void setMovePoints(SetMovePoints * smp) OVERRIDE {};
	void setManaPoints(ObjectInstanceID hid, int val) OVERRIDE {};
	void giveHero(ObjectInstanceID id, PlayerColor player) OVERRIDE {};
	void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags) OVERRIDE {};
	void sendAndApply(CPackForClient * info) OVERRIDE {};
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) OVERRIDE {};

	//////////////////////////////////////////////////////////////////////////
	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions
	friend void processCommand(const std::string &message, CClient *&client); //handling console
	
	int sendRequest(const CPack *request, PlayerColor player); //returns ID given to that request

	void handlePack( CPack * pack ); //applies the given pack and deletes it
	void battleStarted(const BattleInfo * info);
	void commenceTacticPhaseForInt(CBattleGameInterface *battleInt); //will be called as separate thread

	void commitPackage(CPackForClient *pack) OVERRIDE;

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serialize(Handler &h, const int version);
	void battleFinished();
};
