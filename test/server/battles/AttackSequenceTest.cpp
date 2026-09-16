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

namespace
{

/// One walk-and-attack: who walks, and where the attacker is left standing afterwards.
struct AttackSequenceCase
{
	const char * name;
	const char * creature;
	bool returns;
};

}

/// Walk-and-attack, and the step that only a unit with RETURN_AFTER_STRIKE takes afterwards.
class AttackSequenceTest : public BattleTestFixture, public ::testing::WithParamInterface<AttackSequenceCase>
{
public:
	static constexpr int32_t attackerCount = 100;
	static constexpr int32_t defenderCount = 1000;

	/// Hexes far enough apart that the attacker has to walk, and adjacent enough that it can.
	static constexpr int originHex = leftHex;
	static constexpr int attackFromHex = leftHex + 3;
	static constexpr int targetHex = leftHex + 4;
};

TEST_P(AttackSequenceTest, EndsWhereTheCreatureIsSupposedTo)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName(GetParam().creature), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);

	// the bonus stops the target retaliating, so the attacker survives to take the return step
	blockRetaliation(attacker);

	// the return step paths with the unit's remaining movement, which only exists once the
	// battle has actually started
	beginCombat();

	const int64_t healthBefore = defender->getAvailableHealth();

	ASSERT_TRUE(attackFrom(attacker, BattleHex(targetHex), BattleHex(attackFromHex)));
	ASSERT_LT(defender->getAvailableHealth(), healthBefore) << "attack dealt no damage";

	EXPECT_EQ(attacker->getPosition(), BattleHex(GetParam().returns ? originHex : attackFromHex));
}

INSTANTIATE_TEST_SUITE_P(Creatures, AttackSequenceTest, ::testing::Values(
	AttackSequenceCase{"harpyReturnsToStartingHex", "core:harpy", true},
	AttackSequenceCase{"griffinStaysWhereItStruck", "core:griffin", false}
),
	[](const ::testing::TestParamInfo<AttackSequenceCase> & info) { return info.param.name; });
