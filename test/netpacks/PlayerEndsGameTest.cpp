/*
 * PlayerEndsGameTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "lib/CPlayerState.h"
#include "lib/gameState/CGameState.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/serializer/CMemorySerializer.h"

namespace test
{

TEST(PlayerEndsGameTest, SerializesResumeMarker)
{
	PlayerEndsGame gameEnd;
	gameEnd.resumeGameEnd = true;

	auto copy = CMemorySerializer::deepCopy(gameEnd);

	ASSERT_NE(copy, nullptr);
	EXPECT_TRUE(copy->resumeGameEnd);
}

TEST(PlayerEndsGameTest, DefaultsResumeMarkerForOlderVersion)
{
	PlayerEndsGame gameEnd;
	gameEnd.resumeGameEnd = true;

	CMemorySerializer memory;
	memory.oser.version = ESerializationVersion::SCENARIO_EVENT_JOURNAL;
	memory.iser.version = ESerializationVersion::SCENARIO_EVENT_JOURNAL;
	memory.oser & gameEnd;

	PlayerEndsGame copy;
	memory.iser & copy;

	EXPECT_FALSE(copy.resumeGameEnd);
}

TEST(PlayerEndsGameTest, ResumeMarkerDoesNotApplyVictoryTwice)
{
	CGameState gameState;
	const PlayerColor player(0);
	auto [playerState, inserted] = gameState.players.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(player),
		std::forward_as_tuple(&gameState));
	ASSERT_TRUE(inserted);
	playerState->second.status = EPlayerStatus::WINNER;

	PlayerEndsGame gameEnd;
	gameEnd.player = player;
	gameEnd.resumeGameEnd = true;
	gameState.apply(gameEnd);

	EXPECT_EQ(playerState->second.status, EPlayerStatus::WINNER);
}

}
