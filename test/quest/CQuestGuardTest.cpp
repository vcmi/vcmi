/*
 * CQuestGuardTest.cpp, part of VCMI engine
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

#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/CQuest.h"

// Quest guard behaviour as seen from the adventure map: blocking pathfinding,
// accepting payment, and getting torn down.

using namespace QuestScenarios;

class QuestGuardTest : public QuestTest {};

TEST_F(QuestGuardTest, PassableForColor_alwaysFalse)
{
	// A fresh quest guard blocks every colour — no hero can walk through it
	// before the quest is completed.
	auto s = questGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * guard = findObjectAt(s.questPos);
	ASSERT_NE(guard, nullptr);
	for(int c = 0; c < PlayerColor::PLAYER_LIMIT_I; ++c)
		EXPECT_FALSE(guard->passableFor(PlayerColor(c)))
			<< "quest guard should block colour " << c << " before completion";
}

TEST_F(QuestGuardTest, BlockVisitIsTrue_cannotBeTriggeredFromOnTop)
{
	// A hero stepping on the guard tile gets the quest dialog instead of
	// silently triggering it; this asserts the underlying "blocked visitable"
	// flag the pathfinder consults.
	auto s = questGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	const auto * guard = findObjectAt(s.questPos);
	ASSERT_NE(guard, nullptr);
	EXPECT_TRUE(guard->isBlockedVisitable());
}

TEST_F(QuestGuardTest, BringResources_takesResources)
{
	// Paying a "bring 1000 wood" guard deducts exactly 1000 wood from the
	// player's stockpile.
	auto s = questGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	grantResources(PlayerColor(0), GameResID(GameResID::WOOD), 1500);
	const auto woodBefore = gameState->players.at(PlayerColor(0)).resources[GameResID::WOOD];
	ASSERT_GE(woodBefore, 1000) << "scenario must leave hero with at least the demanded amount";

	auto * hero  = findHeroAt(s.heroPos);
	auto * guard = findObjectAt(s.questPos);
	ASSERT_NE(hero,  nullptr);
	ASSERT_NE(guard, nullptr);

	visit(hero, guard);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u)
		<< "first visit with a satisfied limiter should prompt the player";

	answerDialog(hero, /*yes*/ 1);

	const auto woodAfter = gameState->players.at(PlayerColor(0)).resources[GameResID::WOOD];
	EXPECT_EQ(woodBefore - woodAfter, 1000)
		<< "quest should deduct exactly the demanded 1000 wood";
}

TEST_F(QuestGuardTest, SatisfiedQuest_removesObjectOnAcceptance)
{
	// After the player pays a quest guard, the guard disappears from the map
	// so the hero can pass through.
	auto s = questGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	grantResources(PlayerColor(0), GameResID(GameResID::WOOD), 1500);

	auto * hero  = findHeroAt(s.heroPos);
	auto * guard = findObjectAt(s.questPos);
	ASSERT_NE(hero,  nullptr);
	ASSERT_NE(guard, nullptr);

	visit(hero, guard);
	ASSERT_EQ(gameEventCallback->blockingDialogs.size(), 1u);
	answerDialog(hero, /*yes*/ 1);

	EXPECT_EQ(findObjectAt(s.questPos), nullptr)
		<< "quest guard with subID=0 should be removed from the map after acceptance";
}

TEST_F(QuestGuardTest, RepeatVisit_failedRequirements_showsNextVisitText)
{
	// When the player can't yet pay, a re-visit shows the "still need more"
	// message but doesn't re-add the quest to the log.
	auto s = questGuard();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	// Deliberately do not grant resources — limiter fails.

	auto * hero  = findHeroAt(s.heroPos);
	auto * guard = findObjectAt(s.questPos);
	ASSERT_NE(hero,  nullptr);
	ASSERT_NE(guard, nullptr);

	visit(hero, guard);
	const size_t firstAddQuests = gameEventCallback->addedQuests.size();
	const size_t firstWindows   = gameEventCallback->infoWindows.size();
	EXPECT_EQ(firstAddQuests, 1u);
	EXPECT_EQ(firstWindows,   1u);

	visit(hero, guard);
	EXPECT_EQ(gameEventCallback->addedQuests.size(), firstAddQuests)
		<< "second visit must not re-emit AddQuest";
	EXPECT_GT(gameEventCallback->infoWindows.size(), firstWindows)
		<< "second visit should produce its own next-visit info dialog";
	EXPECT_TRUE(gameEventCallback->blockingDialogs.empty())
		<< "unsatisfied limiter must not prompt for completion";
}
