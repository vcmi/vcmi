/*
 * CMapObjectRemovalTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../game/GameStateTest.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

class CMapObjectRemovalTest : public GameStateTest
{
};

TEST_F(CMapObjectRemovalTest, removingObjectClearsDuplicateTileReferences)
{
	startTestGame();

	auto heroes = map->getObjects<CGHeroInstance>();
	ASSERT_FALSE(heroes.empty());

	auto * hero = heroes.front();
	const auto heroPosition = hero->visitablePos();
	auto & heroTile = map->getTile(heroPosition);

	ASSERT_TRUE(vstd::contains(heroTile.visitableObjects, hero->id));
	ASSERT_TRUE(vstd::contains(heroTile.blockingObjects, hero->id));

	// Older saves can contain the same object more than once on a terrain tile.
	heroTile.visitableObjects.push_back(hero->id);
	heroTile.blockingObjects.push_back(hero->id);

	auto removedHero = map->eraseObject(hero->id);
	ASSERT_EQ(removedHero.get(), hero);
	ASSERT_EQ(map->getObject(hero->id), nullptr);

	EXPECT_EQ(map->guardingCreaturePosition(heroPosition), int3(-1, -1, -1));
	EXPECT_FALSE(vstd::contains(heroTile.visitableObjects, hero->id));
	EXPECT_FALSE(vstd::contains(heroTile.blockingObjects, hero->id));
}
