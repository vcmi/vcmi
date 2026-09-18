/*
 * ClassicJoustingTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"

#include "../../AI/BattleAI/Classic/ClassicAttackEvaluator.h"
#include "../../AI/BattleAI/Classic/ClassicBattleRng.h"
#include "../../AI/BattleAI/Classic/ClassicCombatValue.h"
#include "../../AI/BattleAI/Classic/ClassicDecisionTrace.h"
#include "../../lib/battle/BattleAttackInfo.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/ReachabilityInfo.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../server/CGameHandler.h"
#include "../server/battles/BattleTestFixture.h"

namespace
{
class JoustingTestRng final : public IClassicBattleAIRng
{
public:
	int32_t nextIntInclusive(int32_t, int32_t upper) override
	{
		return upper;
	}
};
}

class ClassicJoustingTest : public BattleTestFixture
{
protected:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
		battle()->tacticDistance = 0;
		battle()->obstacles.clear();

		BattleUnitsChanged removal;
		removal.battleID = BattleID(0);
		for(const CStack * stack : battle()->battleGetAllStacks(false))
			removal.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(removal);
	}
};

TEST_F(ClassicJoustingTest, ImmediateChargeUsesActualPathLengthAroundObstacles)
{
	CStack * champion = addStack(BattleSide::ATTACKER, creatureByName("core:champion"), BattleHex(4, 4), 10);
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:archer"), BattleHex(8, 4), 1000);
	forceMaximumDamage(champion);
	blockRetaliation(target);
	champion->addNewBonus(std::make_shared<Bonus>(
		BonusDuration::ONE_BATTLE, BonusType::STACKS_SPEED, BonusSource::TERRAIN_NATIVE, 10, BonusSourceID()));

	// The only crossing is on the top row. Every attack landing on the far
	// side requires a detour, including both occupied cells of the Champion.
	auto barrier = std::make_shared<SpellCreatedObstacle>();
	barrier->passable = false;
	for(int row = 1; row < GameConstants::BFIELD_HEIGHT; ++row)
		barrier->customSize.insert(BattleHex(6, row));
	battle()->obstacles.push_back(barrier);
	battle()->nodeHasChanged();

	auto callback = std::shared_ptr<CBattleInfoCallback>(battle(), [](CBattleInfoCallback *) {});
	auto trace = std::make_shared<ClassicDecisionTrace>();
	ClassicCombatValue values(callback);
	ClassicAttackEvaluator evaluator(callback, std::make_shared<JoustingTestRng>(), trace);
	const ClassicScoredAction choice = evaluator.chooseAction(
		champion, values.buildParameters(BattleSide::ATTACKER, 4));
	ASSERT_TRUE(choice.valid);
	ASSERT_EQ(choice.action.actionType, EActionType::WALK_AND_ATTACK);
	ASSERT_GE(choice.action.target.size(), 2u);
	const BattleHex landing = choice.action.target.front().hexValue;
	ASSERT_TRUE(callback->isMeleeAttackPossible(champion, target, landing));

	// moveStack uses this same getPath result for the server's charge distance.
	const auto path = callback->getPath(champion->getPosition(), landing, champion);
	const int directDistance = BattleHex::getDistance(champion->getPosition(), landing);
	ASSERT_GT(path.second, directDistance);
	ASSERT_LE(path.second, champion->getMovementRange());
	EXPECT_EQ(path.second, callback->getReachability(champion).distances[landing.toInt()]);

	BattleAttackInfo actualCharge(champion, target, path.second, false);
	actualCharge.attackerPos = landing;
	BattleAttackInfo directCharge(champion, target, directDistance, false);
	directCharge.attackerPos = landing;
	const DamageEstimation expected = callback->battleEstimateDamage(actualCharge);
	ASSERT_EQ(expected.damage.min, expected.damage.max);
	ASSERT_LT(expected.damage.min, target->getAvailableHealth());
	ASSERT_GT(expected.damage.min, callback->battleEstimateDamage(directCharge).damage.min);

	const std::string key = std::to_string(champion->unitId()) + ":" + std::to_string(target->unitId());
	const auto firstStrike = std::ranges::find_if(trace->getEntries(), [&](const ClassicDecisionTraceEntry & entry)
	{
		return entry.stage == "attack_sim.first" && entry.key == key;
	});
	ASSERT_NE(firstStrike, trace->getEntries().end());
	EXPECT_EQ(firstStrike->value, expected.damage.min);
}
