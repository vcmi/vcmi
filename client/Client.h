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

#include <memory>
#include <vcmi/Environment.h>

#include "../lib/IGameCallback.h"
#include "../lib/ConditionalWait.h"


VCMI_LIB_NAMESPACE_BEGIN

struct CPackForServer;
class IBattleEventsReceiver;
class CBattleGameInterface;
class CGameInterface;
class BattleAction;
class BattleInfo;
struct BankConfig;

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

template<typename T>
class ThreadSafeVector
{
	using TLock = std::unique_lock<std::mutex>;
	std::vector<T> items;
	std::mutex mx;
	std::condition_variable cond;
	std::atomic<bool> isTerminating = false;

public:
	void requestTermination()
	{
		isTerminating = true;
		clear();
	}

	void clear()
	{
		TLock lock(mx);
		items.clear();
		cond.notify_all();
	}

	void pushBack(const T & item)
	{
		assert(!isTerminating);

		TLock lock(mx);
		items.push_back(item);
		cond.notify_all();
	}

	void waitWhileContains(const T & item)
	{
		TLock lock(mx);
		while(vstd::contains(items, item))
			cond.wait(lock);

		if (isTerminating)
			throw TerminationRequestedException();
	}

	bool tryRemovingElement(const T & item) //returns false if element was not present
	{
		assert(!isTerminating);

		TLock lock(mx);
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
	const BattleCb * battle(const BattleID & battle) const override;
	const GameCb * game() const override;
};

/// Class which handles client - server logic
class CClient : public IGameCallback, public Environment
{
public:
	std::map<PlayerColor, std::shared_ptr<CGameInterface>> playerint;
	std::map<PlayerColor, std::shared_ptr<CBattleGameInterface>> battleints;

	std::map<PlayerColor, std::vector<std::shared_ptr<IBattleEventsReceiver>>> additionalBattleInts;

	std::unique_ptr<BattleAction> currentBattleAction;

	CClient();
	~CClient();

	const Services * services() const override;
	const BattleCb * battle(const BattleID & battle) const override;
	const GameCb * game() const override;
	vstd::CLoggerBase * logger() const override;
	events::EventBus * eventBus() const override;

	void newGame(CGameState * gameState);
	void loadGame(CGameState * gameState);

	void save(const std::string & fname);
	void endNetwork();
	void finishGameplay();
	void endGame();

	void initMapHandler();
	void initPlayerEnvironments();
	void initPlayerInterfaces();
	std::string aiNameForPlayer(const PlayerSettings & ps, bool battleAI, bool alliedToHuman) const; //empty means no AI -> human
	std::string aiNameForPlayer(bool battleAI, bool alliedToHuman) const;
	void installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, PlayerColor color, bool battlecb = false);
	void installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, PlayerColor color, bool needCallback = true);

	ThreadSafeVector<int> waitingRequest;

	void handlePack(CPackForClient & pack); //applies the given pack and deletes it
	int sendRequest(const CPackForServer & request, PlayerColor player); //returns ID given to that request

	void battleStarted(const BattleInfo * info);
	void battleFinished(const BattleID & battleID);
	void startPlayerBattleAction(const BattleID & battleID, PlayerColor color);

	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions

	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> & spells) override {};
	void setResearchedSpells(const CGTownInstance * town, int level, const std::vector<SpellID> & spells, bool accepted) override {};
	bool removeObject(const CGObjectInstance * obj, const PlayerColor & initiator) override {return false;};
	void createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator) override {};
	void setOwner(const CGObjectInstance * obj, PlayerColor owner) override {};
	void giveExperience(const CGHeroInstance * hero, TExpType val) override {};
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, bool abs = false) override {};
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs = false) override {};

	void showBlockingDialog(const IObjectInterface * caller, BlockingDialog * iw) override {};
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override {};
	void showTeleportDialog(TeleportDialog * iw) override {};
	void showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery) override {};
	void giveResource(PlayerColor player, GameResID which, int val) override {};
	void giveResources(PlayerColor player, TResources resources) override {};

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
	bool giveHeroNewArtifact(const CGHeroInstance * h, const ArtifactID & artId, const ArtifactPosition & pos) override {return false;};
	bool giveHeroNewScroll(const CGHeroInstance * h, const SpellID & spellId, const ArtifactPosition & pos) override {return false;};
	bool putArtifact(const ArtifactLocation & al, const ArtifactInstanceID & id, std::optional<bool> askAssemble) override {return false;};
	void removeArtifact(const ArtifactLocation & al) override {};
	bool moveArtifact(const PlayerColor & player, const ArtifactLocation & al1, const ArtifactLocation & al2) override {return false;};

	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void startBattle(const CArmedInstance * army1, const CArmedInstance * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, const BattleLayout & layout, const CGTownInstance * town) override {}; //use hero=nullptr for no hero
	void startBattle(const CArmedInstance * army1, const CArmedInstance * army2) override {}; //if any of armies is hero, hero will be used
	bool moveHero(ObjectInstanceID hid, int3 dst, EMovementMode movementMode, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override {return false;};
	void giveHeroBonus(GiveBonus * bonus) override {};
	void setMovePoints(SetMovePoints * smp) override {};
	void setMovePoints(ObjectInstanceID hid, int val, bool absolute) override {};
	void setManaPoints(ObjectInstanceID hid, int val) override {};
	void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId = ObjectInstanceID()) override {};
	void changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator) override {};
	void sendAndApply(CPackForClient & pack) override {};
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override {};
	void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) override {};

	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode) override {}
	void changeFogOfWar(const std::unordered_set<int3> & tiles, PlayerColor player, ETileVisibility mode) override {}

	void setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value) override {};
	void setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier) override {};
	void setRewardableObjectConfiguration(ObjectInstanceID objid, const Rewardable::Configuration & configuration) override {};
	void setRewardableObjectConfiguration(ObjectInstanceID townInstanceID, BuildingID buildingID, const Rewardable::Configuration & configuration) override{};

	void showInfoDialog(InfoWindow * iw) override {};
	void removeGUI() const;

	vstd::RNG & getRandomGenerator() override;

#if SCRIPTING_ENABLED
	scripting::Pool * getGlobalContextPool() const override;
#endif

private:
	std::map<PlayerColor, std::shared_ptr<CBattleCallback>> battleCallbacks; //callbacks given to player interfaces
	std::map<PlayerColor, std::shared_ptr<CPlayerEnvironment>> playerEnvironments;

#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolImpl> clientScripts;
#endif
	std::unique_ptr<events::EventBus> clientEventBus;

	void reinitScripting();
};
