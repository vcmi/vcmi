/*
 * WaterExplorationBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Behaviors/CaptureObjectsBehavior.h"
#include "AI/Nullkiller2/Behaviors/ExplorationBehavior.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/IClient.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/IObjectInterface.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER(0);

class BoatApplyingClient : public IClient
{
public:
	explicit BoatApplyingClient(std::shared_ptr<CGameState> gameState)
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
		if(dynamic_cast<const BuildBoat *>(&request))
		{
			++boatBuildRequests;
			return ++requestID;
		}

		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		if(!movement)
			return ++requestID;

		auto * hero = gameState->getHero(movement->hid);
		if(!hero || movement->path.empty())
			return ++requestID;

		for(const int3 & destination : movement->path)
		{
			const int3 destinationTile = hero->convertToVisitablePos(destination);

			TryMoveHero appliedMovement;
			appliedMovement.id = hero->id;
			appliedMovement.start = hero->pos;
			appliedMovement.end = destination;
			appliedMovement.movePoints = hero->movementPointsRemaining();
			appliedMovement.result = TryMoveHero::SUCCESS;
			gameState->apply(appliedMovement);

			if(capturedObject.hasValue() && destinationTile == capturePosition)
				gameState->getObjInstance(capturedObject)->setOwner(captureOwner);
			if(appliedMovement.movePoints == 0)
				break;
		}

		return ++requestID;
	}

	void captureOnVisit(const CGObjectInstance * object, PlayerColor owner)
	{
		capturedObject = object->id;
		capturePosition = object->visitablePos();
		captureOwner = owner;
	}

	int getBoatBuildRequests() const
	{
		return boatBuildRequests;
	}

private:
	std::shared_ptr<CGameState> gameState;
	int requestID = 0;
	ObjectInstanceID capturedObject;
	int3 capturePosition = int3(-1);
	PlayerColor captureOwner = PlayerColor::NEUTRAL;
	int boatBuildRequests = 0;
};

TinyH3M::TinyH3MBuilder makeVirtualBoatExplorationMap(PlayerColor shipyardOwner = PLAYER)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2VirtualBoatExploration")
		.playerActive(PLAYER)
		.hero({10, 22, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.shipyard({10, 18, 0}, shipyardOwner);

	for(int y = 0; y <= 17; ++y)
	{
		for(int x = 0; x < 36; ++x)
			builder.terrain({x, y, 0}, TerrainId::WATER);
	}

	return builder;
}

TinyH3M::TinyH3MBuilder makeExistingBoatExplorationMap(int boatCount = 1)
{
	auto builder = makeVirtualBoatExplorationMap();
	for(int i = 0; i < boatCount; ++i)
		builder.boat({12 + i * 2, 17, 0});
	return builder;
}

class WaterExplorationBehaviorTest : public NullkillerTest
{
public:
	void setTileVisible(const int3 & tile, bool visible)
	{
		ASSERT_TRUE(map()->isInTheMap(tile)) << tile.toString();
		gameState()->getPlayerTeam(PLAYER)->fogOfWarMap[tile] = visible ? 1 : 0;
	}

	void setResource(GameResID resource, int amount)
	{
		gameState()->players.at(PLAYER).resources[resource] = amount;
	}

	std::vector<CGHeroInstance *> heroesByStrength() const
	{
		auto result = findAll<CGHeroInstance>();
		std::erase_if(result, [](const CGHeroInstance * hero)
		{
			return hero->getOwner() != PLAYER;
		});
		std::ranges::sort(result, [](const CGHeroInstance * left, const CGHeroInstance * right)
		{
			return left->getTotalStrength() > right->getTotalStrength();
		});
		return result;
	}

	std::vector<CGObjectInstance *> objectsByType(MapObjectID type) const
	{
		std::vector<CGObjectInstance *> result;
		for(const auto & object : map()->objects)
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
		map()->moveObject(object.id, anchor);
		ASSERT_EQ(object.visitablePos(), destination);
	}

	void preparePlanning()
	{
		if(!gateway)
		{
			client = std::make_unique<BoatApplyingClient>(gameState());
			gateway = makeGateway(PLAYER, client.get());
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

	NK2AI::AIGateway & getGateway()
	{
		return *gateway;
	}

	BoatApplyingClient & getClient()
	{
		return *client;
	}

private:
	std::unique_ptr<BoatApplyingClient> client;
	std::unique_ptr<NK2AI::AIGateway> gateway;
};
}

TEST_F(WaterExplorationBehaviorTest, AvailableShipyardProducesVirtualBoatExplorationTask)
{
	startWithMap(makeVirtualBoatExplorationMap());
	setMapVisibility(PLAYER, true);

	const auto * shipyardObject = objectByType(Obj::SHIPYARD);
	const auto * shipyard = dynamic_cast<const IShipyard *>(shipyardObject);
	ASSERT_NE(shipyard, nullptr);
	ASSERT_EQ(shipyard->shipyardStatus(), IBoatGenerator::GOOD);
	const int3 boatPosition = shipyard->bestLocation();
	ASSERT_TRUE(boatPosition.isValid());
	setTileVisible(boatPosition + int3(0, -10, 0), false);

	preparePlanning();
	getGateway().nullkiller->memory->visitableObjs.erase(shipyardObject->id);
	const auto goals = NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get());
	const auto hasBoatConstructionRoute = std::ranges::any_of(goals, [&](const auto & goal)
	{
		const auto parts = goal->decompose(getGateway().nullkiller.get());
		const bool approachesShipyard = std::ranges::any_of(parts, [&](const auto & part)
		{
			const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get());
			return execute && execute->getPath().targetTile() == shipyardObject->visitablePos();
		});
		const bool buildsBoat = std::ranges::any_of(parts, [](const auto & part)
		{
			return part->goalType == NK2AI::Goals::BUILD_BOAT;
		});
		return approachesShipyard && buildsBoat;
	});
	const auto targetsLaunchTile = std::ranges::any_of(goals, [&](const auto & goal)
	{
		const auto parts = goal->decompose(getGateway().nullkiller.get());
		return std::ranges::any_of(parts, [&](const auto & part)
		{
			const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get());
			return execute && execute->getPath().targetTile() == boatPosition;
		});
	});

	EXPECT_TRUE(hasBoatConstructionRoute)
		<< "exploration across water must first approach the shipyard and build the boat";
	EXPECT_FALSE(targetsLaunchTile)
		<< "the empty launch tile is a prerequisite, not an exploration destination";
}

TEST_F(WaterExplorationBehaviorTest, FullyExploredWaterDoesNotProduceVirtualBoatTask)
{
	startWithMap(makeVirtualBoatExplorationMap());
	setMapVisibility(PLAYER, true);

	const auto * shipyardObject = objectByType(Obj::SHIPYARD);
	const auto * shipyard = dynamic_cast<const IShipyard *>(shipyardObject);
	ASSERT_NE(shipyard, nullptr);
	const int3 boatPosition = shipyard->bestLocation();
	ASSERT_TRUE(boatPosition.isValid());

	preparePlanning();
	getGateway().nullkiller->memory->visitableObjs.erase(shipyardObject->id);
	const auto goals = NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get());
	const auto hasLaunchTileRoute = std::ranges::any_of(goals, [&](const auto & goal)
	{
		const auto parts = goal->decompose(getGateway().nullkiller.get());
		return std::ranges::any_of(parts, [&](const auto & part)
		{
			const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get());
			return execute && execute->getPath().targetTile() == boatPosition;
		});
	});

	EXPECT_FALSE(hasLaunchTileRoute)
		<< "a fully explored coast must not create a zero-discovery movement loop";
}

TEST_F(WaterExplorationBehaviorTest, ExistingBoardableBoatSuppressesAdditionalConstruction)
{
	startWithMap(makeExistingBoatExplorationMap());
	setMapVisibility(PLAYER, true);

	const auto * shipyardObject = objectByType(Obj::SHIPYARD);
	const auto * shipyard = dynamic_cast<const IShipyard *>(shipyardObject);
	ASSERT_NE(shipyard, nullptr);
	ASSERT_EQ(shipyard->shipyardStatus(), IBoatGenerator::GOOD);
	setTileVisible(shipyard->bestLocation() + int3(0, -10, 0), false);

	preparePlanning();
	const auto goals = NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get());
	const int3 boatPosition = objectByType(Obj::BOAT)->visitablePos();
	const auto buildsBoat = std::ranges::any_of(goals, [&](const auto & goal)
	{
		const auto parts = goal->decompose(getGateway().nullkiller.get());
		return std::ranges::any_of(parts, [](const auto & part)
		{
			return part->goalType == NK2AI::Goals::BUILD_BOAT;
		});
	});

	EXPECT_FALSE(buildsBoat)
		<< "an existing boardable boat must satisfy the water-transport prerequisite";
	const auto targetsWaterFrontier = std::ranges::any_of(goals, [&](const auto & goal)
	{
		const auto parts = goal->decompose(getGateway().nullkiller.get());
		const bool explores = std::ranges::any_of(parts, [](const auto & part)
		{
			return part->goalType == NK2AI::Goals::EXPLORATION_POINT;
		});
		const bool movesBeyondBoat = std::ranges::any_of(parts, [&](const auto & part)
		{
			const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get());
			return execute && execute->getPath().targetTile() != boatPosition;
		});
		return explores && movesBeyondBoat;
	});
	EXPECT_TRUE(targetsWaterFrontier)
		<< "boarding must remain a prerequisite of a route to actual unexplored water";
}

TEST_F(WaterExplorationBehaviorTest, SailingHeroDoesNotTargetAnotherBoat)
{
	startWithMap(makeExistingBoatExplorationMap(2));
	setMapVisibility(PLAYER, true);
	preparePlanning();

	auto boats = objectsByType(Obj::BOAT);
	ASSERT_EQ(boats.size(), 2);
	auto * firstBoat = dynamic_cast<CGBoat *>(boats.front());
	ASSERT_NE(firstBoat, nullptr);
	auto * hero = heroesByStrength().front();
	moveToVisitable(*hero, firstBoat->visitablePos());
	hero->setBoat(firstBoat);
	preparePlanning();

	const auto paths = getGateway().nullkiller->pathfinder->getPathInfo(
		boats.back()->visitablePos(),
		getGateway().nullkiller->isObjectGraphAllowed());
	const auto goals = NK2AI::Goals::CaptureObjectsBehavior::getVisitGoals(
		paths,
		getGateway().nullkiller.get(),
		boats.back(),
		true);
	EXPECT_TRUE(std::ranges::all_of(goals, [](const auto & goal)
	{
		return goal->invalid();
	})) << "a hero already using water transport must not board a second boat";
}

TEST_F(WaterExplorationBehaviorTest, UncapturedShipyardProducesApproachTaskWhenBoatTileHasNoPath)
{
	startWithMap(makeVirtualBoatExplorationMap(PlayerColor::NEUTRAL));
	setMapVisibility(PLAYER, true);

	const auto * shipyardObject = objectByType(Obj::SHIPYARD);
	const auto * shipyard = dynamic_cast<const IShipyard *>(shipyardObject);
	ASSERT_NE(shipyard, nullptr);
	ASSERT_EQ(shipyard->shipyardStatus(), IBoatGenerator::GOOD);
	const int3 boatPosition = shipyard->bestLocation();
	setTileVisible(boatPosition + int3(0, -10, 0), false);

	preparePlanning();
	getClient().captureOnVisit(shipyardObject, PLAYER);
	const auto goals = NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get());
	auto findShipyardApproach = [&](const auto & candidates) -> NK2AI::Goals::TSubgoal
	{
		for(const auto & goal : candidates)
		{
			const auto parts = goal->decompose(getGateway().nullkiller.get());
			if(std::ranges::any_of(parts, [&](const auto & part)
			{
				const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get());
				return execute && execute->getPath().targetTile() == shipyardObject->visitablePos();
			}))
			{
				return goal;
			}
		}

		return {};
	};

	const auto shipyardApproach = findShipyardApproach(goals);
	ASSERT_TRUE(shipyardApproach)
		<< "a blocking shipyard must be approached before NK2 can build its virtual boat";
	EXPECT_NO_THROW(shipyardApproach->asTask()->accept(&getGateway()));
	EXPECT_EQ(heroesByStrength().front()->visitablePos(), shipyardObject->visitablePos());
	EXPECT_EQ(shipyardObject->getOwner(), PLAYER);
	EXPECT_EQ(getClient().getBoatBuildRequests(), 1)
		<< "the boat must be built before a later exploration pass can move the hero away";
	EXPECT_FALSE(findShipyardApproach(
		NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get())))
		<< "standing on the shipyard must not produce a same-turn visit loop";
}

TEST_F(WaterExplorationBehaviorTest, VirtualBoatReservesMissingResourcesOnlyOnce)
{
	startWithMap(makeVirtualBoatExplorationMap(PlayerColor::NEUTRAL));
	setMapVisibility(PLAYER, true);
	setResource(EGameResID::WOOD, 7);
	setResource(EGameResID::GOLD, 10000);

	const auto * shipyardObject = objectByType(Obj::SHIPYARD);
	const auto * shipyard = dynamic_cast<const IShipyard *>(shipyardObject);
	ASSERT_NE(shipyard, nullptr);
	setTileVisible(shipyard->bestLocation() + int3(0, -10, 0), false);

	preparePlanning();
	auto findReservation = [&](const auto & candidates) -> NK2AI::Goals::TSubgoal
	{
		for(const auto & goal : candidates)
		{
			const auto parts = goal->decompose(getGateway().nullkiller.get());
			const bool targetsShipyard = std::ranges::any_of(parts, [shipyardObject](const auto & part)
			{
				return part->objid == shipyardObject->id.getNum();
			});
			const bool savesResources = std::ranges::any_of(parts, [](const auto & part)
			{
				return part->goalType == NK2AI::Goals::SAVE_RESOURCES;
			});
			if(targetsShipyard && savesResources)
				return goal;
		}

		return {};
	};

	const auto reservation = findReservation(
		NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get()));
	ASSERT_TRUE(reservation);
	EXPECT_NO_THROW(reservation->asTask()->accept(&getGateway()));
	EXPECT_EQ(getGateway().nullkiller->getLockedResources()[EGameResID::WOOD], 10);
	EXPECT_FALSE(findReservation(
		NK2AI::Goals::ExplorationBehavior().decompose(getGateway().nullkiller.get())))
		<< "an existing boat reservation must not consume every planning pass";
}
