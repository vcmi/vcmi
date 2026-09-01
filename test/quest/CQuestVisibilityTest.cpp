/*
 * CQuestVisibilityTest.cpp, part of VCMI engine
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
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/Quest.h"

// A quest source stays accessible to a player who holds it in their quest
// log even when it sits under fog of war, and its log entry is dropped when the
// source is removed.

using namespace QuestScenarios;

class QuestVisibilityTest : public QuestTest {};

TEST_F(QuestVisibilityTest, QuestSource_FogOfWarVisibility_questHolderRemainsAccessibleUnderFog)
{
	auto s = seerArmy();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = expectAt<SeerHut>(s.questPos);
	ASSERT_NE(hero, nullptr);

	// Hide the whole map for the player so visibility can only come from the override.
	const auto teamId = gameState()->players.at(PlayerColor(0)).team;
	for(auto & tile : gameState()->teams.at(teamId).fogOfWarMap)
		tile = 0;

	ASSERT_FALSE(gameState()->isVisibleFor(seer, PlayerColor(0)))
		<< "precondition: seer must be hidden under fog before the quest is logged";

	visit(hero, seer); // first visit registers the quest in player 0's log

	EXPECT_TRUE(gameState()->isVisibleFor(seer, PlayerColor(0)))
		<< "a quest source in the player's log must stay accessible under fog of war";
}

TEST_F(QuestVisibilityTest, QuestSource_RemovedSourceErasesQuestLogEntry)
{
	auto s = seerArmy();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	auto * hero = findHeroAt(s.heroPos);
	auto * seer = expectAt<SeerHut>(s.questPos);
	ASSERT_NE(hero, nullptr);

	visit(hero, seer);
	ASSERT_FALSE(gameState()->players.at(PlayerColor(0)).quests.empty())
		<< "precondition: visiting the seer must register a quest-log entry";

	gameEvents().removeObject(seer, PlayerColor(0));

	EXPECT_TRUE(gameState()->players.at(PlayerColor(0)).quests.empty())
		<< "removing the quest source must erase its quest-log entry";
}
