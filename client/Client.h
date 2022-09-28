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

#include <vcmi/Environment.h>

#include "../lib/IGameCallback.h"
#include "../lib/battle/BattleAction.h"
#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/CStopWatch.h"
#include "../lib/int3.h"
#include "../lib/CondSh.h"
#include "../lib/CPathfinder.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CPack;
struct CPackForServer;
class CCampaignState;
class IGameEventsReceiver;
class IBattleEventsReceiver;
class CBattleGameInterface;
class CGameState;
class CGameInterface;
class BattleAction;
struct CPathsInfo;
class BinaryDeserializer;
class BinarySerializer;

template<typename T> class CApplier;

#if SCRIPTING_ENABLED
namespace scripting
{
	class PoolImpl;
}
#endif

namespace events
{
	class EventBus;
}

VCMI_LIB_NAMESPACE_END

class CBattleCallback;
class CCallback;
class CClient;
class CBaseForCLApply;

namespace boost { class thread; }

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

class CPlayerEnvironment : public Environment
{
public:
	PlayerColor player;
	CClient * cl;
	std::shared_ptr<CCallback> mainCallback;

	CPlayerEnvironment(PlayerColor player_, CClient * cl_, std::shared_ptr<CCallback> mainCallback_);
	const Services * services() const override;
	vstd::CLoggerBase * logger() const override;
	events::EventBus * eventBus() const override;
	const BattleCb * battle() const override;
	const GameCb * game() const override;
};

/// Class which handles client - server logic
class CClient : public IGameCallback, public CBattleInfoCallback, public Environment
{
public:
	std::map<PlayerColor, std::shared_ptr<CGameInterface>> playerint;
	std::map<PlayerColor, std::shared_ptr<CBattleGameInterface>> battleints;

	std::map<PlayerColor, std::vector<std::shared_ptr<IBattleEventsReceiver>>> additionalBattleInts;

	boost::optional<BattleAction> curbaction;

	CClient();
	~CClient();

	const Services * services() const override;
	const BattleCb * battle() const override;
	const GameCb * game() const override;
	vstd::CLoggerBase * logger() const override;
	events::EventBus * eventBus() const override;

	void newGame(CGameState * gameState);
	void loadGame();
	void serialize(BinarySerializer & h, const int version);
	void serialize(BinaryDeserializer & h, const int version);

	void save(const std::string & fname);
	void endGame();

	void initMapHandler();
	void initPlayerEnvironments();
	void initPlayerInterfaces();
	std::string aiNameForPlayer(const PlayerSettings & ps, bool battleAI); //empty means no AI -> human
	std::string aiNameForPlayer(bool battleAI);
	void installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, PlayerColor color, bool battlecb = false);
	void installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, PlayerColor color, bool needCallback = true);

	static ThreadSafeVector<int> waitingRequest; //FIXME: make this normal field (need to join all threads before client destruction)

	void handlePack(CPack * pack); //applies the given pack and deletes it
	int sendRequest(const CPackForServer * request, PlayerColor player); //returns ID given to that request

	void battleStarted(const BattleInfo * info);
	void commenceTacticPhaseForInt(std::shared_ptr<CBattleGameInterface> battleInt); //will be called as separate thread
	void battleFinished();
	void startPlayerBattleAction(PlayerColor color);
	void stopPlayerBattleAction(PlayerColor color);
	void stopAllBattleActions();

	void invalidatePaths();
	std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h);
	virtual PlayerColor getLocalPlayer() const override;

	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions

	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> & spells) override {};
	bool removeObject(const CGObjectInstance * obj) override {return false;};
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
	bool swapGarrisonOnSiege(ObjectInstanceID tid) override {return false;};
	void giveHeroNewArtifact(const CGHeroInstance * h, const CArtifact * artType, ArtifactPosition pos) override {};
	void giveHeroArtifact(const CGHeroInstance * h, const CArtifactInstance * a, ArtifactPosition pos) override {};
	void putArtifact(const ArtifactLocation & al, const CArtifactInstance * a) override {};
	void removeArtifact(const ArtifactLocation & al) override {};
	bool moveArtifact(const ArtifactLocation & al1, const ArtifactLocation & al2) override {return false;};

	void showCompInfo(ShowInInfobox * comp) override {};
	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void startBattlePrimary(const CArmedInstance * army1, const CArmedInstance * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool creatureBank = false, const CGTownInstance * town = nullptr) override {}; //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance * army1, const CArmedInstance * army2, int3 tile, bool creatureBank = false) override {}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance * army1, const CArmedInstance * army2, bool creatureBank = false) override {}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override {return false;};
	void giveHeroBonus(GiveBonus * bonus) override {};
	void setMovePoints(SetMovePoints * smp) override {};
	void setManaPoints(ObjectInstanceID hid, int val) override {};
	void giveHero(ObjectInstanceID id, PlayerColor player) override {};
	void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags) override {};
	void sendAndApply(CPackForClient * pack) override {};
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override {};

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) override {}
	void changeFogOfWar(std::unordered_set<int3, ShashInt3> & tiles, PlayerColor player, bool hide) override {}

	void setObjProperty(ObjectInstanceID objid, int prop, si64 val) override {}

	void showInfoDialog(InfoWindow * iw) override {};
	void showInfoDialog(const std::string & msg, PlayerColor player) override {};

#if SCRIPTING_ENABLED
	scripting::Pool * getGlobalContextPool() const override;
	scripting::Pool * getContextPool() const override;
#endif

private:
	std::map<PlayerColor, std::shared_ptr<CBattleCallback>> battleCallbacks; //callbacks given to player interfaces
	std::map<PlayerColor, std::shared_ptr<CPlayerEnvironment>> playerEnvironments;

#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolImpl> clientScripts;
#endif
	std::unique_ptr<events::EventBus> clientEventBus;

	std::shared_ptr<CApplier<CBaseForCLApply>> applier;

	mutable boost::mutex pathCacheMutex;
	std::map<const CGHeroInstance *, std::shared_ptr<CPathsInfo>> pathCache;

	std::map<PlayerColor, std::shared_ptr<boost::thread>> playerActionThreads;

	void waitForMoveAndSend(PlayerColor color);
	void reinitScripting();
};
