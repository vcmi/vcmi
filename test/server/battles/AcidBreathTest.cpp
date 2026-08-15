/*
 * AcidBreathTest.cpp, part of VCMI engine
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

/// One scenario: who the rust dragon hits, and how much acid damage that target takes.
struct AcidBreathCase
{
	const char * name;
	int defendingCreature;
	int64_t expectedDamage;
	const char * expectedLogLine;
};

}

/// Acid breath is the rust dragon's after-attack ability: every hit permanently lowers the
/// target's defence, and some hits also deal extra damage scaled by the size of the attacking
/// stack. This pins what the ability produces - the damage, the spell announcement that drives
/// the client animation, and the combat log.
class AcidBreathTest : public BattleTestFixture, public ::testing::WithParamInterface<AcidBreathCase>
{
public:
	static constexpr int32_t dragonCount = 10;
	/// Large enough to survive every attack of the run, so that no scenario ends early.
	static constexpr int32_t targetCount = 100000;
	/// How many attacks one scenario makes.
	static constexpr int attacks = 20;
	/// Defence the target loses on every hit, whether or not the damage half triggers.
	static constexpr int defenceLostPerHit = 3;

	/// The dragon is double wide and occupies this hex and the one behind it.
	static constexpr int dragonHex = rightHex;
	static constexpr int targetHex = leftHex;
};

TEST_P(AcidBreathTest, dealsExpectedDamage)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * target = addStack(BattleSide::ATTACKER, CreatureID(scenario.defendingCreature), BattleHex(targetHex), targetCount);
	CStack * dragon = addStack(BattleSide::DEFENDER, creatureByName("core:rustDragon"), BattleHex(dragonHex), dragonCount);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(dragon, nullptr);

	// retaliation would shrink the attacking stack, and the acid damage scales with its size
	blockRetaliation(dragon);

	const int defenceBefore = target->getDefense(false);
	int defenceAfterFirstHit = 0;

	for(int i = 0; i < attacks; ++i)
	{
		ASSERT_TRUE(target->alive()) << "target died on attack " << i;
		ASSERT_TRUE(attack(dragon, BattleHex(targetHex)));

		if(i == 0)
			defenceAfterFirstHit = target->getDefense(false);
	}

	// the defence reduction is the half of the ability that applies on every hit
	EXPECT_EQ(defenceAfterFirstHit, defenceBefore - defenceLostPerHit) << scenario.name;

	const auto casts = server.castsOf(SpellID::ACID_BREATH_DAMAGE);
	ASSERT_FALSE(casts.empty()) << scenario.name << ": acid breath never triggered";

	for(const auto & cast : casts)
	{
		EXPECT_EQ(cast.damage, scenario.expectedDamage) << scenario.name;

		// how much of the target the damage takes down depends on what earlier attacks left of it,
		// so only the damage line is the same every time. The line about the dead is there exactly
		// when the damage killed somebody, and starts with the newline that H3 puts in front of it
		ASSERT_FALSE(cast.logLines.empty()) << scenario.name;
		EXPECT_EQ(cast.logLines.front(), scenario.expectedLogLine) << scenario.name;
		EXPECT_EQ(cast.logLines.size(), cast.killed > 0 ? 2u : 1u) << scenario.name;

		if(cast.killed > 0)
        {
			EXPECT_EQ(cast.logLines.back().front(), '\n') << scenario.name;
        }

		// what the client turns into the acid animation and sound
		EXPECT_FALSE(cast.announcement.castByHero) << scenario.name;
		EXPECT_FALSE(cast.announcement.activeCast) << scenario.name; // a passive ability, not an action
		EXPECT_EQ(cast.announcement.casterStack, static_cast<si32>(dragon->unitId())) << scenario.name;
		EXPECT_EQ(cast.announcement.affectedCres, std::vector<ui32>{target->unitId()}) << scenario.name;
		EXPECT_TRUE(cast.announcement.resistedCres.empty()) << scenario.name;
		EXPECT_TRUE(cast.announcement.reflectedCres.empty()) << scenario.name;
	}

	// the damage half is chance-based, so it fires on some of the attacks but never on all of them.
	// The exact count is a property of the seed, which nothing about acid breath should have to pin
	EXPECT_LT(casts.size(), static_cast<size_t>(attacks)) << scenario.name;
}

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int ironGolem = 33;
constexpr int goldGolem = 116;
constexpr int diamondGolem = 117;
}

// The rust dragon deals 25 acid damage per creature in its stack, so 10 of them deal 250 to a
// target with no magic damage reduction. Golems reduce it by their own percentage, which is why
// they are here: the ability is cast as a spell, and a script dealing raw damage would skip that.
INSTANTIATE_TEST_SUITE_P(Scenarios, AcidBreathTest, ::testing::Values(
	AcidBreathCase{"plainTarget",  pikeman,      250, "The Acid breath does 250 damage."},
	AcidBreathCase{"ironGolem",    ironGolem,     62, "The Acid breath does 62 damage."},
	AcidBreathCase{"goldGolem",    goldGolem,     37, "The Acid breath does 37 damage."},
	AcidBreathCase{"diamondGolem", diamondGolem,  12, "The Acid breath does 12 damage."}
),
	[](const ::testing::TestParamInfo<AcidBreathCase> & info) { return info.param.name; });
