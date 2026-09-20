/*
 * OneWayPortalBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Behaviors/ExplorationBehavior.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"

#include "mock/TinyH3MBuilder.h"
#include "mock/mock_MapServiceTinyH3M.h"
#include "mock/mock_Services.h"

#include "lib/CPlayerState.h"
#include "lib/StartInfo.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
#include "lib/callback/GameRandomizer.h"
#include "lib/filesystem/ResourcePath.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"
#include "lib/mapping/CMapHeader.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/networkPacks/SaveLocalState.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);
constexpr int PORTAL_CHANNEL = 0;

class ApplyingClient : public IClient
{
public:
	explicit ApplyingClient(std::shared_ptr<CGameState> gameState)
		: gameState(std::move(gameState))
	{
	}

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor,
		const BattleID &,
		const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(const auto * localState = dynamic_cast<const SaveLocalState *>(&request))
		{
			*gameState->getPlayerState(localState->player)->playerLocalSettings = localState->data;
			return ++requestID;
		}
		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		if(!movement)
			return ++requestID;

		auto * hero = gameState->getHero(movement->hid);
		if(!hero || movement->path.empty())
			return ++requestID;

		for(const int3 & requestedDestination : movement->path)
		{
			const bool stopPartway = partialMovementDestination.isValid();
			const int3 destination = stopPartway
				? hero->convertFromVisitablePos(partialMovementDestination)
				: requestedDestination;
			const int3 destinationTile = hero->convertToVisitablePos(destination);
			requestedTiles.push_back(destinationTile);

			TryMoveHero appliedMovement;
			appliedMovement.id = hero->id;
			appliedMovement.start = hero->pos;
			appliedMovement.movePoints = stopPartway
				? partialMovementPoints
				: hero->movementPointsRemaining();

			if(!stopPartway && destinationTile == entrance)
			{
				appliedMovement.result = TryMoveHero::TELEPORTATION;
				appliedMovement.end = hero->convertFromVisitablePos(exit);
			}
			else
			{
				appliedMovement.result = TryMoveHero::SUCCESS;
				appliedMovement.end = destination;
			}

			gameState->apply(appliedMovement);
			if(stopPartway || appliedMovement.movePoints == 0)
				break;
		}

		return ++requestID;
	}

	void connectPortal(const int3 & entrancePos, const int3 & exitPos)
	{
		entrance = entrancePos;
		exit = exitPos;
	}

	bool requested(const int3 & tile) const
	{
		return vstd::contains(requestedTiles, tile);
	}

	void stopMovementAt(const int3 & destination, int remainingMovementPoints = 0)
	{
		partialMovementDestination = destination;
		partialMovementPoints = remainingMovementPoints;
	}

private:
	std::shared_ptr<CGameState> gameState;
	std::vector<int3> requestedTiles;
	int3 entrance = int3(-1);
	int3 exit = int3(-1);
	int requestID = 0;
	int3 partialMovementDestination = int3(-1);
	int partialMovementPoints = 0;
};

TinyH3M::TinyH3MBuilder makeOneWayPortalMap(
	const std::vector<uint16_t> & heroArmySizes,
	bool withTown = true,
	int exitCount = 1)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2OneWayPortal")
		.playerActive(PLAYER);

	if(withTown)
		builder.randomTown({5, 5, 0}, PLAYER);

	for(size_t i = 0; i < heroArmySizes.size(); ++i)
	{
		builder
			.hero({7 + static_cast<int>(i) * 2, 10, 0}, HeroTypeID(static_cast<int>(i)), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, heroArmySizes[i]}});
	}

	builder.oneWayPortalEntrance({16, 10, 0}, PORTAL_CHANNEL);
	for(int i = 0; i < exitCount; ++i)
		builder.oneWayPortalExit({24 + i * 3, 24, 0}, PORTAL_CHANNEL);

	return builder;
}

TinyH3M::TinyH3MBuilder makeBoatAssistedOneWayPortalMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2BoatAssistedOneWayPortal")
		.playerActive(PLAYER)
		.hero({10, 22, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.shipyard({10, 18, 0}, PLAYER)
		.oneWayPortalEntrance({10, 6, 0}, PORTAL_CHANNEL)
		.oneWayPortalExit({24, 24, 0}, PORTAL_CHANNEL);

	for(int y = 8; y <= 17; ++y)
	{
		for(int x = 0; x < 36; ++x)
			builder.terrain({x, y, 0}, TerrainId::WATER);
	}

	return builder;
}

TinyH3M::TinyH3MBuilder makeOneWayPortalBarrierMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2OneWayPortalBarrier")
		.playerActive(PLAYER)
		.hero({5, 10, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.oneWayPortalEntrance({10, 10, 0}, PORTAL_CHANNEL)
		.oneWayPortalExit({24, 24, 0}, PORTAL_CHANNEL);

	for(int y = 0; y < 36; ++y)
	{
		for(int x = 0; x < 36; ++x)
		{
			if(y != 10)
				builder.terrain({x, y, 0}, TerrainId::WATER);
		}
	}

	return builder;
}

TinyH3M::TinyH3MBuilder makeIsolatedEnemyPortalMap()
{
	auto builder = makeOneWayPortalMap({500, 1});
	builder
		.playerActive(ENEMY)
		.randomTown({30, 24, 0}, ENEMY);

	for(int y = 0; y < 36; ++y)
		builder.terrain({20, y, 0}, TerrainId::WATER);

	return builder;
}

class OneWayPortalBehaviorTest : public ::testing::Test, public MapListener
{
public:
	void SetUp() override
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(&services);
	}

	void TearDown() override
	{
		gateway.reset();
		client.reset();
		gameState.reset();
		mapService.reset();
		map = nullptr;
	}

	void mapLoaded(CMap * loadedMap) override
	{
		ASSERT_EQ(map, nullptr);
		map = loadedMap;
	}

	void startWithMap(TinyH3M::TinyH3MBuilder builder)
	{
		auto bytes = builder.build();
		mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

		StartInfo startInfo;
		startInfo.mapname = "tiny";
		startInfo.difficulty = 0;
		startInfo.mode = EStartMode::NEW_GAME;

		auto header = mapService->loadMapHeader(ResourcePath(startInfo.mapname));
		ASSERT_NE(header.get(), nullptr) << "TinyH3M scenario header failed to load";

		for(int i = 0; i < static_cast<int>(header->players.size()); ++i)
		{
			const PlayerInfo & playerInfo = header->players[i];
			if(!(playerInfo.canHumanPlay || playerInfo.canComputerPlay))
				continue;

			PlayerSettings & playerSettings = startInfo.playerInfos[PlayerColor(i)];
			playerSettings.color = PlayerColor(i);
			playerSettings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			playerSettings.name = "Player";
			playerSettings.castle = playerInfo.defaultCastle();
			playerSettings.hero = playerInfo.defaultHero();
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(mapService.get(), &startInfo, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr) << "gameState init did not populate the CMap";
	}

	void setAllTilesVisible(bool visible)
	{
		auto & fogOfWar = teamState().fogOfWarMap;
		for(int z = 0; z < map->levels(); ++z)
		{
			for(int x = 0; x < map->width; ++x)
			{
				for(int y = 0; y < map->height; ++y)
					fogOfWar[int3(x, y, z)] = visible ? 1 : 0;
			}
		}
	}

	void setTileVisible(const int3 & tile, bool visible)
	{
		ASSERT_TRUE(map->isInTheMap(tile)) << tile.toString();
		teamState().fogOfWarMap[tile] = visible ? 1 : 0;
	}

	void setResource(GameResID resource, int amount)
	{
		gameState->players.at(PLAYER).resources[resource] = amount;
	}

	std::vector<CGHeroInstance *> heroesByStrength() const
	{
		std::vector<CGHeroInstance *> result;
		for(const auto & object : map->objects)
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(object.get());
			if(hero && hero->getOwner() == PLAYER)
				result.push_back(hero);
		}

		std::ranges::sort(result, [](const CGHeroInstance * left, const CGHeroInstance * right)
		{
			return left->getTotalStrength() > right->getTotalStrength();
		});
		return result;
	}

	CGHeroInstance * heroByOwner(PlayerColor owner) const
	{
		for(const auto & object : map->objects)
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(object.get());
			if(hero && hero->getOwner() == owner)
				return hero;
		}

		return nullptr;
	}

	std::vector<CGObjectInstance *> objectsByType(MapObjectID type) const
	{
		std::vector<CGObjectInstance *> result;
		for(const auto & object : map->objects)
		{
			if(object && object->ID == type)
				result.push_back(object.get());
		}
		return result;
	}

	CGObjectInstance * objectByType(MapObjectID type) const
	{
		const auto objects = objectsByType(type);
		return objects.empty() ? nullptr : objects.front();
	}

	void moveToVisitable(CGObjectInstance & object, const int3 & destination)
	{
		const int3 anchor = object.anchorPos() + destination - object.visitablePos();
		map->moveObject(object.id, anchor);
		ASSERT_EQ(object.visitablePos(), destination);
	}

	void recalculateGuardingPositions()
	{
		map->calculateGuardingGreaturePositions();
	}

	void markMissionCritical(const CGHeroInstance & hero)
	{
		EventCondition condition(EventCondition::CONTROL);
		condition.objectID = hero.id;
		condition.objectType = Obj(Obj::HERO);

		EventExpression::OperatorNone noneOf;
		noneOf.expressions.emplace_back(condition);

		TriggeredEvent event;
		event.identifier = "protectHero";
		event.effect.type = EventEffect::DEFEAT;
		event.trigger = EventExpression(noneOf);
		map->triggeredEvents.push_back(event);

		ASSERT_TRUE(hero.isMissionCritical());
	}

	void preparePlanning()
	{
		if(!gateway)
		{
			client = std::make_unique<ApplyingClient>(gameState);
			auto callback = std::make_shared<CCallback>(
				gameState,
				std::optional<PlayerColor>{PLAYER},
				client.get());
			gateway = std::make_unique<NK2AI::AIGateway>();
			gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		}

		for(auto * hero : heroesByStrength())
			hero->setMovementPoints(2500);

		gateway->nullkiller->heroManager->update();

		NK2AI::PathfinderSettings settings;
		settings.useHeroChain = false;
		settings.useDimensionDoor = false;
		gateway->nullkiller->pathfinder->updatePaths(
			gateway->nullkiller->getHeroesForPathfinding(),
			settings);
		gateway->nullkiller->decomposer->reset();
	}

	void restartGateway()
	{
		gateway.reset();
		client.reset();
		preparePlanning();
	}

	NK2AI::Goals::TGoalVec decomposePortalBehavior()
	{
		preparePlanning();
		return decomposePreparedPortalBehavior();
	}

	NK2AI::Goals::TGoalVec decomposePreparedPortalBehavior()
	{
		NK2AI::Goals::TGoalVec result;
		gateway->nullkiller->decomposer->decompose(
			result,
			NK2AI::Goals::sptr(NK2AI::Goals::ExplorationBehavior()),
			5);
		return result;
	}

	NK2AI::Goals::TGoalVec portalTasks(
		const NK2AI::Goals::TGoalVec & goals,
		const CGObjectInstance & entrance) const
	{
		NK2AI::Goals::TGoalVec result;
		for(const auto & goal : goals)
		{
			if(goal->asTask()->isObjectAffected(entrance.id))
				result.push_back(goal);
		}
		return result;
	}

	void teleportHero(
		CGHeroInstance & hero,
		const CGObjectInstance & entrance,
		const CGObjectInstance & exit)
	{
		TryMoveHero move;
		move.id = hero.id;
		move.result = TryMoveHero::TELEPORTATION;
		move.start = hero.convertFromVisitablePos(entrance.visitablePos());
		move.end = hero.convertFromVisitablePos(exit.visitablePos());
		move.movePoints = hero.movementPointsRemaining();

		gameState->apply(move);
		gateway->heroMoved(move, false);
		ASSERT_EQ(hero.visitablePos(), exit.visitablePos());
	}

	void moveHero(CGHeroInstance & hero, const int3 & destination)
	{
		TryMoveHero move;
		move.id = hero.id;
		move.result = TryMoveHero::SUCCESS;
		move.start = hero.pos;
		move.end = hero.convertFromVisitablePos(destination);
		move.movePoints = hero.movementPointsRemaining();

		gameState->apply(move);
		gateway->heroMoved(move, false);
		ASSERT_EQ(hero.visitablePos(), destination);
	}

	void finishTurn()
	{
		for(auto * hero : heroesByStrength())
			hero->setMovementPoints(0);

		gateway->makeTurn();
	}

	void returnHeroToTown(CGHeroInstance & hero)
	{
		auto * town = objectByType(Obj::TOWN);
		ASSERT_NE(town, nullptr);

		TryMoveHero move;
		move.id = hero.id;
		move.result = TryMoveHero::SUCCESS;
		move.start = hero.pos;
		move.end = hero.convertFromVisitablePos(town->visitablePos());
		move.movePoints = hero.movementPointsRemaining();

		gameState->apply(move);
		gateway->heroMoved(move, false);
		ASSERT_EQ(hero.visitablePos(), town->visitablePos());
	}

	void advanceDay()
	{
		NewTurn newTurn;
		newTurn.day = gameState->day + 1;
		gameState->apply(newTurn);
	}

	NK2AI::AIGateway & getGateway()
	{
		return *gateway;
	}

	ApplyingClient & getClient()
	{
		return *client;
	}

private:
	TeamState & teamState()
	{
		return gameState->teams.at(gameState->players.at(PLAYER).team);
	}

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	ServicesMock services;
	CMap * map = nullptr;
	std::unique_ptr<ApplyingClient> client;
	std::unique_ptr<NK2AI::AIGateway> gateway;
};
}

TEST_F(OneWayPortalBehaviorTest, VisibleExitDoesNotSuppressScoutProbe)
{
	startWithMap(makeOneWayPortalMap({100, 1}));
	setAllTilesVisible(true);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 2);
	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);

	const auto goals = decomposePortalBehavior();
	const auto probes = portalTasks(goals, *entrance);

	ASSERT_EQ(probes.size(), 1)
		<< "a visible exit incorrectly suppressed the one-way portal probe";
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(heroes.back()->id))
		<< "the one-way portal probe must use the scout";
	EXPECT_FALSE(probes.front()->asTask()->isObjectAffected(heroes.front()->id))
		<< "the main must not be assigned while a scout is eligible";
}

TEST_F(OneWayPortalBehaviorTest, OneWayEntranceIsReachableButCannotBeUsedAsCorridor)
{
	startWithMap(makeOneWayPortalBarrierMap());
	setAllTilesVisible(true);
	preparePlanning();

	const auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);

	const auto entrancePaths = getGateway().nullkiller->pathfinder->getPathInfo(
		entrance->visitablePos(),
		false);
	EXPECT_FALSE(entrancePaths.empty())
		<< "explicit probe goals must still be able to target the entrance";

	const auto pathsPastEntrance = getGateway().nullkiller->pathfinder->getPathInfo(
		entrance->visitablePos() + int3(4, 0, 0),
		false);
	EXPECT_TRUE(pathsPastEntrance.empty())
		<< "ordinary plans must not treat a one-way entrance as a walkable corridor";
}

TEST_F(OneWayPortalBehaviorTest, BoatProbeReservesMissingResources)
{
	startWithMap(makeBoatAssistedOneWayPortalMap());
	setAllTilesVisible(true);
	setResource(EGameResID::WOOD, 7);
	setResource(EGameResID::GOLD, 10000);

	const auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);
	const auto * shipyard = dynamic_cast<const IShipyard *>(objectByType(Obj::SHIPYARD));
	ASSERT_NE(shipyard, nullptr);
	ASSERT_EQ(shipyard->shipyardStatus(), IBoatGenerator::GOOD);

	const auto goals = decomposePortalBehavior();
	const auto probes = portalTasks(goals, *entrance);
	ASSERT_EQ(probes.size(), 1)
		<< "a portal probe behind a future boat must produce a resource-saving prerequisite";

	const auto parts = probes.front()->decompose(getGateway().nullkiller.get());
	ASSERT_FALSE(parts.empty());
	EXPECT_EQ(parts.back()->goalType, NK2AI::Goals::SAVE_RESOURCES);

	NK2AI::Goals::taskptr(*probes.front())->accept(&getGateway());
	EXPECT_EQ(getGateway().nullkiller->getLockedResources()[EGameResID::WOOD], 10);
	EXPECT_EQ(getGateway().nullkiller->getLockedResources()[EGameResID::GOLD], 1000);

	const auto repeatedProbes = portalTasks(decomposePortalBehavior(), *entrance);
	EXPECT_TRUE(repeatedProbes.empty())
		<< "an existing boat reservation must not produce another resource-saving task";
	EXPECT_EQ(getGateway().nullkiller->getLockedResources()[EGameResID::WOOD], 10);
	EXPECT_EQ(getGateway().nullkiller->getLockedResources()[EGameResID::GOLD], 1000);
}

TEST_F(OneWayPortalBehaviorTest, PartialPortalRouteRetainsReservationAndLocksHero)
{
	startWithMap(makeOneWayPortalMap({100}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * hero = heroesByStrength().front();
	ASSERT_NE(entrance, nullptr);
	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);
	ASSERT_EQ(probes.size(), 1);

	getClient().stopMovementAt(hero->visitablePos() + int3(1, 0, 0), 500);
	EXPECT_NO_THROW(probes.front()->asTask()->accept(&getGateway()));
	EXPECT_NE(hero->visitablePos(), entrance->visitablePos());
	EXPECT_EQ(hero->movementPointsRemaining(), 500);
	EXPECT_EQ(
		getGateway().nullkiller->getHeroLockedReason(hero),
		NK2AI::HeroLockedReason::HERO_CHAIN)
		<< "a partially completed portal route must resume on a later planning pass";
	EXPECT_EQ(
		getGateway().nullkiller->memory->getOneWayPortalReservation(entrance->id),
		std::optional<ObjectInstanceID>(hero->id));
}

TEST_F(OneWayPortalBehaviorTest, HiddenExitStillProducesScoutProbe)
{
	startWithMap(makeOneWayPortalMap({100, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	setTileVisible(exit->visitablePos(), false);

	const auto heroes = heroesByStrength();
	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);

	ASSERT_EQ(probes.size(), 1);
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(heroes.back()->id));
	EXPECT_FALSE(probes.front()->asTask()->isObjectAffected(heroes.front()->id));
}

TEST_F(OneWayPortalBehaviorTest, ObservedEnemyTraversalDoesNotSuppressOwnScoutProbe)
{
	auto builder = makeOneWayPortalMap({100, 1});
	builder
		.playerActive(ENEMY)
		.hero({7, 14, 0}, HeroTypeID(2), ENEMY)
		.heroGarrison({{CreatureID::ARCHER, 1}});
	startWithMap(std::move(builder));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	auto * enemy = heroByOwner(ENEMY);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	ASSERT_NE(enemy, nullptr);
	preparePlanning();

	teleportHero(*enemy, *entrance, *exit);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 2);
	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);

	ASSERT_EQ(probes.size(), 1)
		<< "observing an enemy traversal must not consume the AI's own portal probe";
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(heroes.back()->id))
		<< "the AI must still send its own scout after observing the enemy";
}

TEST_F(OneWayPortalBehaviorTest, ExhaustedHeroAtEntranceDoesNotProduceRepeatedProbe)
{
	startWithMap(makeOneWayPortalMap({100}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	const auto heroes = heroesByStrength();
	ASSERT_NE(entrance, nullptr);
	ASSERT_EQ(heroes.size(), 1);
	auto * hero = heroes.front();
	moveToVisitable(*hero, entrance->visitablePos());
	preparePlanning();
	hero->setMovementPoints(0);

	EXPECT_TRUE(portalTasks(decomposePreparedPortalBehavior(), *entrance).empty())
		<< "an exhausted hero already at the entrance must wait for the next turn";
}

TEST_F(OneWayPortalBehaviorTest, TwoScoutsProduceOneTaskForTheNearestScout)
{
	startWithMap(makeOneWayPortalMap({200, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);
	preparePlanning();

	const auto heroes = heroesByStrength();
	std::vector<CGHeroInstance *> scouts;
	for(auto * hero : heroes)
	{
		if(getGateway().nullkiller->heroManager->getHeroRoleOrDefaultInefficient(hero) == NK2AI::HeroRole::SCOUT)
			scouts.push_back(hero);
	}
	ASSERT_EQ(scouts.size(), 2);

	const auto nearest = *std::ranges::min_element(scouts, [entrance](const auto * left, const auto * right)
	{
		return left->visitablePos().dist2d(entrance->visitablePos())
			< right->visitablePos().dist2d(entrance->visitablePos());
	});
	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);

	ASSERT_EQ(probes.size(), 1) << "one portal must not schedule multiple scouts";
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(nearest->id));
}

TEST_F(OneWayPortalBehaviorTest, SafeMainIsFallbackWhenNoScoutExists)
{
	startWithMap(makeOneWayPortalMap({100}));
	setAllTilesVisible(true);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 1);
	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);

	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);

	ASSERT_EQ(probes.size(), 1);
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(heroes.front()->id));
}

TEST_F(OneWayPortalBehaviorTest, MissionCriticalMainWithoutScoutIsProtected)
{
	startWithMap(makeOneWayPortalMap({500}));
	setAllTilesVisible(true);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 1);
	markMissionCritical(*heroes.front());
	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty());
}

TEST_F(OneWayPortalBehaviorTest, SoleNoTownMissionCriticalHeroCanUseOnlyProgress)
{
	startWithMap(makeOneWayPortalMap({100}, false));
	setAllTilesVisible(true);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 1);
	markMissionCritical(*heroes.front());
	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	ASSERT_NE(entrance, nullptr);

	const auto probes = portalTasks(decomposePortalBehavior(), *entrance);

	ASSERT_EQ(probes.size(), 1);
	EXPECT_TRUE(probes.front()->asTask()->isObjectAffected(heroes.front()->id));
}

TEST_F(OneWayPortalBehaviorTest, MainClearsGuardWithoutRepeatingPreviouslyProbedEntrance)
{
	auto builder = makeOneWayPortalMap({500, 1});
	builder.monster({18, 10, 0}, CreatureID::ARCHER, 1);
	startWithMap(std::move(builder));

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * guard = objectByType(Obj::MONSTER);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(guard, nullptr);
	moveToVisitable(*guard, entrance->visitablePos() + int3(1, 0, 0));
	recalculateGuardingPositions();
	setAllTilesVisible(true);

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 2);
	const auto assertSafeGuardTask = [&](const auto & goals)
	{
		const auto guardTasks = std::ranges::count_if(goals, [&](const auto & goal)
		{
			return goal->asTask()->isObjectAffected(guard->id)
				&& goal->asTask()->isObjectAffected(heroes.front()->id);
		});

		EXPECT_EQ(guardTasks, 1)
			<< "the main must receive one safe attack on the entrance guard";
		EXPECT_TRUE(portalTasks(goals, *entrance).empty())
			<< "clearing the guard must not send the main through the entrance";
		EXPECT_TRUE(std::ranges::none_of(goals, [&](const auto & goal)
		{
			return goal->asTask()->isObjectAffected(guard->id)
				&& goal->asTask()->isObjectAffected(heroes.back()->id);
		})) << "the scout must not be used to clear the entrance guard";
	};

	assertSafeGuardTask(decomposePortalBehavior());

	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(exit, nullptr);
	teleportHero(*heroes.back(), *entrance, *exit);
	advanceDay();

	const auto goalsAfterProbe = decomposePortalBehavior();
	assertSafeGuardTask(goalsAfterProbe);

	const auto guardTask = std::find_if(goalsAfterProbe.begin(), goalsAfterProbe.end(), [&](const auto & goal)
	{
		return goal->asTask()->isObjectAffected(guard->id)
			&& goal->asTask()->isObjectAffected(heroes.front()->id);
	});
	ASSERT_NE(guardTask, goalsAfterProbe.end());

	getClient().connectPortal(entrance->visitablePos(), exit->visitablePos());
	EXPECT_NO_THROW((*guardTask)->asTask()->accept(&getGateway()));
	EXPECT_FALSE(getClient().requested(entrance->visitablePos()))
		<< "clearing the guard must not move the main onto the portal entrance";
	EXPECT_NE(heroes.front()->visitablePos(), exit->visitablePos())
		<< "clearing the guard must not teleport the main";
}

TEST_F(OneWayPortalBehaviorTest, ActualRandomExitDrivesForcedLandingFight)
{
	auto builder = makeOneWayPortalMap({500, 1}, true, 2);
	builder.monster({30, 30, 0}, CreatureID::ARCHER, 100);
	startWithMap(std::move(builder));

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto exits = objectsByType(Obj::MONOLITH_ONE_WAY_EXIT);
	auto * guard = objectByType(Obj::MONSTER);
	ASSERT_NE(entrance, nullptr);
	ASSERT_EQ(exits.size(), 2);
	ASSERT_NE(guard, nullptr);

	auto * actualExit = exits.back();
	moveToVisitable(*actualExit, {0, 0, 0});
	moveToVisitable(*guard, {1, 1, 0});
	recalculateGuardingPositions();
	setAllTilesVisible(true);
	preparePlanning();

	auto * scout = heroesByStrength().back();
	teleportHero(*scout, *entrance, *actualExit);
	const auto goals = decomposePortalBehavior();

	const auto forcedFight = std::ranges::find_if(goals, [&](const auto & goal)
	{
		return goal->asTask()->isObjectAffected(scout->id)
			&& goal->asTask()->isObjectAffected(guard->id);
	});
	ASSERT_NE(forcedFight, goals.end())
		<< "the forced action must use the guard at the actual server-selected exit";

	const auto evaluationContext = getGateway().nullkiller->priorityEvaluator->buildEvaluationContext(*forcedFight);
	EXPECT_EQ(
		getGateway().nullkiller->priorityEvaluator->evaluate(
			*forcedFight,
			NK2AI::PriorityEvaluator::PriorityTier::ESCAPE,
			evaluationContext),
		1.0f) << "the regular planner must prioritize the forced landing fight";
	EXPECT_TRUE(portalTasks(goals, *entrance).empty())
		<< "the old entrance plan must be invalid after the actual exit is observed";
}

TEST_F(OneWayPortalBehaviorTest, CompletedProbeWithoutReturnDoesNotScheduleSecondScout)
{
	startWithMap(makeOneWayPortalMap({500, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	const auto heroes = heroesByStrength();
	auto * scout = heroes.back();
	teleportHero(*scout, *entrance, *exit);
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty())
		<< "a completed probe without a return route must suppress additional scouts";
}

TEST_F(OneWayPortalBehaviorTest, StrongerMainReinforcesCompletedLandingForIsolatedEnemy)
{
	startWithMap(makeIsolatedEnemyPortalMap());
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	const auto heroes = heroesByStrength();
	ASSERT_EQ(heroes.size(), 2);
	auto * main = heroes.front();
	auto * scout = heroes.back();
	teleportHero(*scout, *entrance, *exit);
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty())
		<< "a hero still resolving the portal landing must suppress another expedition";

	moveHero(*scout, exit->visitablePos() + int3(1, 0, 0));
	const auto reinforcements = portalTasks(decomposePortalBehavior(), *entrance);
	ASSERT_EQ(reinforcements.size(), 1)
		<< "an isolated enemy objective must allow one stronger main-force follow-up";
	EXPECT_TRUE(reinforcements.front()->asTask()->isObjectAffected(main->id));
	EXPECT_FALSE(reinforcements.front()->asTask()->isObjectAffected(scout->id));

	teleportHero(*main, *entrance, *exit);
	moveHero(*main, exit->visitablePos() + int3(0, 1, 0));
	advanceDay();
	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty())
		<< "an expedition at the strongest available force must not cause portal flooding";
}

TEST_F(OneWayPortalBehaviorTest, FailedExitGuardAllowsStrongerMainRetry)
{
	startWithMap(makeOneWayPortalMap({500, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	const auto heroes = heroesByStrength();
	auto * failedScout = heroes.back();
	teleportHero(*failedScout, *entrance, *exit);
	getGateway().nullkiller->memory->recordOneWayPortalGuardFailure(
		entrance->id,
		1,
		failedScout->getTotalStrength());
	getGateway().nullkiller->memory->removeOneWayPortalHero(failedScout->id);
	advanceDay();

	const auto retries = portalTasks(decomposePortalBehavior(), *entrance);
	ASSERT_EQ(retries.size(), 1)
		<< "losing the exit fight must not permanently suppress a safe main-force retry";
	EXPECT_TRUE(retries.front()->asTask()->isObjectAffected(heroes.front()->id));
	EXPECT_FALSE(retries.front()->asTask()->isObjectAffected(heroes[1]->id));
	EXPECT_FALSE(retries.front()->asTask()->isObjectAffected(failedScout->id));
}

TEST_F(OneWayPortalBehaviorTest, FailedExitGuardRejectsUnsafeRetry)
{
	startWithMap(makeOneWayPortalMap({100, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	auto * failedScout = heroesByStrength().back();
	teleportHero(*failedScout, *entrance, *exit);
	getGateway().nullkiller->memory->recordOneWayPortalGuardFailure(
		entrance->id,
		std::numeric_limits<uint64_t>::max() / 2,
		failedScout->getTotalStrength());
	getGateway().nullkiller->memory->removeOneWayPortalHero(failedScout->id);
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty())
		<< "a failed probe must not feed another hero to an overwhelmingly strong exit guard";
}

TEST(OneWayPortalMemoryTest, GuardFailureRoundTripsAndLegacyStateDefaultsSafely)
{
	const ObjectInstanceID entrance(17);
	NK2AI::AIMemory saved;
	saved.recordOneWayPortalGuardFailure(entrance, 12345, 6789);

	JsonNode serialized;
	saved.saveOneWayPortalState(serialized);
	NK2AI::AIMemory loaded;
	loaded.loadOneWayPortalState(serialized);
	EXPECT_EQ(
		loaded.getOneWayPortalGuardFailure(entrance),
		std::make_optional(std::pair<uint64_t, uint64_t>(12345, 6789)));

	JsonNode legacy;
	legacy["probedEntrances"].Vector().emplace_back(entrance.getNum());
	loaded.loadOneWayPortalState(legacy);
	EXPECT_TRUE(loaded.wasOneWayPortalProbed(entrance));
	EXPECT_FALSE(loaded.getOneWayPortalGuardFailure(entrance).has_value());
}

TEST(OneWayPortalMemoryTest, CompletedLandingKeepsSerializedNoReturnHistory)
{
	const ObjectInstanceID entrance(17);
	const ObjectInstanceID exit(18);
	const ObjectInstanceID hero(19);
	NK2AI::AIMemory saved;
	saved.recordOneWayPortalTraversal(entrance, exit, hero, 3);
	ASSERT_TRUE(saved.hasActiveOneWayPortalJourney(entrance));
	ASSERT_TRUE(saved.completeOneWayPortalJourney(hero));
	EXPECT_FALSE(saved.hasActiveOneWayPortalJourney(entrance));
	EXPECT_EQ(saved.getUnreturnedOneWayPortalHeroes(entrance), std::vector{hero});

	JsonNode serialized;
	saved.saveOneWayPortalState(serialized);
	NK2AI::AIMemory loaded;
	loaded.loadOneWayPortalState(serialized);
	EXPECT_FALSE(loaded.hasActiveOneWayPortalJourney(entrance));
	EXPECT_EQ(loaded.getUnreturnedOneWayPortalHeroes(entrance), std::vector{hero});
}

TEST_F(OneWayPortalBehaviorTest, ReloadedProbeWithoutReturnDoesNotScheduleSecondScout)
{
	startWithMap(makeOneWayPortalMap({500, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	auto * scout = heroesByStrength().back();
	teleportHero(*scout, *entrance, *exit);
	finishTurn();
	advanceDay();
	restartGateway();
	setTileVisible(exit->visitablePos(), false);
	getGateway().tileHidden({exit->visitablePos()});
	getClient().connectPortal(entrance->visitablePos(), exit->visitablePos());

	getGateway().nullkiller->makeTurn();

	EXPECT_FALSE(getClient().requested(entrance->visitablePos()))
		<< "loading a save after a probe must not send another scout through the same portal";
}

TEST_F(OneWayPortalBehaviorTest, LegacySaveWithHeroBesideExitDoesNotScheduleSecondScout)
{
	startWithMap(makeOneWayPortalMap({500, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);

	auto * scout = heroesByStrength().back();
	moveToVisitable(*scout, exit->visitablePos() + int3(0, 1, 0));
	restartGateway();
	setTileVisible(exit->visitablePos(), false);
	getGateway().tileHidden({exit->visitablePos()});
	getClient().connectPortal(entrance->visitablePos(), exit->visitablePos());

	getGateway().nullkiller->makeTurn();

	EXPECT_FALSE(getClient().requested(entrance->visitablePos()))
		<< "loading an older save with a scout beside the exit must not send another scout";
}

TEST_F(OneWayPortalBehaviorTest, PortalProbeIsNotStarvedByDistantTreasure)
{
	auto builder = makeOneWayPortalMap({1}, false);
	builder.resource({30, 30, 0}, GameResID::GOLD, 10000);
	startWithMap(std::move(builder));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();
	getClient().connectPortal(entrance->visitablePos(), exit->visitablePos());

	getGateway().nullkiller->makeTurn();

	EXPECT_TRUE(getClient().requested(entrance->visitablePos()))
		<< "a safe one-way portal probe must execute before a distant treasure consumes the hero's turn";
}

TEST_F(OneWayPortalBehaviorTest, HiddenEntranceDoesNotForgetCompletedProbe)
{
	startWithMap(makeOneWayPortalMap({500, 1, 1}));
	setAllTilesVisible(true);

	auto * entrance = objectByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	auto * exit = objectByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_NE(entrance, nullptr);
	ASSERT_NE(exit, nullptr);
	preparePlanning();

	auto * scout = heroesByStrength().back();
	teleportHero(*scout, *entrance, *exit);

	setAllTilesVisible(false);
	getGateway().tileHidden({entrance->visitablePos()});
	setAllTilesVisible(true);
	getGateway().tileRevealed({entrance->visitablePos()});
	advanceDay();

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrance).empty())
		<< "losing sight of a portal must not permit another scout without a return route";
}

TEST_F(OneWayPortalBehaviorTest, ReturningThroughPortalChainAllowsOneScoutOnNextTurn)
{
	auto builder = makeOneWayPortalMap({500, 1, 1});
	builder
		.oneWayPortalEntrance({24, 20, 0}, 1)
		.oneWayPortalExit({12, 20, 0}, 1);
	startWithMap(std::move(builder));
	setAllTilesVisible(true);

	const auto entrances = objectsByType(Obj::MONOLITH_ONE_WAY_ENTRANCE);
	const auto exits = objectsByType(Obj::MONOLITH_ONE_WAY_EXIT);
	ASSERT_EQ(entrances.size(), 2);
	ASSERT_EQ(exits.size(), 2);
	preparePlanning();

	auto * firstScout = heroesByStrength().back();
	teleportHero(*firstScout, *entrances.front(), *exits.front());
	teleportHero(*firstScout, *entrances.back(), *exits.back());
	returnHeroToTown(*firstScout);

	EXPECT_TRUE(portalTasks(decomposePortalBehavior(), *entrances.front()).empty())
		<< "one entrance must not be traversed twice in the same turn";

	advanceDay();
	EXPECT_EQ(portalTasks(decomposePortalBehavior(), *entrances.front()).size(), 1)
		<< "a scout that returned through a portal chain must reopen every entrance in that chain";
}
