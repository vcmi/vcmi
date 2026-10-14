/*
 * DestructionTest.cpp, part of VCMI engine
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

/// One scenario: which destroyer attacks, how large the victim stack is, and how many of it are
/// left once the ability has taken its share.
struct DestructionCase
{
	const char * name;
	const char * destroyer;
	int32_t victimCount;
	int32_t expectedVictimCount;
};

}

/// Destruction kills creatures of the attacked stack outright, on top of the damage of the
/// attack itself - either a share of the stack or a fixed number of them. The victim is a black
/// dragon, whose health is far above what one attacker can deal, so the count that survives is
/// decided by the ability alone.
class DestructionTest : public BattleTestFixture, public ::testing::WithParamInterface<DestructionCase>
{
public:
	static constexpr int32_t destroyerCount = 1;
};

TEST_P(DestructionTest, killsExpectedCreatures)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * victim = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), scenario.victimCount);
	CStack * destroyer = addStack(BattleSide::DEFENDER, creatureByName(scenario.destroyer), BattleHex(rightHex), destroyerCount);
	ASSERT_NE(victim, nullptr);
	ASSERT_NE(destroyer, nullptr);

	blockRetaliation(destroyer);

	ASSERT_TRUE(attack(destroyer, BattleHex(leftHex)));

	EXPECT_EQ(victim->getCount(), scenario.expectedVictimCount) << scenario.name;
}

// One destroyer deals a handful of damage to a stack of 300-health dragons, which never kills one
// of them, so every creature missing afterwards was taken by the ability.
INSTANTIATE_TEST_SUITE_P(Scenarios, DestructionTest, ::testing::Values(
	DestructionCase{"killsShareOfTheStack", "vcmi-test:testDestroyerShare", 100, 80},
	DestructionCase{"killsFixedAmount",     "vcmi-test:testDestroyerCount", 100, 93},

	// a fifth of three creatures rounds down to none
	DestructionCase{"shareTooSmallForOneCreature", "vcmi-test:testDestroyerShare", 3, 3},

	// asking for more than the stack holds leaves it dead rather than negative
	DestructionCase{"fixedAmountLargerThanTheStack", "vcmi-test:testDestroyerCount", 5, 0}
),
	[](const ::testing::TestParamInfo<DestructionCase> & info) { return info.param.name; });
