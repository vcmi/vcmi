/*
 * ScriptApiBindingsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

/// The battle queries that take explicit hexes cannot be checked from C++ - what they have to get
/// right is the marshalling, and only a script goes through that. The probe script runs them all
/// and reports by damaging itself, one point for a clean sweep.
class ScriptApiBindingsTest : public BattleTestFixture
{
public:
	/// Right-hand edge of the battlefield, so that the script can walk twelve hexes west of itself
	/// and still land on the field.
	static constexpr int probeHex = 5 * GameConstants::BFIELD_WIDTH + 15;
	static constexpr int targetHex = probeHex - 1;
};

TEST_F(ScriptApiBindingsTest, PositionAwareQueriesAnswerAboutTheHexesTheyAreGiven)
{
	startGame();
	startBattle();

	CStack * probe = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testApiProbe"), BattleHex(probeHex), 10);
	// an archer, because the probe hates shooters and checks that the hate finds its subject
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(2), BattleHex(targetHex), 10);
	ASSERT_NE(probe, nullptr);
	ASSERT_NE(target, nullptr);

	ASSERT_TRUE(probe->hasBonusOfType(BonusType::PERCENTAGE_DAMAGE_BOOST)) << "the melee-only marker did not load";
	ASSERT_TRUE(probe->hasBonusOfType(BonusType::HATES_TRAIT)) << "the hate did not load";

	// retaliation would injure the probe as well, and its own health is how the script reports
	blockRetaliation(probe);

	const int64_t healthBefore = probe->getAvailableHealth();

	ASSERT_TRUE(attack(probe, BattleHex(targetHex)));

	const int64_t reported = healthBefore - probe->getAvailableHealth();

	ASSERT_NE(reported, 0) << "the probe script never ran";
	EXPECT_EQ(reported, 1) << "check number " << (reported - 1) << " of the probe script failed";
}
