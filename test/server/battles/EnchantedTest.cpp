/*
 * EnchantedTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

namespace
{

constexpr int32_t stackCount = 10;
/// Attack that the fixture creatures are declared with, before any enchantment.
constexpr int32_t baseAttack = 10;
/// What bloodlust adds at the mastery the fixture creatures cast it at.
constexpr int32_t bloodlustAttack = 3;

}

/// The enchanted ability keeps a spell applied to its bearer for the whole battle, re-applying it
/// at the start of every round. What it is worth is the stat the spell changes, so that is what
/// these check: the attack of a creature under a permanent bloodlust.
class EnchantedTest : public BattleTestFixture
{
};

TEST_F(EnchantedTest, EnchantsItsBearerWhenTheBattleStarts)
{
	startGame();
	startBattle();

	CStack * enchanter = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testEnchanter"), BattleHex(leftHex), stackCount);
	ASSERT_NE(enchanter, nullptr);
	ASSERT_EQ(enchanter->getAttack(false), baseAttack);

	beginCombat();

	EXPECT_EQ(enchanter->getAttack(false), baseAttack + bloodlustAttack);
}

TEST_F(EnchantedTest, KeepsTheEnchantmentAsRoundsPass)
{
	startGame();
	startBattle();

	CStack * enchanter = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testEnchanter"), BattleHex(leftHex), stackCount);
	CStack * opponent = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex), stackCount);
	ASSERT_NE(enchanter, nullptr);
	ASSERT_NE(opponent, nullptr);

	beginCombat();

	// the effect is granted for a limited number of turns, and would run out were it not renewed
	for(int round = 0; round < 3; ++round)
	{
		endRound();
		EXPECT_EQ(enchanter->getAttack(false), baseAttack + bloodlustAttack) << "round " << battle()->getRound();
	}
}

TEST_F(EnchantedTest, MassiveEnchantmentReachesEveryAllyAndNoEnemy)
{
	startGame();
	startBattle();

	CStack * enchanter = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testMassEnchanter"), BattleHex(leftHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(leftHex - 1), stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex), stackCount);
	ASSERT_NE(enchanter, nullptr);
	ASSERT_NE(ally, nullptr);
	ASSERT_NE(enemy, nullptr);

	beginCombat();

	EXPECT_EQ(enchanter->getAttack(false), baseAttack + bloodlustAttack);
	EXPECT_EQ(ally->getAttack(false), baseAttack + bloodlustAttack);
	EXPECT_EQ(enemy->getAttack(false), baseAttack);
}
