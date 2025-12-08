/*
 * GameStateTest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

#include "../mock/mock_Services.h"
#include "../mock/mock_MapService.h"
#include "../mock/mock_IGameEventCallback.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/filesystem/ResourcePath.h"

/**
 * Base test fixture for game state tests that need a fully initialized game.
 *
 * This class provides:
 * - Game state initialization with a test map
 * - Server callback implementation for applying netpacks
 * - Mock services and event callbacks
 * - Helper methods for common game state operations
 *
 * Inherit from this class to test netpacks and other game state operations.
 */
class GameStateTest : public ::testing::Test, public ServerCallback, public MapListener
{
public:
	GameStateTest()
		: gameEventCallback(std::make_shared<GameEventCallbackMock>(this)),
		mapService("test/MiniTest/", this),
		map(nullptr)
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
	}

	bool describeChanges() const override
	{
		return true;
	}

	void apply(CPackForClient & pack) override
	{
		gameState->apply(pack);
	}

	void complain(const std::string & problem) override
	{
		FAIL() << "Server-side assertion: " << problem;
	}

	vstd::RNG * getRNG() override
	{
		return &randomGenerator;
	}

	// Unused battle-specific overrides - implement as needed in derived classes
	void apply(BattleLogMessage &) override {}
	void apply(BattleStackMoved &) override {}
	void apply(BattleUnitsChanged &) override {}
	void apply(SetStackEffect &) override {}
	void apply(StacksInjured &) override {}
	void apply(BattleObstaclesChanged &) override {}
	void apply(CatapultAttack &) override {}

	void mapLoaded(CMap * map) override
	{
		EXPECT_EQ(this->map, nullptr);
		this->map = map;
	}

	/**
	 * Initialize a test game with the specified map.
	 *
	 * This method:
	 * - Loads the map header
	 * - Configures player settings
	 * - Initializes the game state
	 * - Makes the game ready for testing
	 *
	 * @param mapName Name of the test map to load (default: "anything")
	 */
	void startTestGame(const std::string & mapName = "anything")
	{
		StartInfo si;
		si.mapname = mapName;
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		std::unique_ptr<CMapHeader> header = mapService.loadMapHeader(ResourcePath(si.mapname));
		ASSERT_NE(header.get(), nullptr) << "Failed to load map header for: " << mapName;

		// Setup player info
		for(int i = 0; i < header->players.size(); i++)
		{
			const PlayerInfo & pinfo = header->players[i];

			if (!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si.playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			pset.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			pset.name = "Player";
			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();

			if(pset.hero != HeroTypeID::RANDOM && pinfo.hasCustomMainHero())
			{
				pset.hero = pinfo.mainCustomHeroId;
				pset.heroNameTextId = pinfo.mainCustomHeroNameTextId;
				pset.heroPortrait = HeroTypeID(pinfo.mainCustomHeroPortrait);
			}
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(&mapService, &si, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr) << "Game state initialization failed - map is null";
	}

protected:
	std::shared_ptr<CGameState> gameState;
	std::shared_ptr<GameEventCallbackMock> gameEventCallback;
	MapServiceMock mapService;
	ServicesMock services;
	CMap * map;
	CRandomGenerator randomGenerator;
};
