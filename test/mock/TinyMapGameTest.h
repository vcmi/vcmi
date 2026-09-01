/*
 * TinyMapGameTest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "TinyH3MBuilder.h"
#include "mock_MapServiceTinyH3M.h"
#include "mock_Services.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapping/CMap.h"

#include <vcmi/ServerCallback.h>

class CCallback;
class CGHeroInstance;
class CGObjectInstance;
class IClient;

/// Loads a TinyH3M scenario into a live game state and exposes common map helpers.
class TinyMapGameTest : public ::testing::Test, public ServerCallback, public MapListener
{
public:
	void SetUp() override;
	void TearDown() override;

	bool describeChanges() const override { return true; }
	void apply(CPackForClient & pack) override { currentGameState->apply(pack); }
	void complain(const std::string & problem) override;
	vstd::RNG * getRNG() override { return &testRandomGenerator; }
	bool rollCombatAbility(const IBattleInfoCallback &, const battle::Unit &, int) override { return false; }

	void apply(BattleLogMessage &) override {}
	void apply(BattleStackMoved &) override {}
	void apply(BattleUnitsChanged &) override {}
	void apply(SetStackEffect &) override {}
	void apply(StacksInjured &) override {}
	void apply(BattleObstaclesChanged &) override {}
	void apply(CatapultAttack &) override {}

	void mapLoaded(CMap * loadedMap) override;

	void startWithMap(TinyH3M::TinyH3MBuilder builder);
	void startWithMap(TinyH3M::TinyH3MBuilder builder, EMapDifficulty difficulty);

	CGObjectInstance * findObjectAt(const int3 & pos) const;
	CGHeroInstance * findHeroAt(const int3 & pos) const;
	CGHeroInstance * findHeroByOwner(PlayerColor owner) const;

	template<class T>
	T * expectAt(const int3 & pos) const
	{
		auto * object = findObjectAt(pos);
		EXPECT_NE(object, nullptr) << "no object at " << pos.toString();
		auto * result = dynamic_cast<T *>(object);
		EXPECT_NE(result, nullptr) << "object at " << pos.toString() << " has unexpected dynamic type";
		return result;
	}

	template<class T>
	T * findFirst() const
	{
		for(const auto & object : loadedMap->objects)
		{
			if(auto * result = dynamic_cast<T *>(object.get()))
				return result;
		}
		return nullptr;
	}

	template<class T>
	std::vector<T *> findAll() const
	{
		std::vector<T *> result;
		for(const auto & object : loadedMap->objects)
		{
			if(auto * converted = dynamic_cast<T *>(object.get()))
				result.push_back(converted);
		}
		return result;
	}

	void revealMap(PlayerColor player);
	void setMapVisibility(PlayerColor player, bool visible);
	std::shared_ptr<CCallback> makeCallback(PlayerColor player, IClient * client = nullptr) const;
	void grantResources(PlayerColor player, GameResID resource, int amount);
	void overrideSettingBeforeInit(EGameSettings option, bool value);

protected:
	virtual void onMapStarted() {}
	/// Services the game state is initialized with. The mock covers map-level tests; a test that
	/// runs real game logic overrides this with the game library.
	virtual Services * gameServices() { return &services; }
	/// Applied to every player of the scenario, for tests that need a specific starting setup.
	virtual void configurePlayer(PlayerSettings & settings) const {}

	const std::shared_ptr<CGameState> & gameState() const { return currentGameState; }
	CMap * map() const { return loadedMap; }

private:
	struct PendingOverride
	{
		EGameSettings option;
		bool value;
	};

	std::vector<PendingOverride> pendingOverrides;
	std::shared_ptr<CGameState> currentGameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	ServicesMock services;
	CMap * loadedMap = nullptr;
	CRandomGenerator testRandomGenerator;
};
