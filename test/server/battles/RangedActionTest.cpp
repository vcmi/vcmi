/*
 * RangedActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/bonuses/Bonus.h"

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int archer = 2; // one shot per turn
}

/// Actions a unit makes from where it stands: shooting, and what the first aid tent and the
/// catapult do.
class RangedActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex shooterHex = BattleHex(3, 5);
	static inline const BattleHex targetHex = BattleHex(12, 5);
};

TEST_F(RangedActionTest, shotAtMeleeUnitUsesOneShotWithoutRetaliation)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto shooterHealth = shooter->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();
	const auto shotsBefore = shooter->shots.available();

	ASSERT_TRUE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(shooter->getAvailableHealth(), shooterHealth);
	EXPECT_EQ(shooter->shots.available(), shotsBefore - 1);
}

TEST_F(RangedActionTest, shooterNextToEnemyCannotShoot)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), shooterHex.cloneInDirection(BattleHex::RIGHT), stackCount);

	const auto targetHealth = target->getAvailableHealth();
	const auto shotsBefore = shooter->shots.available();

	EXPECT_FALSE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(shooter->shots.available(), shotsBefore);
}

TEST_F(RangedActionTest, targetWithRangedRetaliationShootsBack)
{
	startGame();
	startBattle();

	CStack * shooter = addStack(BattleSide::ATTACKER, CreatureID(archer), shooterHex, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(archer), targetHex, stackCount);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::RANGED_RETALIATION, BonusSource::OTHER, 0, BonusSourceID()));

	const auto shooterHealth = shooter->getAvailableHealth();
	const auto targetShots = target->shots.available();

	ASSERT_TRUE(act(BattleAction::makeShotAttack(shooter, target)));
	EXPECT_LT(shooter->getAvailableHealth(), shooterHealth);
	EXPECT_EQ(target->shots.available(), targetShots - 1);
}

TEST_F(RangedActionTest, firstAidTentHealsWoundedStack)
{
	startGame();
	startBattle();

	CStack * tent = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::FIRST_AID_TENT), BattleHex(2, 8), 1);
	CStack * patient = addStack(BattleSide::ATTACKER, CreatureID(pikeman), shooterHex, stackCount);

	injure(patient, patient->getMaxHealth() / 2);
	const auto woundedHealth = patient->getAvailableHealth();
	ASSERT_LT(woundedHealth, patient->getMaxHealth() * stackCount);

	ASSERT_TRUE(act(BattleAction::makeHeal(tent, patient)));
	EXPECT_GT(patient->getAvailableHealth(), woundedHealth);
	EXPECT_LE(patient->getAvailableHealth(), patient->getMaxHealth() * stackCount);
}

TEST_F(RangedActionTest, catapultShootsAtWalls)
{
	startGame();
	startSiege();

	CStack * catapult = addStack(BattleSide::ATTACKER, CreatureID(CreatureID::CATAPULT), BattleHex(2, 8), 1);
	// the town has no garrison, and a side without units would lose the battle after the shot
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	BattleAction shot;
	shot.side = BattleSide::ATTACKER;
	shot.stackNumber = catapult->unitId();
	shot.actionType = EActionType::CATAPULT;
	shot.aimToHex(battle()->wallPartToBattleHex(EWallPart::UPPER_WALL));

	ASSERT_TRUE(act(shot));

	// the hero's own catapult takes its automatic turn right after, so only this one's shots count
	std::vector<CatapultAttack> shots;
	for(const auto & attack : server.catapultAttacks)
		if(attack.attacker == static_cast<int>(catapult->unitId()))
			shots.push_back(attack);

	// a catapult without Ballistics makes one shot, which may still miss the wall it aimed at
	ASSERT_EQ(shots.size(), 1u);
	EXPECT_NE(shots.front().attackedPart, EWallPart::INVALID);
}
