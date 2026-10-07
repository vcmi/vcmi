/*
 * CompleteQuestTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/CompleteQuest.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/callback/CCallback.h"
#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);
const int3 HERO_POS(5, 5, 0);
const int3 GUARD_POS(20, 20, 0);
const int3 SEER_POS(10, 5, 0);

using B = TinyH3M::TinyH3MBuilder;

class Nullkiller2_Goals_CompleteQuest : public NullkillerTest
{
};
}

TEST_F(Nullkiller2_Goals_CompleteQuest, levelMissionVisitsObjectGrantingExperience)
{
	B builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(PLAYER)
		.playerActive(ENEMY)
		.hero(HERO_POS, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.hero({30, 30, 0}, HeroTypeID(1), ENEMY)
		.questGuard(GUARD_POS, B::missionLevel(5))
		.seerHut(SEER_POS, B::missionLevel(1), B::rewardExperience(20000));
	startWithMap(std::move(builder));
	revealMap(PLAYER);

	const auto * guard = findObjectAt(GUARD_POS);
	const auto * seer = findObjectAt(SEER_POS);
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(guard, nullptr);
	ASSERT_NE(seer, nullptr);
	ASSERT_NE(hero, nullptr);
	ASSERT_LT(hero->level, 5u);

	const auto gateway = makeGateway(PLAYER);
	gateway->nullkiller->memory->addVisitableObject(seer);
	NK2AI::HeroMap<NK2AI::HeroRole> heroes;
	heroes[hero] = NK2AI::HeroRole::MAIN;
	gateway->nullkiller->pathfinder->updatePaths(heroes, NK2AI::PathfinderSettings());

	const auto goals = NK2AI::Goals::CompleteQuest(QuestInfo(guard->id), *gateway->cc).decompose(gateway->nullkiller.get());
	EXPECT_TRUE(std::ranges::any_of(goals, [seer](const NK2AI::Goals::TSubgoal & goal){ return goal->objid == seer->id.getNum(); }))
		<< "a hero below the required level is sent to the seer hut that grants experience";
}
