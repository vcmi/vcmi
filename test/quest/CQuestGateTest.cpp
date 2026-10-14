/*
 * CQuestGateTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/Quest.h"

// Quest Gate semantics. A Quest Gate stays on the map and is
// passable once its limiter is satisfied; a "toll" gate (consumable limiter)
// charges the cost on every passage and is never persistently completed.

using B = TinyH3M::TinyH3MBuilder;

namespace
{
const int3 kHeroPos(5, 5, 0);
const int3 kGatePos(8, 8, 0);

// HOTA map with one player, a town, a knight hero, and a Quest Gate
// (BORDER_GATE subID 1000) carrying the given mission.
B gateScenario(TinyH3M::Quest mission)
{
	B b(EMapFormat::HOTA);
	b.hotaVersion(3)
		.playerActive(PlayerColor(0))
		.randomTown(int3(2, 2, 0), PlayerColor(0))
		.hero(kHeroPos, HeroTypeID(6), PlayerColor(0)) // Christian (knight)
		.questGate(kGatePos, std::move(mission));
	return b;
}
}

// ---- Toll classification (pure unit, no map) --------------------------------

TEST(QuestGateToll, consumableLimiterIsTollPredicateIsNot)
{
	auto isToll = [](const std::function<void(Quest &)> & setup) {
		Quest q;
		setup(q);
		return q.isToll();
	};

	EXPECT_TRUE (isToll([](Quest & q){ q.mission.resources[GameResID::WOOD] = 10; }));
	EXPECT_TRUE (isToll([](Quest & q){ q.mission.artifacts.push_back(ArtifactID(0)); }));
	EXPECT_TRUE (isToll([](Quest & q){ q.mission.creatures.emplace_back(CreatureID(0), 1); }));

	EXPECT_FALSE(isToll([](Quest &){}));
	EXPECT_FALSE(isToll([](Quest & q){ q.mission.heroLevel = 5; }));
	EXPECT_FALSE(isToll([](Quest & q){ q.mission.requiredKeys.push_back(MapObjectSubID(0)); }));
}

// ---- Map-based behaviour -----------------------------------------------------

class QuestGateTest : public QuestTest {};

TEST_F(QuestGateTest, NonTollSatisfiedPassesAndStays)
{
	auto s = gateScenario(B::missionLevel(1)); // a level-1 hero satisfies it; non-toll
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero = findHeroAt(kHeroPos);
	auto * gate = expectAt<QuestGate>(kGatePos);
	ASSERT_NE(hero, nullptr);

	EXPECT_TRUE(gate->passableFor(hero));
	visit(hero, gate);
	visit(hero, gate);
	EXPECT_TRUE(gate->passableFor(hero)) << "non-toll gate remains passable after passing";
	EXPECT_NE(findObjectAt(kGatePos), nullptr) << "gate is never removed";
}

TEST_F(QuestGateTest, NonTollUnmatchedHeroBlocked)
{
	auto s = gateScenario(B::missionLevel(99)); // hero is level 1, cannot satisfy
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * gate = expectAt<QuestGate>(kGatePos);
	EXPECT_FALSE(gate->passableFor(findHeroAt(kHeroPos)));
}

TEST_F(QuestGateTest, TollPassableForHeroOverloadDifferentiatesPaying)
{
	auto s = gateScenario(B::missionResources({10, 0, 0, 0, 0, 0, 0})); // 10 wood toll
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero  = findHeroAt(kHeroPos);
	auto * gate  = expectAt<QuestGate>(kGatePos);
	auto & player = gameState()->players.at(PlayerColor(0));

	player.resources[GameResID::WOOD] = 5;
	EXPECT_FALSE(gate->passableFor(hero)) << "hero who cannot pay the toll is blocked";

	player.resources[GameResID::WOOD] = 30;
	EXPECT_TRUE(gate->passableFor(hero)) << "hero who can pay the toll may pass";
}

TEST_F(QuestGateTest, TollChargedEveryPassAndNeverCompleted)
{
	auto s = gateScenario(B::missionResources({10, 0, 0, 0, 0, 0, 0}));
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s)));

	auto * hero   = findHeroAt(kHeroPos);
	auto * gate   = expectAt<QuestGate>(kGatePos);
	auto & player = gameState()->players.at(PlayerColor(0));
	player.resources[GameResID::WOOD] = 30;

	visit(hero, gate);
	EXPECT_EQ(player.resources[GameResID::WOOD], 20) << "first passage takes the toll";
	visit(hero, gate);
	EXPECT_EQ(player.resources[GameResID::WOOD], 10) << "every passage takes the toll";

	EXPECT_NE(findObjectAt(kGatePos), nullptr) << "toll gate is never removed";
	EXPECT_FALSE(gate->getQuest().isCompleted) << "toll gate is never persistently completed";
}
