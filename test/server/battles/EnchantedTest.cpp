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
public:
	CStack * addEnchanter(const std::string & creature, BattleSide side, const BattleHex & hex)
	{
		CStack * stack = addStack(side, creatureByName(creature), hex, stackCount);
		EXPECT_NE(stack, nullptr);
		return stack;
	}
};

TEST_F(EnchantedTest, EnchantsItsBearerWhenTheBattleStarts)
{
	startGame();
	startBattle();

	CStack * enchanter = addEnchanter("vcmi-test:testEnchanter", BattleSide::ATTACKER, BattleHex(leftHex));
	ASSERT_NE(enchanter, nullptr);
	ASSERT_EQ(enchanter->getAttack(false), baseAttack);

	beginCombat();

	EXPECT_EQ(enchanter->getAttack(false), baseAttack + bloodlustAttack);
}

TEST_F(EnchantedTest, KeepsTheEnchantmentAsRoundsPass)
{
	startGame();
	startBattle();

	CStack * enchanter = addEnchanter("vcmi-test:testEnchanter", BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * opponent = addEnchanter("vcmi-test:testSoulStealer", BattleSide::DEFENDER, BattleHex(rightHex));
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

	CStack * enchanter = addEnchanter("vcmi-test:testMassEnchanter", BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * ally = addEnchanter("vcmi-test:testSoulStealer", BattleSide::ATTACKER, BattleHex(leftHex - 1));
	CStack * enemy = addEnchanter("vcmi-test:testSoulStealer", BattleSide::DEFENDER, BattleHex(rightHex));
	ASSERT_NE(enchanter, nullptr);
	ASSERT_NE(ally, nullptr);
	ASSERT_NE(enemy, nullptr);

	beginCombat();

	EXPECT_EQ(enchanter->getAttack(false), baseAttack + bloodlustAttack);
	EXPECT_EQ(ally->getAttack(false), baseAttack + bloodlustAttack);
	EXPECT_EQ(enemy->getAttack(false), baseAttack);
}
