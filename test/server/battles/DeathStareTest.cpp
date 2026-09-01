/*
 * DeathStareTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/bonuses/BonusParameters.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"

namespace
{

/// One scenario: who the gorgons stare at, and how many of them die per triggered stare.
struct DeathStareCase
{
	const char * name;
	int defendingCreature;
	uint32_t expectedKills;   ///< most one stare can kill; 0 when the target is immune to it
	std::vector<std::string> expectedLog;
};

}

/// Death stare kills creatures of the attacked stack outright: every gorgon of the attacking
/// stack rolls its own chance, and the total is capped at the share of the stack that could have
/// rolled it. This pins what the ability produces - the kills, the spell announcement that drives
/// the client animation, and the combat log.
class DeathStareTest : public BattleTestFixture, public ::testing::WithParamInterface<DeathStareCase>
{
public:
	/// Large enough that the per-creature roll and the cap on it are both exercised: at 10% each,
	/// this many gorgons kill up to ten creatures per stare.
	static constexpr int32_t gorgonCount = 100;
	/// Large enough to survive every attack of the run, so that no scenario ends early.
	static constexpr int32_t targetCount = 100000;
	/// How many attacks one scenario makes. The roll is biased against long streaks of failure,
	/// so a run this long always contains several triggers.
	static constexpr int attacks = 20;

	/// The gorgon is double wide and occupies this hex and the one behind it.
	static constexpr int gorgonHex = rightHex;
	static constexpr int targetHex = leftHex;
};

TEST_P(DeathStareTest, killsExpectedCreatures)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * target = addStack(BattleSide::ATTACKER, CreatureID(scenario.defendingCreature), BattleHex(targetHex), targetCount);
	CStack * gorgon = addStack(BattleSide::DEFENDER, creatureByName("core:mightyGorgon"), BattleHex(gorgonHex), gorgonCount);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(gorgon, nullptr);

	// retaliation would shrink the staring stack, and every gorgon in it rolls its own chance
	blockRetaliation(gorgon);

	for(int i = 0; i < attacks; ++i)
	{
		ASSERT_TRUE(target->alive()) << "target died on attack " << i;
		ASSERT_TRUE(attack(gorgon, BattleHex(targetHex)));
	}

	const auto casts = server.castsOf(SpellID::DEATH_STARE);
	ASSERT_FALSE(casts.empty()) << scenario.name << ": death stare never triggered";

	for(const auto & cast : casts)
	{
		// what the client turns into the death stare animation and sound
		EXPECT_FALSE(cast.announcement.castByHero) << scenario.name;
		EXPECT_FALSE(cast.announcement.activeCast) << scenario.name; // a passive ability, not an action
		EXPECT_EQ(cast.announcement.casterStack, static_cast<si32>(gorgon->unitId())) << scenario.name;
		EXPECT_TRUE(cast.announcement.resistedCres.empty()) << scenario.name;
		EXPECT_TRUE(cast.announcement.reflectedCres.empty()) << scenario.name;

		// the cap is what makes this exact rather than a distribution: at 10% each, this many
		// gorgons can never kill more than a tenth of their own number
		EXPECT_LE(cast.killed, scenario.expectedKills) << scenario.name;
		EXPECT_EQ(cast.damage, static_cast<int64_t>(cast.killed) * target->getMaxHealth()) << scenario.name;
	}

	// a hundred gorgons rolling their chance separately practically never all miss, so every
	// attack lands a stare. An immune target is announced and animated all the same - the roll
	// happens before the spell, which is what filters the target back out
	EXPECT_EQ(casts.size(), static_cast<size_t>(attacks)) << scenario.name;

	// how many die is a roll, so no single stare can be pinned down. What can is the cap, which
	// a run this long always reaches, so the heaviest stare of the run is the one measured here
	const auto & heaviest = *std::max_element(casts.begin(), casts.end(),
		[](const RecordedCast & left, const RecordedCast & right) { return left.killed < right.killed; });

	EXPECT_EQ(heaviest.killed, scenario.expectedKills) << scenario.name;
	EXPECT_EQ(heaviest.logLines, scenario.expectedLog) << scenario.name;

	if(scenario.expectedKills == 0)
		EXPECT_TRUE(heaviest.announcement.affectedCres.empty()) << scenario.name;
	else
		EXPECT_EQ(heaviest.announcement.affectedCres, std::vector<ui32>{target->unitId()}) << scenario.name;
}

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int skeleton = 56;   // undead, absolutely immune to the stare
constexpr int ironGolem = 33;  // non-living, likewise immune
}

// Every gorgon of the stack rolls its own 10% chance, and the total is capped at a tenth of the
// stack, so 100 gorgons kill at most 10 creatures per stare. The immune targets are here because
// the immunity lives in the spell's targetCondition, which a script dealing raw damage would skip -
// note that they still get the cast and its animation, they just survive it.
INSTANTIATE_TEST_SUITE_P(Scenarios, DeathStareTest, ::testing::Values(
	DeathStareCase{"livingTarget", pikeman,  10, {"10 Pikemen die under the terrible gaze of the Mighty Gorgons."}},
	DeathStareCase{"undeadTarget", skeleton,  0, {}},
	DeathStareCase{"golemTarget",  ironGolem, 0, {}}
),
	[](const ::testing::TestParamInfo<DeathStareCase> & info) { return info.param.name; });

/// The "commander" situation is not part of the death stare script - it comes from the patch core
/// stacks over it, so this covers both that ability and that patching a combat script works.
class DeathStareCommanderTest : public BattleTestFixture
{
public:
	/// Far more health than the bearer can chew through, so every creature missing afterwards was
	/// taken by the stare.
	static constexpr int32_t victimCount = 1000;
	static constexpr int32_t bearerCount = 10;
	/// Kills before the level ratio of the two stacks is applied.
	static constexpr int killsBeforeRatio = 14;

	static void giveCommanderStare(CStack * unit, int value)
	{
		const std::string scriptName = "deathStare";
		auto script = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "script", scriptName);
		ASSERT_TRUE(script.has_value());

		JsonNode parameters;
		parameters["situation"].String() = "commander";

		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_EVENT_TRIGGER, BonusSource::OTHER, value, BonusSourceID(), BonusSubtypeID(ScriptID(*script)));
		bonus->parameters = std::make_shared<BonusParameters>(parameters);

		unit->addNewBonus(bonus);
	}
};

TEST_F(DeathStareCommanderTest, KillsScaleWithTheLevelRatio)
{
	startGame();
	startBattle();

	CStack * victim = addStack(BattleSide::ATTACKER, creatureByName("core:titan"), BattleHex(leftHex), victimCount);
	CStack * bearer = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), bearerCount);
	ASSERT_NE(victim, nullptr);
	ASSERT_NE(bearer, nullptr);

	// a retaliating titan would wipe out the stack whose level the kills are scaled by
	blockRetaliation(bearer);

	giveCommanderStare(bearer, killsBeforeRatio);

	ASSERT_TRUE(attack(bearer, BattleHex(leftHex)));

	const auto casts = server.castsOf(SpellID::DEATH_STARE);
	ASSERT_EQ(casts.size(), 1u) << "the stare is not rolled for, so it lands on every attack";

	// nothing is rolled here - the patch kills a flat number, worth less against bigger creatures
	EXPECT_EQ(casts.front().killed, static_cast<uint32_t>(killsBeforeRatio * bearer->unitLevel() / victim->unitLevel()));
}
