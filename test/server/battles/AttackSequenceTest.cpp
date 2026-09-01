/*
 * AttackSequenceTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

/// Walk-and-attack, and the step that only a unit with RETURN_AFTER_STRIKE takes afterwards.
class AttackSequenceTest : public BattleTestFixture
{
public:
	static constexpr int32_t attackerCount = 100;
	static constexpr int32_t defenderCount = 1000;

	/// Hexes far enough apart that the attacker has to walk, and adjacent enough that it can.
	static constexpr int originHex = leftHex;
	static constexpr int attackFromHex = leftHex + 3;
	static constexpr int targetHex = leftHex + 4;

	/// Walks `creature` from `originHex` to `attackFromHex`, strikes the unit on `targetHex`, and
	/// answers where it ended up.
	BattleHex walkAndAttack(const CreatureID & creature)
	{
		startGame();
		startBattle();

		CStack * attacker = addStack(BattleSide::ATTACKER, creature, BattleHex(originHex), attackerCount);
		CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);
		EXPECT_NE(attacker, nullptr);
		EXPECT_NE(defender, nullptr);

		// the bonus stops the target retaliating, so the attacker survives to take the return step
		blockRetaliation(attacker);

		// the return step paths with the unit's remaining movement, which only exists once the
		// battle has actually started
		beginCombat();

		const int64_t healthBefore = defender->getAvailableHealth();

		EXPECT_TRUE(attackFrom(attacker, BattleHex(targetHex), BattleHex(attackFromHex)));
		EXPECT_LT(defender->getAvailableHealth(), healthBefore) << "attack dealt no damage";

		return attacker->getPosition();
	}
};

TEST_F(AttackSequenceTest, harpyReturnsToStartingHex)
{
	EXPECT_EQ(walkAndAttack(creatureByName("core:harpy")), BattleHex(originHex));
}

TEST_F(AttackSequenceTest, ordinaryAttackerStaysWhereItStruck)
{
	EXPECT_EQ(walkAndAttack(creatureByName("core:griffin")), BattleHex(attackFromHex));
}
