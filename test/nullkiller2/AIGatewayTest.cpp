/*
 * AIGatewayTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
const PlayerColor PLAYER(0);

TinyH3M::TinyH3MBuilder makeUnreachableMovementMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2UnreachableMovement")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.terrain({30, 30, 0}, TerrainId::WATER);

	return builder;
}

class AIGatewayTest : public NullkillerTest
{
};
}

TEST_F(AIGatewayTest, reportsUnreachableMovementAsBlocked)
{
	startWithMap(makeUnreachableMovementMap());
	revealMap(PLAYER);

	auto * hero = findFirst<CGHeroInstance>();
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2500);

	const auto gateway = makeGateway(PLAYER);
	gateway->nullkiller->heroManager->update();
	NK2AI::PathfinderSettings settings;
	settings.useHeroChain = false;
	settings.useDimensionDoor = false;
	gateway->nullkiller->pathfinder->updatePaths(
		gateway->nullkiller->getHeroesForPathfinding(),
		settings);

	const int3 startingPosition = hero->visitablePos();
	const int3 unreachableWater(30, 30, 0);
	EXPECT_EQ(
		gateway->moveHeroToTile(unreachableWater, NK2AI::HeroPtr(hero, gateway->cc.get())),
		NK2AI::HeroMovementResult::BLOCKED);
	EXPECT_EQ(hero->visitablePos(), startingPosition);
}
