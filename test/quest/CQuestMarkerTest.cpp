/*
 * CQuestMarkerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestScenarios.h"
#include "QuestTest.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/Quest.h"

// The quest-log minimap marks the source object plus, by inspecting the
// limiter, kill targets / matching keymaster tents / heroes that satisfy it.

using namespace QuestScenarios;

namespace
{
bool hasTile(const std::vector<int3> & tiles, const int3 & t)
{
	return std::find(tiles.begin(), tiles.end(), t) != tiles.end();
}
}

class QuestMarkerTest : public QuestTest {};

TEST_F(QuestMarkerTest, Source_isAlwaysMarked)
{
	auto s = seerArmy();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	auto * seer = expectAt<SeerHut>(s.questPos);

	auto tiles = QuestInfo(seer->id).getMarkerTiles(gameState().get());

	EXPECT_TRUE(hasTile(tiles, seer->visitablePos())) << "the quest source must always be marked";
}

TEST_F(QuestMarkerTest, KillCreature_marksTarget)
{
	auto s = seerKillCreature();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	auto * seer    = expectAt<SeerHut>(s.questPos);
	auto * monster = expectAt<CGCreature>(s.secondHeroPos);

	auto tiles = QuestInfo(seer->id).getMarkerTiles(gameState().get());

	EXPECT_TRUE(hasTile(tiles, seer->visitablePos()));
	EXPECT_TRUE(hasTile(tiles, monster->visitablePos())) << "kill target must be marked from destroyedObjects";
}

TEST_F(QuestMarkerTest, BringArtifact_marksHeroHoldingIt)
{
	auto s = seerArtifact(); // hero carries the required artifact in its backpack
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	auto * seer = expectAt<SeerHut>(s.questPos);
	auto * hero = findHeroAt(s.heroPos);
	ASSERT_NE(hero, nullptr);

	auto tiles = QuestInfo(seer->id).getMarkerTiles(gameState().get());

	EXPECT_TRUE(hasTile(tiles, hero->visitablePos()))
		<< "a hero already satisfying the limiter (carrying the artifact) must be marked";
}

TEST_F(QuestMarkerTest, Level_marksQualifyingHeroOnly)
{
	auto s = seerLevel(); // hero ~level 5: passes the easy seer (>=3), fails the hard one (>=10)
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	auto * easySeer = expectAt<SeerHut>(s.questPos);
	auto * hardSeer = expectAt<SeerHut>(s.questPos2);
	auto * hero = findHeroAt(s.heroPos);
	ASSERT_NE(hero, nullptr);

	auto easyTiles = QuestInfo(easySeer->id).getMarkerTiles(gameState().get());
	auto hardTiles = QuestInfo(hardSeer->id).getMarkerTiles(gameState().get());

	EXPECT_TRUE(hasTile(easyTiles, hero->visitablePos())) << "hero meets the easy level requirement";
	EXPECT_FALSE(hasTile(hardTiles, hero->visitablePos())) << "hero does not meet the hard level requirement";
}

TEST_F(QuestMarkerTest, BorderGuard_marksMatchingKeymaster)
{
	auto s = questBorderGuard(); // questPos: keymaster (colour 0), questPos2: border guard
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	auto * guard     = findObjectAt(s.questPos2);
	auto * keymaster = findObjectAt(s.questPos);
	ASSERT_NE(guard,     nullptr);
	ASSERT_NE(keymaster, nullptr);

	auto tiles = QuestInfo(guard->id).getMarkerTiles(gameState().get());

	EXPECT_TRUE(hasTile(tiles, guard->visitablePos()));
	EXPECT_TRUE(hasTile(tiles, keymaster->visitablePos()))
		<< "the matching keymaster tent must be marked for a border guard";
}
