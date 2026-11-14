/*
 * AdventureCastActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"
#include "AI/Nullkiller2/Pathfinding/Actions/AdventureSpellCastMovementActions.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const int3 CURSED_TILE(5, 10, 0);
const int3 TARGET(16, 10, 0);
const int WATER_BEGIN_X = 8;
const int WATER_END_X = 14;

/// Hero on cursed ground west of a lake that only Fly can cross
TinyH3M::TinyH3MBuilder makeCursedShoreMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(PLAYER)
		.terrainOverlay(CURSED_TILE, Obj::CURSED_GROUND1)
		.hero(CURSED_TILE + int3(1, 0, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 10}})
		.heroSpells({SpellID(SpellID::decode("core:fly"))})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}});

	for(int y = 0; y < 36; ++y)
		for(int x = WATER_BEGIN_X; x <= WATER_END_X; ++x)
			builder.terrain({x, y, 0}, TerrainId::WATER);

	return builder;
}

class Nullkiller2_Pathfinding_AdventureCastAction : public NullkillerTest
{
};
}

TEST_F(Nullkiller2_Pathfinding_AdventureCastAction, heroOnCursedGroundCastsFlyAfterLeavingIt)
{
	startWithMap(makeCursedShoreMap());
	revealMap(PLAYER);

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), CURSED_TILE);
	hero->mana = 100;
	hero->setMovementPoints(2000);

	const auto gateway = makeGateway(PLAYER);
	NK2AI::HeroMap<NK2AI::HeroRole> heroes;
	heroes[hero] = NK2AI::HeroRole::MAIN;
	gateway->nullkiller->pathfinder->updatePaths(heroes, NK2AI::PathfinderSettings());

	const auto paths = gateway->nullkiller->pathfinder->getPathInfo(TARGET);
	ASSERT_FALSE(paths.empty()) << "the hero can walk off the cursed ground and cast Fly there";

	for(const auto & path : paths)
	{
		bool castsFly = false;
		for(size_t i = 0; i < path.nodes.size(); ++i)
		{
			if(!std::dynamic_pointer_cast<const NK2AI::AIPathfinding::AirWalkingAction>(path.nodes[i].specialAction))
				continue;

			castsFly = true;
			const int3 castTile = i + 1 < path.nodes.size() ? path.nodes[i + 1].coord : hero->visitablePos();
			EXPECT_NE(castTile, CURSED_TILE) << path.toString();
		}
		EXPECT_TRUE(castsFly) << path.toString();
	}
}
