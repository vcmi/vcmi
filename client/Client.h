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

#include "../lib/callback/IClient.h"
#include "../lib/callback/CGameInfoCallback.h"
#include "../lib/ConditionalWait.h"
#include "../lib/ResourceSet.h"


VCMI_LIB_NAMESPACE_BEGIN

struct CPackForClient;
struct CPackForServer;
class IBattleEventsReceiver;
class CBattleGameInterface;
class CGameInterface;
class BattleAction;
class BattleInfo;
struct BankConfig;
class CCallback;
class CBattleCallback;

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
		cond.wait(lock, [this, &item](){ return !vstd::contains(items, item);});

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
class CClient : public Environment, public IClient
{
	std::shared_ptr<CGameState> gamestate;
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

	CGameState & gameState() { return *gamestate; }
	const CGameState & gameState() const { return *gamestate; }
	IGameInfoCallback & gameInfo();

	void newGame(std::shared_ptr<CGameState> gameState);
	void loadGame(std::shared_ptr<CGameState> gameState);

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

	//Set of metrhods that allows adding more interfaces for this player that'll receive game event call-ins.
	void registerBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents, PlayerColor color);
	void unregisterBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents, PlayerColor color);

	ThreadSafeVector<int> waitingRequest;

	void handlePack(CPackForClient & pack); //applies the given pack and deletes it
	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override; //returns ID given to that request
	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor player, const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

	void battleStarted(const BattleID & battle);
	void battleFinished(const BattleID & battleID);
	void startPlayerBattleAction(const BattleID & battleID, PlayerColor color);

	friend class CCallback; //handling players actions
	friend class CBattleCallback; //handling players actions

	void removeGUI() const;

private:
	std::map<PlayerColor, std::shared_ptr<CBattleCallback>> battleCallbacks; //callbacks given to player interfaces
	std::map<PlayerColor, std::shared_ptr<CPlayerEnvironment>> playerEnvironments;

#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::PoolImpl> clientScripts;
#endif
	std::unique_ptr<events::EventBus> clientEventBus;

	void reinitScripting();
};
