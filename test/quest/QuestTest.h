/*
 * QuestTest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

#include "../mock/TinyH3MBuilder.h"
#include "../mock/mock_MapServiceTinyH3M.h"
#include "../mock/mock_Services.h"
#include "../mock/mock_IGameEventCallback.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapHeader.h"

#include <vcmi/ServerCallback.h>

class CGObjectInstance;
class CGHeroInstance;
class CGSeerHut;
class CGQuestGuard;

/// Test fixture for scenarios involving quest objects. Loads a TinyH3MBuilder
/// scenario into a live CGameState and exposes helpers for the everyday
/// quest-flow steps: locate a placed object, walk a hero onto it, answer
/// dialogs, hand out resources, advance the calendar.
class QuestTest : public ::testing::Test, public ServerCallback, public MapListener
{
public:
	QuestTest()
		: gameEventCallback(std::make_shared<GameEventCallbackMock>(this))
	{
	}

	void SetUp() override
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(&services);
	}

	void TearDown() override
	{
		gameState.reset();
		mapService.reset();
		map = nullptr;
	}

	// ---- ServerCallback overrides ---------------------------------------

	bool describeChanges() const override { return true; }
	void apply(CPackForClient & pack) override { gameState->apply(pack); }
	void complain(const std::string & problem) override
	{
		FAIL() << "Server-side assertion: " << problem;
	}
	vstd::RNG * getRNG() override { return &randomGenerator; }

	// Battle-specific overrides are no-ops for quest tests.
	void apply(BattleLogMessage &) override {}
	void apply(BattleStackMoved &) override {}
	void apply(BattleUnitsChanged &) override {}
	void apply(SetStackEffect &) override {}
	void apply(StacksInjured &) override {}
	void apply(BattleObstaclesChanged &) override {}
	void apply(CatapultAttack &) override {}

	// ---- MapListener override -------------------------------------------

	void mapLoaded(CMap * loadedMap) override
	{
		EXPECT_EQ(this->map, nullptr);
		this->map = loadedMap;
		// Game-setting overrides must land before initObj runs, since some
		// settings get baked into per-object state at init time.
		for(const auto & ov : pendingOverrides)
		{
			JsonNode node;
			node.Bool() = ov.value;
			loadedMap->overrideGameSetting(ov.option, node);
		}
	}

	// ---- public test API ------------------------------------------------

	/// Load a scenario into a fresh game. After this returns, `map` and
	/// `gameState` are populated and ready for assertions.
	void startWithMap(TinyH3M::TinyH3MBuilder builder);

	/// Find the object placed on a given tile.
	CGObjectInstance * findObjectAt(const int3 & pos) const;
	CGHeroInstance   * findHeroAt(const int3 & pos) const;
	CGHeroInstance   * findHeroByOwner(PlayerColor owner) const;

	/// Locate the object at `pos` and verify it has dynamic type `T`. Fails the
	/// test if no object is there or the type does not match.
	template<class T>
	T * expectAt(const int3 & pos) const
	{
		auto * obj = findObjectAt(pos);
		EXPECT_NE(obj, nullptr) << "no object at " << pos.toString();
		auto * casted = dynamic_cast<T *>(obj);
		EXPECT_NE(casted, nullptr) << "object at " << pos.toString() << " has unexpected dynamic type";
		return casted;
	}

	/// Find the first / all object(s) of a given dynamic type on the map.
	template<class T>
	T * findFirst() const
	{
		for(const auto & obj : map->objects)
		{
			if(auto * casted = dynamic_cast<T *>(obj.get()))
				return casted;
		}
		return nullptr;
	}

	template<class T>
	std::vector<T *> findAll() const
	{
		std::vector<T *> out;
		for(const auto & obj : map->objects)
		{
			if(auto * casted = dynamic_cast<T *>(obj.get()))
				out.push_back(casted);
		}
		return out;
	}

	/// Top up a player's stockpile (scenarios start with empty resources).
	void grantResources(PlayerColor player, GameResID which, int amount);

	/// Pretend the player has just defeated `target` — short-circuits the
	/// adventure-map battle that a kill-quest would normally require.
	void markObjectDestroyed(PlayerColor player, ObjectInstanceID target);

	/// Walk `hero` onto `obj` and trigger its visit handler.
	void visit(CGHeroInstance * hero, CGObjectInstance * obj);

	/// Answer the most recent BlockingDialog. Fails if none is pending.
	void answerDialog(CGHeroInstance * hero, int32_t answer);

	/// Advance the in-game calendar by `days`.
	void advanceDays(int days);

	/// Override a game setting before any object's init runs. Must be called
	/// before startWithMap — some settings bake into per-object state at init,
	/// so a post-startWithMap override won't take effect on already-built objects.
	void overrideSettingBeforeInit(EGameSettings option, bool value);

protected:
	struct PendingOverride
	{
		EGameSettings option;
		bool          value;
	};
	std::vector<PendingOverride> pendingOverrides;

	std::shared_ptr<CGameState>            gameState;
	std::shared_ptr<GameEventCallbackMock> gameEventCallback;
	std::unique_ptr<MapServiceTinyH3M>     mapService;
	ServicesMock                           services;
	CMap *                                 map = nullptr;
	CRandomGenerator                       randomGenerator;
};
