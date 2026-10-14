/*
 * AIUtilityTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/AIUtility.h"

#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/gameState/CGameState.h"
#include "lib/gameState/QuestInfo.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/Quest.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const int3 HERO_POS(5, 5, 0);
const int3 SEER_POS(7, 5, 0);

TinyH3M::TinyH3MBuilder makeQuestlessSeerMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.hotaVersion(3)
		.size(36, false)
		.name("NK2QuestlessSeer")
		.playerActive(PLAYER)
		.randomTown({2, 2, 0}, PLAYER)
		.hero(HERO_POS, HeroTypeID(0), PLAYER)
		.seerHutMulti(SEER_POS, {}, {});

	return builder;
}

class Nullkiller2_AIUtility : public NullkillerTest {};
}

TEST_F(Nullkiller2_AIUtility, trackedSeerWithoutActiveQuestIsNotVisitable)
{
	ASSERT_NO_FATAL_FAILURE(startWithMap(makeQuestlessSeerMap()));

	const auto * hero = findHeroAt(HERO_POS);
	const auto * seer = expectAt<SeerHut>(SEER_POS);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(seer->getActiveQuest(), nullptr);

	gameState()->players.at(PLAYER).quests.push_back(seer->getQuestIdentity());

	const auto callback = makeCallback(PLAYER);
	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), callback);

	EXPECT_FALSE(NK2AI::shouldVisit(gateway->nullkiller.get(), hero, seer));
}
