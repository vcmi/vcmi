/*
 * Client.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/IGameCallback.h"
#include "../lib/battle/BattleAction.h"
#include "../lib/CStopWatch.h"
#include "../lib/int3.h"
#include "../lib/CondSh.h"

struct CPack;
struct CPackForServer;
class CCampaignState;
class CBattleCallback;
class IGameEventsReceiver;
class IBattleEventsReceiver;
class CBattleGameInterface;
class CGameState;
class CGameInterface;
class CCallback;
struct BattleAction;
class CClient;
class CScriptingModule;
struct CPathsInfo;
class BinaryDeserializer;
class BinarySerializer;
namespace boost { class thread; }

template<typename T> class CApplier;
class CBaseForCLApply;

template<typename T>
class ThreadSafeVector
{
	typedef std::vector<T> TVector;
	typedef boost::unique_lock<boost::mutex> TLock;
	TVector items;
	boost::mutex mx;
	boost::condition_variable cond;

public:
	void clear()
	{
		TLock lock(mx);
		items.clear();
		cond.notify_all();
	}

	void pushBack(const T & item)
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

	void waitWhileContains(const T & item)
	{
		auto lock = getLock();
		while(vstd::contains(items, item))
			cond.wait(lock);
	}

	bool tryRemovingElement(const T & item) //returns false if element was not present
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
	CApplier<CBaseForCLApply> * applier;
	std::unique_ptr<CPathsInfo> pathInfo;

	std::map<PlayerColor, std::shared_ptr<boost::thread>> playerActionThreads;
	void waitForMoveAndSend(PlayerColor color);

public:
	std::map<PlayerColor, std::shared_ptr<CCallback>> callbacks; //callbacks given to player interfaces
	std::map<PlayerColor, std::shared_ptr<CBattleCallback>> battleCallbacks; //callbacks given to player interfaces
	std::vector<std::shared_ptr<IGameEventsReceiver>> privilegedGameEventReceivers; //scripting modules, spectator interfaces
	std::vector<std::shared_ptr<IBattleEventsReceiver>> privilegedBattleEventReceivers; //scripting modules, spectator interfaces
	std::map<PlayerColor, std::shared_ptr<CGameInterface>> playerint;
	std::map<PlayerColor, std::shared_ptr<CBattleGameInterface>> battleints;

	std::map<PlayerColor, std::vector<std::shared_ptr<IGameEventsReceiver>>> additionalPlayerInts;
	std::map<PlayerColor, std::vector<std::shared_ptr<IBattleEventsReceiver>>> additionalBattleInts;

	boost::optional<BattleAction> curbaction;

	CScriptingModule * erm;
	CClient();
	~CClient();
	void init();

	void newGame();
	void loadGame();
	void serialize(BinarySerializer & h, const int version);
	void serialize(BinaryDeserializer & h, const int version);

	void save(const std::string & fname);
	void endGame(bool closeConnection = true);

	void initMapHandler();
	void initPlayerInterfaces();
	std::string aiNameForPlayer(const PlayerSettings & ps, bool battleAI); //empty means no AI -> human
	std::string aiNameForPlayer(bool battleAI);
	void installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, boost::optional<PlayerColor> color, bool battlecb = false);
	void installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, boost::optional<PlayerColor> color, bool needCallback = true);


	static ThreadSafeVector<int> waitingRequest; //FIXME: make this normal field (need to join all threads before client destruction)

	void handlePack(CPack * pack); //applies the given pack and deletes it
	void stopConnection();
	void commitPackage(CPackForClient * pack) override;
	int sendRequest(const CPackForServer * request, PlayerColor player); //returns ID given to that request

	void battleStarted(const BattleInfo * info);
	void commenceTacticPhaseForInt(std::shared_ptr<CBattleGameInterface> battleInt); //will be called as separate thread
	void battleFinished();
	void startPlayerBattleAction(PlayerColor color);
	void stopPlayerBattleAction(PlayerColor color);
	void stopAllBattleActions();

	void invalidatePaths();
	const CPathsInfo * getPathsInfo(const CGHeroInstance * h);
	virtual PlayerColor getLocalPlayer() const override;

	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions

	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> & spells) override {};
	bool removeObject(const CGObjectInstance * obj) override {return false;};
	void setBlockVis(ObjectInstanceID objid, bool bv) override {};
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) override {};
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs = false) override {};
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs = false) override {};

	void showBlockingDialog(BlockingDialog * iw) override {};
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override {};
	void showTeleportDialog(TeleportDialog * iw) override {};
	void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) override {};
	void giveResource(PlayerColor player, Res::ERes which, int val) override {};
	virtual void giveResources(PlayerColor player, TResources resources) override {};

	void giveCreatures(const CArmedInstance * objid, const CGHeroInstance * h, const CCreatureSet & creatures, bool remove) override {};
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> & creatures) override {};
	bool changeStackType(const StackLocation & sl, const CCreature * c) override {return false;};
	bool changeStackCount(const StackLocation & sl, TQuantity count, bool absoluteValue = false) override {return false;};
	bool insertNewStack(const StackLocation & sl, const CCreature * c, TQuantity count) override {return false;};
	bool eraseStack(const StackLocation & sl, bool forceRemoval = false) override {return false;};
	bool swapStacks(const StackLocation & sl1, const StackLocation & sl2) override {return false;}
	bool addToSlot(const StackLocation & sl, const CCreature * c, TQuantity count) override {return false;}
	void tryJoiningArmy(const CArmedInstance * src, const CArmedInstance * dst, bool removeObjWhenFinished, bool allowMerging) override {}
	bool moveStack(const StackLocation & src, const StackLocation & dst, TQuantity count = -1) override {return false;}

	void removeAfterVisit(const CGObjectInstance * object) override {};

	void giveHeroNewArtifact(const CGHeroInstance * h, const CArtifact * artType, ArtifactPosition pos) override {};
	void giveHeroArtifact(const CGHeroInstance * h, const CArtifactInstance * a, ArtifactPosition pos) override {};
	void putArtifact(const ArtifactLocation & al, const CArtifactInstance * a) override {};
	void removeArtifact(const ArtifactLocation & al) override {};
	bool moveArtifact(const ArtifactLocation & al1, const ArtifactLocation & al2) override {return false;};
	void synchronizeArtifactHandlerLists() override {};

	void showCompInfo(ShowInInfobox * comp) override {};
	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void startBattlePrimary(const CArmedInstance * army1, const CArmedInstance * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool creatureBank = false, const CGTownInstance * town = nullptr) override {}; //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance * army1, const CArmedInstance * army2, int3 tile, bool creatureBank = false) override {}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance * army1, const CArmedInstance * army2, bool creatureBank = false) override {}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	void setAmount(ObjectInstanceID objid, ui32 val) override {};
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override {return false;};
	void giveHeroBonus(GiveBonus * bonus) override {};
	void setMovePoints(SetMovePoints * smp) override {};
	void setManaPoints(ObjectInstanceID hid, int val) override {};
	void giveHero(ObjectInstanceID id, PlayerColor player) override {};
	void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags) override {};
	void sendAndApply(CPackForClient * info) override {};
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override {};

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) override {}
	void changeFogOfWar(std::unordered_set<int3, ShashInt3> & tiles, PlayerColor player, bool hide) override {}
};
