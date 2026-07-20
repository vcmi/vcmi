/*
 * ExploreNeighbourTileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/ExploreNeighbourTile.h"

#include "mock/TinyH3MBuilder.h"
#include "mock/mock_MapServiceTinyH3M.h"
#include "mock/mock_Services.h"

#include "lib/CPlayerState.h"
#include "lib/StartInfo.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/GameRandomizer.h"
#include "lib/filesystem/ResourcePath.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"
#include "lib/mapping/CMapHeader.h"

#include <cstdio>
#include <cstdlib>

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);

NK2AI::NeighbourExplorationCandidate makeAcceptedCandidate()
{
	NK2AI::NeighbourExplorationCandidate candidate;
	candidate.sameDay = true;
	candidate.accessible = true;
	candidate.safe = true;
	candidate.tilesDiscovered = 3;
	candidate.movementCost = 0.5f;
	return candidate;
}

TinyH3M::TinyH3MBuilder makeStrategicNeighbourMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2StrategicNeighbour")
		.playerActive(PLAYER)
		.playerActive(ENEMY)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.randomTown({9, 5, 0}, ENEMY);

	return builder;
}

class VisitabilityTrapObject : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	bool wasVisited(const CGHeroInstance *) const override
	{
		std::fputs(
			"ExploreNeighbourTile queried remembered object visitability while scoring a neighbour\n",
			stderr);
		std::abort();
	}
};

class Nullkiller2_Goals_ExploreNeighbourTileStrategic : public ::testing::Test, public MapListener
{
public:
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

	void mapLoaded(CMap * loadedMap) override
	{
		EXPECT_EQ(map, nullptr);
		map = loadedMap;
	}

	void startWithMap(TinyH3M::TinyH3MBuilder builder)
	{
		auto bytes = builder.build();
		mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

		StartInfo si;
		si.mapname = "tiny";
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		auto header = mapService->loadMapHeader(ResourcePath(si.mapname));
		ASSERT_NE(header.get(), nullptr) << "TinyH3M scenario header failed to load";

		for(int i = 0; i < static_cast<int>(header->players.size()); ++i)
		{
			const PlayerInfo & pinfo = header->players[i];
			if(!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si.playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			pset.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			pset.name = "Player";
			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(mapService.get(), &si, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr) << "gameState init did not populate the CMap";
	}

	CGHeroInstance * findHeroByOwner(PlayerColor owner) const
	{
		for(const auto & obj : map->objects)
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(obj.get());
			if(hero && hero->getOwner() == owner)
				return hero;
		}
		return nullptr;
	}

	void setAllTilesVisible(PlayerColor player, bool visible)
	{
		auto & fow = teamState(player).fogOfWarMap;

		for(int z = 0; z < map->levels(); ++z)
		{
			for(int x = 0; x < map->width; ++x)
			{
				for(int y = 0; y < map->height; ++y)
					fow[int3(x, y, z)] = visible ? 1 : 0;
			}
		}
	}

	void setTerrain(const int3 & pos, ETerrainId terrain)
	{
		map->getTile(pos).terrainType = TerrainId(terrain);
	}

	std::shared_ptr<CCallback> makeCallback(PlayerColor player)
	{
		return std::make_shared<CCallback>(gameState, std::optional<PlayerColor>{player}, nullptr);
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway(const std::shared_ptr<CCallback> & callback)
	{
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		return gateway;
	}

	CGObjectInstance * addVisitabilityTrapObject(
		const std::shared_ptr<CCallback> & callback,
		const CGObjectInstance * appearanceSource,
		const int3 & pos)
	{
		auto trap = std::make_shared<VisitabilityTrapObject>(callback.get());
		trap->ID = Obj::ARTIFACT;
		trap->subID = MapObjectSubID(0);
		trap->tempOwner = PLAYER;
		trap->id = ObjectInstanceID(static_cast<si32>(map->objects.size()));
		trap->appearance = appearanceSource->appearance;
		trap->setAnchorPos(pos);

		map->objects.push_back(trap);
		return trap.get();
	}

private:
	TeamState & teamState(PlayerColor player)
	{
		return gameState->teams.at(gameState->players.at(player).team);
	}

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	ServicesMock services;
	CMap * map = nullptr;
};
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, acceptsSafeSameDayDiscovery)
{
	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(makeAcceptedCandidate());

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 18.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, discountsNormalMovementCost)
{
	auto cheaperCandidate = makeAcceptedCandidate();
	auto expensiveCandidate = makeAcceptedCandidate();
	expensiveCandidate.movementCost = 1.5f;

	EXPECT_GT(
		NK2AI::evaluateNeighbourExplorationCandidate(cheaperCandidate).value,
		NK2AI::evaluateNeighbourExplorationCandidate(expensiveCandidate).value)
		<< "equal discovery should prefer the neighbour that leaves more movement";
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, clampsTinyMovementCost)
{
	auto candidate = makeAcceptedCandidate();
	candidate.movementCost = 0.05f;

	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 90.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsNextDayMove)
{
	auto candidate = makeAcceptedCandidate();
	candidate.sameDay = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsUnsafeDestination)
{
	auto candidate = makeAcceptedCandidate();
	candidate.safe = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsMoveWithNoDiscovery)
{
	auto candidate = makeAcceptedCandidate();
	candidate.tilesDiscovered = 0;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST_F(Nullkiller2_Goals_ExploreNeighbourTileStrategic, findsVisibleStrategicTargetWithoutNewDiscovery)
{
	startWithMap(makeStrategicNeighbourMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	setAllTilesVisible(PLAYER, true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	const auto target = NK2AI::Goals::ExploreNeighbourTile::findTarget(
		hero,
		gateway->nullkiller.get());

	ASSERT_TRUE(target.has_value());
	EXPECT_EQ(target->tilesDiscovered, 0);
}

TEST_F(Nullkiller2_Goals_ExploreNeighbourTileStrategic, ignoresKnownObjectsBeforeVisitabilityChecks)
{
	startWithMap(makeStrategicNeighbourMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(hero->appearance, nullptr);
	hero->setMovementPoints(2000);

	setAllTilesVisible(PLAYER, true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	auto * trap = addVisitabilityTrapObject(callback, hero, int3(30, 30, 0));
	gateway->nullkiller->memory->addVisitableObject(trap);

	const auto target = NK2AI::Goals::ExploreNeighbourTile::findTarget(
		hero,
		gateway->nullkiller.get());

	ASSERT_TRUE(target.has_value());
	EXPECT_EQ(target->tilesDiscovered, 0);
}

TEST_F(Nullkiller2_Goals_ExploreNeighbourTileStrategic, reportsUnreachableLivePathAsFailure)
{
	startWithMap(makeStrategicNeighbourMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	const int3 unreachableTile(6, 5, 0);
	setTerrain(unreachableTile, ETerrainId::ROCK);
	setAllTilesVisible(PLAYER, true);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);

	EXPECT_THROW(
		gateway->moveHeroToTile(unreachableTile, NK2AI::HeroPtr(hero, callback.get())),
		NK2AI::cannotFulfillGoalException);
}
