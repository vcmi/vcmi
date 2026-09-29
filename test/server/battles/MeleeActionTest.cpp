/*
 * MeleeActionTest.cpp, part of VCMI engine
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
constexpr int pikeman = 0; // walks 4 hexes
constexpr int harpy = 72; // flies 6 hexes, returns after striking
}

/// What a WALK_AND_ATTACK action does on the server: who strikes in which order, where the
/// attacker ends up, and what cancels or adds to the attack. Attackers and targets share one row.
class MeleeActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex attackerStart = BattleHex(4, 5);
	static inline const BattleHex attackFrom = BattleHex(7, 5);
	static inline const BattleHex targetHex = BattleHex(8, 5);
};

TEST_F(MeleeActionTest, targetRetaliatesAfterBeingHit)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto attackerHealth = attacker->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_LT(attacker->getAvailableHealth(), attackerHealth);
}

TEST_F(MeleeActionTest, targetWithFirstStrikeHitsBeforeAttacker)
{
	startGame();
	startBattle();

	// one attacker against a large stack: the strike that comes first kills it before it can hit
	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, 1);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, 100);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FIRST_STRIKE, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(BonusCustomSubtype::damageTypeAll)));

	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_FALSE(attacker->alive());
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
}

TEST_F(MeleeActionTest, returningAttackerFliesBackToStart)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(harpy), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackerStart);
}

TEST_F(MeleeActionTest, returningAttackerCanStayAfterStrike)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(harpy), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom, false)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackFrom);
}

TEST_F(MeleeActionTest, longWeaponStrikesOverFreeHexWithoutRetaliation)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), twoHexesAway, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));

	const auto attackerHealth = attacker->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getAvailableHealth(), attackerHealth);
	EXPECT_EQ(attacker->getPosition(), twoHexesAway);
}

TEST_F(MeleeActionTest, targetTwoHexesAwayIsRefusedWithoutLongWeapon)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), twoHexesAway, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	EXPECT_FALSE(attack(attacker, targetHex));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), twoHexesAway);
}

TEST_F(MeleeActionTest, quicksandOnTheWayCancelsAttack)
{
	startGame();
	startBattle();

	const BattleHex halfway = BattleHex(5, 5);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(3, 5), stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	addQuicksand(halfway);

	const auto targetHealth = target->getAvailableHealth();

	// stepping into a trap is a legal move that ends the action early
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom)));
	EXPECT_EQ(attacker->getPosition(), halfway);
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
}
