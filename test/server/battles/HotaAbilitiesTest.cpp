/*
 * HotaAbilitiesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/bonuses/BonusCustomTypes.h"

namespace
{

/// Damage from one automaton detonation
constexpr int64_t detonationDamage = 95;

}

/// Integration tests for HotA Lua abilities copied into vcmi-test
class HotaAbilitiesTest : public BattleTestFixture
{
public:
	static constexpr int32_t bigStack = 1000;

	/// Adds a stack with an inherited expert Runes skill
	CStack * addRuneBearer()
	{
		attackerSideHero->setSecSkillLevel(skillByName("vcmi-test:runes"), 3, ChangeValueMode::ABSOLUTE);
		return addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(leftHex), bigStack);
	}

	int runeLevelOf(const CStack * unit) const
	{
		return unit->valOfBonuses(static_cast<BonusType>(BonusTypeID::decode("RUNE_LEVEL_COUNTER")));
	}
};

//----------------------------------------------------------------------------------------------
// Detonation
//----------------------------------------------------------------------------------------------

class DetonationTest : public HotaAbilitiesTest
{
public:
	/// Adds an automaton and its attacker
	void setUpDetonation(int32_t automatonCount = 1)
	{
		startGame();
		startBattle();

		automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), automatonCount);
		killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	}

	/// Kills the automaton to trigger detonation
	void detonate()
	{
		ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
		ASSERT_FALSE(automaton->alive()) << "the automaton has to die for it to detonate";
	}

	CStack * automaton = nullptr;
	CStack * killer = nullptr;
};

/// Verifies per-target damage caps during detonation
TEST_F(DetonationTest, DamagesEveryAdjacentUnitAndCapsOnlyTheCappedOne)
{
	setUpDetonation();

	CStack * capped = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex + 1), bigStack);
	CStack * uncapped = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex - GameConstants::BFIELD_WIDTH), bigStack);

	beginCombat();

	const int64_t cappedBefore = capped->getAvailableHealth();
	const int64_t uncappedBefore = uncapped->getAvailableHealth();

	detonate();

	// The cap is 10% of one creature with 100 health.
	EXPECT_EQ(cappedBefore - capped->getAvailableHealth(), 10);
	EXPECT_EQ(uncappedBefore - uncapped->getAvailableHealth(), detonationDamage);
}

/// Verifies UNIT_DEATH dispatch for clones
TEST_F(DetonationTest, AnswersTheDeathOfAClone)
{
	setUpDetonation();

	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), bigStack);

	beginCombat();

	// Set clone state without applying the Clone spell.
	makeClone(automaton);

	const int64_t healthBefore = victim->getAvailableHealth();

	detonate();

	EXPECT_EQ(healthBefore - victim->getAvailableHealth(), detonationDamage);
}

/// Verifies a subsequent death-event batch from detonation damage
TEST_F(DetonationTest, SetsOffTheAutomatonNextToIt)
{
	// Three automatons make the first detonation lethal to the second.
	setUpDetonation(3);

	CStack * second = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex + 1), 1);
	CStack * bystander = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), bigStack);

	beginCombat();

	const int64_t healthBefore = bystander->getAvailableHealth();

	detonate();
	ASSERT_FALSE(second->alive()) << "the first blast has to kill the second automaton";

	// The bystander is adjacent only to the second automaton.
	EXPECT_EQ(healthBefore - bystander->getAvailableHealth(), detonationDamage);
}

/// Verifies rebirth priority before other handlers in the same death batch
TEST_F(DetonationTest, RunsAfterARebirthOfTheSameBatch)
{
	startGame();
	startBattle();

	automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex + 1), 10);
	// Dragon breath kills both targets in one attack.
	killer = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), bigStack);

	beginCombat();

	detonate();

	ASSERT_TRUE(phoenix->alive()) << "the phoenix has to come back for the ordering to show";
	EXPECT_LT(phoenix->getAvailableHealth(), phoenix->getCount() * phoenix->getMaxHealth())
		<< "the blast found the phoenix standing, so the rebirth of the batch ran before it";
}

//----------------------------------------------------------------------------------------------
// Devour corpses
//----------------------------------------------------------------------------------------------

TEST_F(HotaAbilitiesTest, DevouredCorpseGrantsOneExtraStrike)
{
	startGame();
	startBattle();

	CStack * devourer = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDevourer"), BattleHex(leftHex), bigStack);
	CStack * prey = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);

	beginCombat();

	const uint32_t preyID = prey->unitId();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_FALSE(prey->alive()) << "the attack did not create a corpse";
	ASSERT_EQ(devourer->getTotalAttacks(false), 1) << "an attack that walked nowhere ate nothing";

	ASSERT_TRUE(move(devourer, BattleHex(rightHex)));

	EXPECT_EQ(devourer->getTotalAttacks(false), 2);
	const CStack * corpse = battle()->getStack(preyID, false);
	ASSERT_NE(corpse, nullptr);
	EXPECT_TRUE(corpse->isGhost()) << "consumed corpse must be removed";
}

/// Verifies that only executed additional attacks consume banked strikes
TEST_F(HotaAbilitiesTest, StrikeIsSpentOnlyOnAnExtraAttack)
{
	startGame();
	startBattle();

	CStack * devourer = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDevourer"), BattleHex(leftHex), bigStack);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);
	CStack * next = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex + 1), bigStack);

	beginCombat();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_TRUE(move(devourer, BattleHex(rightHex)));
	ASSERT_EQ(devourer->getTotalAttacks(false), 2);

	// The dragons survive both attacks, consuming the banked strike.
	blockRetaliation(next);
	ASSERT_TRUE(attack(devourer, BattleHex(rightHex + 1)));

	EXPECT_EQ(devourer->getTotalAttacks(false), 1);
}

/// Verifies that movement handlers run before attack count calculation
TEST_F(HotaAbilitiesTest, CorpseDevouredOnTheWayInFeedsTheAttackItWalkedInto)
{
	startGame();
	startBattle();

	CStack * devourer = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDevourer"), BattleHex(leftHex), bigStack);
	CStack * prey = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);
	CStack * next = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex + 1), bigStack);

	beginCombat();

	const uint32_t preyID = prey->unitId();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_FALSE(prey->alive()) << "the attack did not create a corpse";
	ASSERT_EQ(devourer->getTotalAttacks(false), 1);

	blockRetaliation(next);

	// Move onto the corpse and attack the next stack in one action.
	ASSERT_TRUE(attackFrom(devourer, BattleHex(rightHex + 1), BattleHex(rightHex)));

	const CStack * corpse = battle()->getStack(preyID, false);
	ASSERT_NE(corpse, nullptr);
	EXPECT_TRUE(corpse->isGhost()) << "the corpse was eaten on the way in";
	EXPECT_EQ(devourer->getTotalAttacks(false), 1) << "and the strike it paid for was thrown in that same attack";
}

//----------------------------------------------------------------------------------------------
// Runes
//----------------------------------------------------------------------------------------------

TEST_F(HotaAbilitiesTest, DefendingGrantsThreeRuneLevels)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();

	beginCombat();

	const int attackBefore = bearer->getAttack(false);

	ASSERT_TRUE(defend(bearer));

	EXPECT_EQ(runeLevelOf(bearer), 3);
	EXPECT_EQ(bearer->getAttack(false) - attackBefore, 2);
}

TEST_F(HotaAbilitiesTest, AttackWithoutRetaliationGrantsOneRuneLevel)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	blockRetaliation(bearer);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));

	EXPECT_EQ(runeLevelOf(bearer), 1);
}

/// Verifies maximum rune gain per action instead of the sum of attack and retaliation gains
TEST_F(HotaAbilitiesTest, AnsweredAttackGrantsTwoRuneLevels)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));

	EXPECT_EQ(runeLevelOf(bearer), 2) << "retaliation has the larger gain within the same action";
}

TEST_F(HotaAbilitiesTest, HeroSpellGrantsTwoRuneLevels)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 9999;

	startBattle();

	// Give Runes to the target side to verify cross-side action participants.
	defenderSideHero->setSecSkillLevel(skillByName("vcmi-test:runes"), 3, ChangeValueMode::ABSOLUTE);
	CStack * bearer = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), bearer));

	EXPECT_EQ(runeLevelOf(bearer), 2);
}

TEST_F(HotaAbilitiesTest, SpellCastByAUnitGrantsNoRuneLevel)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	CStack * caster = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testJuggernaut"), BattleHex(rightHex), bigStack);

	beginCombat();

	const int64_t healthBefore = bearer->getAvailableHealth();

	ASSERT_TRUE(castAsUnit(caster, spellByName("vcmi-test:heatStroke"), BattleHex(leftHex)));
	ASSERT_LT(bearer->getAvailableHealth(), healthBefore) << "heat stroke did not damage the target";

	EXPECT_EQ(runeLevelOf(bearer), 0) << "the hit names a casting unit, and only a hero's spell counts";
}

//----------------------------------------------------------------------------------------------
// Heat stroke
//----------------------------------------------------------------------------------------------

namespace
{

struct HeatStrokeCase
{
	const char * name;
	BattleSide side;
	BattleHex aim;
	BattleHex victim;
	bool burns;
};

}

class HeatStrokeTest : public HotaAbilitiesTest
{
public:
	/// Origin for relative cone coordinates
	static const BattleHex origin;

	/// Returns damage to `victimHex` from a cast aimed at `aimHex`
	int64_t damageAt(BattleSide side, const BattleHex & aimHex, const BattleHex & victimHex)
	{
		startGame();
		startBattle();

		const BattleSide otherSide = side == BattleSide::ATTACKER ? BattleSide::DEFENDER : BattleSide::ATTACKER;

		juggernaut = addStack(side, creatureByName("vcmi-test:testJuggernaut"), origin, bigStack);
		CStack * victim = addStack(otherSide, creatureByName("vcmi-test:testDamageCapped"), victimHex, bigStack);

		beginCombat();

		const int64_t healthBefore = victim->getAvailableHealth();

		EXPECT_TRUE(castAsUnit(juggernaut, spellByName("vcmi-test:heatStroke"), aimHex));

		return healthBefore - victim->getAvailableHealth();
	}

	CStack * juggernaut = nullptr;
};

const BattleHex HeatStrokeTest::origin(leftHex);

class HeatStrokeConeTest : public HeatStrokeTest, public ::testing::WithParamInterface<HeatStrokeCase>
{
};

TEST_P(HeatStrokeConeTest, BurnsWhatIsInsideTheCone)
{
	const int64_t damage = damageAt(GetParam().side, GetParam().aim, GetParam().victim);

	if(GetParam().burns)
		EXPECT_GT(damage, 0);
	else
		EXPECT_EQ(damage, 0);
}

INSTANTIATE_TEST_SUITE_P(Cones, HeatStrokeConeTest, ::testing::Values(
	// The aimed hex is part of the cone.
	HeatStrokeCase{"aimedHexOnTheAttackerSide", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToEast(), HeatStrokeTest::origin.copyToEast(), true},

	// Measure from the footprint half facing the aim point.
	HeatStrokeCase{"twoHexesWestOfTheRearHalfOnTheAttackerSide", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToWest().copyToWest(), HeatStrokeTest::origin.copyToWest().copyToWest(), true},

	// Defender-side double-wide footprints extend east.
	HeatStrokeCase{"twoHexesEastOfTheRearHalfOnTheDefenderSide", BattleSide::DEFENDER,
		HeatStrokeTest::origin.copyToEast().copyToEast(), HeatStrokeTest::origin.copyToEast().copyToEast(), true},

	// Adjacent hex outside the 120-degree cone.
	HeatStrokeCase{"hexOutsideTheCone", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToEast(), HeatStrokeTest::origin.copyToNorthEast().copyToNorthWest(), false}
),
	[](const ::testing::TestParamInfo<HeatStrokeCase> & info) { return info.param.name; });

/// Verifies use of capped effective luck instead of raw bonus sum
TEST_F(HeatStrokeTest, LuckIsTakenCappedRatherThanAsTheSumOfItsBonuses)
{
	const BattleHex aim = origin.copyToEast();

	startGame();
	startBattle();

	juggernaut = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testJuggernaut"), origin, bigStack);
	// Prevent either result from killing the target.
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), aim, 100 * bigStack);

	// Equal attack and defence isolate creature damage.
	const int defenceGap = juggernaut->getAttack(false) - victim->getDefense(false);
	victim->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, defenceGap, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE)));

	// The raw value guarantees luck while the capped value does not.
	juggernaut->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OTHER, 24, BonusSourceID()));
	ASSERT_LT(juggernaut->luckVal(), 24) << "effective luck must be lower than the raw bonus sum";

	beginCombat();

	const int64_t healthBefore = victim->getAvailableHealth();

	ASSERT_TRUE(castAsUnit(juggernaut, spellByName("vcmi-test:heatStroke"), aim));

	const int64_t plainStrike = bigStack * juggernaut->getMinDamage(false);
	ASSERT_EQ(juggernaut->getMinDamage(false), juggernaut->getMaxDamage(false)) << "a flat range leaves nothing to roll";

	EXPECT_EQ(healthBefore - victim->getAvailableHealth(), plainStrike)
		<< "the raw sum would roll 24 dice out of 24 and double every strike";
}
