/*
 * PriorityEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/ArmyManager.h"
#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/Composition.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Markers/ExplorationPoint.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
const PlayerColor PLAYER(0);

TinyH3M::TinyH3MBuilder makePlannedArmyEvaluationMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2PlannedArmyEvaluation")
		.playerActive(PLAYER)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 10}})
		.hero({6, 5, 0}, HeroTypeID(1), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 50}})
		.hero({7, 5, 0}, HeroTypeID(2), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 40}})
		.town({10, 10, 0}, FactionID::CASTLE, PLAYER);

	return builder;
}

class PlannedArmyEvaluationTest : public NullkillerTest
{
};
}

TEST_F(PlannedArmyEvaluationTest, UsesPlannedArmyForGlobalLossBudget)
{
	startWithMap(makePlannedArmyEvaluationMap());

	const auto gateway = makeGateway(PLAYER);
	const auto * targetHero = findHeroAt({5, 5, 0});
	const auto * plannedArmy = findHeroAt({6, 5, 0});
	ASSERT_NE(targetHero, nullptr);
	ASSERT_NE(plannedArmy, nullptr);
	gateway->nullkiller->armyManager->update();

	NK2AI::AIPath path;
	path.targetHero = targetHero;
	path.heroArmy = plannedArmy;
	path.exchangeCount = 1;
	path.targetObjectDanger = 0;
	path.armyLoss = plannedArmy->getArmyStrength() * 2 / 5;
	path.targetObjectArmyLoss = 0;
	path.chainMask = 1;
	path.nodes.push_back({
		0.5f,
		0,
		{8, 5, 0},
		EPathfindingLayer::LAND,
		0,
		targetHero,
		-1,
		1,
		{},
		false});

	const auto task = sptr(NK2AI::Goals::Composition()
		.addNext(NK2AI::Goals::ExplorationPoint({8, 5, 0}, 80))
		.addNext(NK2AI::Goals::ExecuteHeroChain(path, nullptr)));
	auto context = gateway->nullkiller->priorityEvaluator->buildEvaluationContext(task);
	// This focused evaluator test does not initialize the danger hit map.
	context.enemyHeroDangerRatio = 0;

	EXPECT_NEAR(context.powerRatio, 0.5f, 0.001f);
	EXPECT_GT(gateway->nullkiller->priorityEvaluator->evaluate(
		task,
		NK2AI::PriorityEvaluator::EXPLORE_AND_GATHER,
		context), 0.0f);
}

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestBeatsOrdinaryHeroHunting)
{
	EXPECT_GT(NK2AI::evaluateEnemyTownConquestValue(0.8f, 3), 2.0f * 1.5f)
		<< "visible enemy towns should clearly outrank the capped strategic value of a loose enemy hero";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, lastVisibleEnemyTownGetsEliminationPressure)
{
	EXPECT_GT(
		NK2AI::evaluateEnemyTownConquestValue(1.0f, 1),
		NK2AI::evaluateEnemyTownConquestValue(1.5f, 3))
		<< "taking the last visible enemy town should outrank a normal enemy capital capture";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, nonEnemyTownKeepsBaseConquestValue)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateEnemyTownConquestValue(1.2f, 0),
		1.2f)
		<< "only visible enemy towns should get extra conquest pressure";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestCanRelaxArmyLossLimit)
{
	EXPECT_GT(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, true),
		0.39f)
		<< "enemy town conquest should not be rejected by the ordinary creeping loss cap";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, ordinaryTargetsKeepArmyLossLimit)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, false),
		0.35f)
		<< "the relaxed loss cap is only for enemy-town conquest";
}
