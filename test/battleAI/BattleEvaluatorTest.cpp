/*
 * BattleEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../server/battles/BattleTestFixture.h"

#include "AI/BattleAI/BattleEvaluator.h"
#include "lib/GameLibrary.h"
#include "lib/ObstacleHandler.h"
#include "lib/battle/BattleAction.h"
#include "lib/battle/CObstacleInstance.h"
#include "lib/battle/CPlayerBattleCallback.h"
#include "lib/callback/CBattleCallback.h"
#include "server/CGameHandler.h"

namespace test
{
class BattleEvaluatorTest : public BattleTestFixture
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->stacks.clear();
	}

	BattleAction moveTowards(
		const CStack * stack,
		const BattleHexArray & movementTargets,
		const BattleHexArray & finalDestinationHexes)
	{
		std::shared_ptr<Environment> environment = gameHandler;
		auto callback = std::make_shared<CBattleCallback>(defenderSideHero->getOwner(), nullptr);
		callback->onBattleStarted(battle());

		auto hypotheticalBattle = std::make_shared<HypotheticBattle>(environment.get(), callback->getBattle(BattleID(0)));
		DamageCache damageCache;
		damageCache.buildDamageCache(hypotheticalBattle, BattleSide::DEFENDER);
		PotentialTargets targets(stack, damageCache, hypotheticalBattle);
		BattleEvaluator evaluator(environment, callback, hypotheticalBattle, damageCache, stack,
			defenderSideHero->getOwner(), BattleID(0), BattleSide::DEFENDER, 1.0f, 2);

		return evaluator.goTowardsNearest(stack, movementTargets, targets, finalDestinationHexes);
	}

	BattleAction moveTowards(const CStack * stack, const CStack * target)
	{
		auto primaryTargetHexes = target->getAttackableHexes(stack);
		return moveTowards(stack, primaryTargetHexes, primaryTargetHexes);
	}

	void addObstacle()
	{
		const auto * obstacleInfo = LIBRARY->obstacles()->getByName("core:12");
		ASSERT_NE(obstacleInfo, nullptr);

		auto obstacle = std::make_shared<CObstacleInstance>();
		obstacle->ID = obstacleInfo->obstacle.getNum(); // ObDtS03: the dead tree and rocks from the reported battle
		obstacle->pos = BattleHex(9, 6);
		battle()->obstacles.push_back(obstacle);
	}
};

TEST_F(BattleEvaluatorTest, AttacksSuitableTargetOnWayToReportedMovementWaypoint)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(1, 8), 30);
	auto * griffin = addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(7, 4), 1);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(14, 5), 3);

	addObstacle();

	BattleHex movementWaypoint(9, 7);
	BattleHexArray movementWaypoints{movementWaypoint};
	auto reachability = battle()->getReachability(behemoth);
	ASSERT_EQ(reachability.distances[movementWaypoint.toInt()], behemoth->getMovementRange(0));
	auto action = moveTowards(behemoth, movementWaypoints, elf->getAttackableHexes(behemoth));

	ASSERT_EQ(action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_EQ(action.target.size(), 2);
	EXPECT_EQ(action.target[1].hexValue, griffin->getPosition());
}

TEST_F(BattleEvaluatorTest, SkipsTargetIfAttackWouldDelayPrimaryTarget)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(2, 7), 30);
	addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(13, 1), 1);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(13, 5), 3);

	auto action = moveTowards(behemoth, elf);

	EXPECT_EQ(action.actionType, EActionType::WALK);
}

TEST_F(BattleEvaluatorTest, SkipsUnprofitableTargetWithoutDelayingPrimaryTarget)
{
	auto * elf = addStack(BattleSide::ATTACKER, creatureByName("core:woodElf"), BattleHex(1, 8), 30);
	addStack(BattleSide::ATTACKER, creatureByName("core:angel"), BattleHex(7, 4), 20);
	auto * behemoth = addStack(BattleSide::DEFENDER, creatureByName("core:behemoth"), BattleHex(14, 5), 3);

	addObstacle();

	BattleHexArray movementWaypoints{BattleHex(9, 7)};
	auto action = moveTowards(behemoth, movementWaypoints, elf->getAttackableHexes(behemoth));

	EXPECT_EQ(action.actionType, EActionType::WALK);
}
}
