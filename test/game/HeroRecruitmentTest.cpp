/*
 * HeroRecruitmentTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_Services.h"
#include "mock/mock_MapService.h"
#include "mock/mock_IGameEventCallback.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/TavernHeroesPool.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/filesystem/ResourcePath.h"

class HeroRecruitmentTest : public ::testing::Test, public ServerCallback, public MapListener
{
public:
	HeroRecruitmentTest()
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

	// Unused battle-specific overrides - not needed for hero recruitment tests
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

	void startTestGame()
	{
		StartInfo si;
		si.mapname = "anything";
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		std::unique_ptr<CMapHeader> header = mapService.loadMapHeader(ResourcePath(si.mapname));
		ASSERT_NE(header.get(), nullptr);

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

		ASSERT_NE(map, nullptr);
	}

	std::shared_ptr<CGameState> gameState;
	std::shared_ptr<GameEventCallbackMock> gameEventCallback;
	MapServiceMock mapService;
	ServicesMock services;
	CMap * map;
	CRandomGenerator randomGenerator;
};

// Test that hero recruitment properly assigns an ID to the recruited hero
TEST_F(HeroRecruitmentTest, recruitedHeroGetsId)
{
	startTestGame();

	// Get the first player and find a town
	PlayerColor playerColor = PlayerColor(0);
	auto * playerState = gameState->getPlayerState(playerColor);
	ASSERT_NE(playerState, nullptr);

	// Get initial state
	auto initialHeroes = playerState->getHeroes();
	size_t initialHeroCount = initialHeroes.size();

	// Find a town for recruitment
	auto towns = playerState->getTowns();
	ASSERT_FALSE(towns.empty()) << "Player must have at least one town for hero recruitment";
	auto * town = towns.front();
	ASSERT_NE(town, nullptr);

	// Get a hero type from the pool
	auto poolHeroes = gameState->heroesPool->getHeroesFor(playerColor);
	ASSERT_FALSE(poolHeroes.empty()) << "Hero pool must have heroes available";

	const auto * poolHero = poolHeroes.front();
	ASSERT_NE(poolHero, nullptr);
	HeroTypeID heroType = poolHero->getHeroTypeID();

	// Create and apply HeroRecruited pack
	HeroRecruited pack;
	pack.tid = town->id;
	pack.hid = heroType;
	pack.player = playerColor;
	pack.tile = town->visitablePos();

	gameEventCallback->sendAndApply(pack);

	// Verify player now has one more hero
	auto finalHeroes = playerState->getHeroes();
	EXPECT_EQ(finalHeroes.size(), initialHeroCount + 1)
		<< "Player should have exactly one more hero after recruitment";

	// Find the newly recruited hero
	CGHeroInstance * recruitedHero = nullptr;
	for (auto * hero : finalHeroes)
	{
		if (std::find(initialHeroes.begin(), initialHeroes.end(), hero) == initialHeroes.end())
		{
			recruitedHero = hero;
			break;
		}
	}

	ASSERT_NE(recruitedHero, nullptr) << "Recruited hero must exist in player's hero list";

	// This is the key assertion that validates our fix:
	// The hero must have an ID assigned after being added to the map
	EXPECT_TRUE(recruitedHero->id.hasValue())
		<< "Recruited hero must have an ID assigned (validates fix for assertion crash)";

	// Verify other properties
	EXPECT_EQ(recruitedHero->getOwner(), playerColor)
		<< "Recruited hero must be owned by the recruiting player";
	EXPECT_EQ(recruitedHero->visitablePos(), town->visitablePos())
		<< "Recruited hero must be at town's position";
	EXPECT_EQ(recruitedHero->getHeroTypeID(), heroType)
		<< "Recruited hero must have the correct hero type";

	// Verify the hero is on the map
	auto * heroOnMap = dynamic_cast<CGHeroInstance *>(map->getObject(recruitedHero->id));
	EXPECT_EQ(heroOnMap, recruitedHero)
		<< "Recruited hero must be retrievable from map by ID";
}

// Test hero recruitment without a town (should still assign ID)
TEST_F(HeroRecruitmentTest, recruitedHeroWithoutTownGetsId)
{
	startTestGame();

	PlayerColor playerColor = PlayerColor(0);
	auto * playerState = gameState->getPlayerState(playerColor);
	ASSERT_NE(playerState, nullptr);

	auto poolHeroes = gameState->heroesPool->getHeroesFor(playerColor);
	ASSERT_FALSE(poolHeroes.empty());

	const auto * poolHero = poolHeroes.front();
	HeroTypeID heroType = poolHero->getHeroTypeID();

	// Recruit hero without a town (tavern can be in random locations)
	HeroRecruited pack;
	pack.tid = ObjectInstanceID(); // No town
	pack.hid = heroType;
	pack.player = playerColor;
	pack.tile = int3(5, 5, 0); // Random valid position

	gameEventCallback->sendAndApply(pack);

	// Find the recruited hero
	auto finalHeroes = playerState->getHeroes();
	ASSERT_FALSE(finalHeroes.empty());

	auto * recruitedHero = finalHeroes.back();
	ASSERT_NE(recruitedHero, nullptr);

	// Key assertion: ID must be assigned even without a town
	EXPECT_TRUE(recruitedHero->id.hasValue())
		<< "Hero recruited without town must still have ID assigned";

	EXPECT_EQ(recruitedHero->getOwner(), playerColor);
	EXPECT_EQ(recruitedHero->getHeroTypeID(), heroType);
}
