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

/// Damage one detonation of a single automaton deals, from the 90 + 5 * N of the ability.
constexpr int64_t detonationDamage = 95;

}

/// The abilities Horn of the Abyss implements in Lua, on the engine features they need. The scripts
/// themselves are the ones that mod ships, copied into the test fixtures so that a change here that
/// breaks them is noticed before the mod is.
class HotaAbilitiesTest : public BattleTestFixture
{
public:
	static constexpr int32_t bigStack = 1000;

	/// The runes of an expert hero, which every stack of its army inherits.
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

/// An automaton damages everything around it when it dies. The fixture creature carries the
/// ability from the start, where the mod arms it with a self-cast first.
class DetonationTest : public HotaAbilitiesTest
{
public:
	/// An automaton the scenario is about, and the stack that will kill it.
	void setUpDetonation(int32_t automatonCount = 1)
	{
		startGame();
		startBattle();

		automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), automatonCount);
		killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	}

	/// Kills the automaton, which is the only way to set it off.
	void detonate()
	{
		ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
		ASSERT_FALSE(automaton->alive()) << "the automaton has to die for it to detonate";
	}

	CStack * automaton = nullptr;
	CStack * killer = nullptr;
};

/// Every adjacent unit is damaged, and a target whose incoming damage is capped keeps that cap to
/// itself - it used to lower the blast for every target the loop reached after it.
TEST_F(DetonationTest, DamagesEveryAdjacentUnitAndCapsOnlyTheCappedOne)
{
	setUpDetonation();

	CStack * capped = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex + 1), bigStack);
	CStack * uncapped = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex - GameConstants::BFIELD_WIDTH), bigStack);

	beginCombat();

	const int64_t cappedBefore = capped->getAvailableHealth();
	const int64_t uncappedBefore = uncapped->getAvailableHealth();

	detonate();

	// the cap is 10% of a creature with 100 health
	EXPECT_EQ(cappedBefore - capped->getAvailableHealth(), 10);
	EXPECT_EQ(uncappedBefore - uncapped->getAvailableHealth(), detonationDamage);
}

/// What a clone leaves behind is not what set off the charge, so it detonates like anything else.
/// This is also the only scenario pinning that the death of a clone is announced at all - the
/// rebirth of a clone is refused by the script rather than by the engine.
TEST_F(DetonationTest, AnswersTheDeathOfAClone)
{
	setUpDetonation();

	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), bigStack);

	beginCombat();

	// marking the automaton a clone is the shortest way to the state a cloned one would be in
	makeClone(automaton);

	const int64_t healthBefore = victim->getAvailableHealth();

	detonate();

	EXPECT_EQ(healthBefore - victim->getAvailableHealth(), detonationDamage);
}

/// One blast kills the automaton beside it, which detonates in its turn - the deaths of a batch
/// are announced, and the deaths they cause are announced after them.
TEST_F(DetonationTest, SetsOffTheAutomatonNextToIt)
{
	// three of them, so that the 90 + 5 * N of the first blast is past the health of the second
	setUpDetonation(3);

	CStack * second = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex + 1), 1);
	CStack * bystander = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), bigStack);

	beginCombat();

	const int64_t healthBefore = bystander->getAvailableHealth();

	detonate();
	ASSERT_FALSE(second->alive()) << "the first blast has to kill the second automaton";

	// the bystander only touches the second automaton, so anything it lost came from the chain
	EXPECT_EQ(healthBefore - bystander->getAvailableHealth(), detonationDamage);
}

/// Deaths of one blow are answered as a batch ordered by priority, so a stack that comes back from
/// its own death is already standing when anything else reacting to that batch runs.
TEST_F(DetonationTest, RunsAfterARebirthOfTheSameBatch)
{
	startGame();
	startBattle();

	automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex + 1), 10);
	// breath reaches the hex behind the one it strikes, so one blow kills both
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

/// Every corpse the worm walks onto is one more blow of its next attack.
TEST_F(HotaAbilitiesTest, DevouredCorpseGrantsOneExtraStrike)
{
	startGame();
	startBattle();

	CStack * devourer = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDevourer"), BattleHex(leftHex), bigStack);
	CStack * prey = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);

	beginCombat();

	const uint32_t preyID = prey->unitId();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_FALSE(prey->alive()) << "a corpse is what the scenario is about";
	ASSERT_EQ(devourer->getTotalAttacks(false), 1) << "an attack that walked nowhere ate nothing";

	ASSERT_TRUE(move(devourer, BattleHex(rightHex)));

	EXPECT_EQ(devourer->getTotalAttacks(false), 2);
	const CStack * corpse = battle()->getStack(preyID, false);
	ASSERT_NE(corpse, nullptr);
	EXPECT_TRUE(corpse->isGhost()) << "the corpse was eaten rather than left to be resurrected";
}

/// Strikes are spent one per extra blow actually thrown, so a target that dies to the first blow
/// costs nothing.
TEST_F(HotaAbilitiesTest, StrikeIsSpentOnlyOnAnExtraBlow)
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

	// the dragons survive every blow, so both of them are thrown and the banked one is spent
	blockRetaliation(next);
	ASSERT_TRUE(attack(devourer, BattleHex(rightHex + 1)));

	EXPECT_EQ(devourer->getTotalAttacks(false), 1);
}

/// A corpse eaten on the way into an attack pays for a blow of that same attack, rather than of
/// the next one - the walk of a walk-and-attack happens before the number of blows is settled.
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
	ASSERT_FALSE(prey->alive()) << "a corpse to walk onto is what the scenario is about";
	ASSERT_EQ(devourer->getTotalAttacks(false), 1);

	blockRetaliation(next);

	// walks onto the corpse and strikes the stack beyond it in one action
	ASSERT_TRUE(attackFrom(devourer, BattleHex(rightHex + 1), BattleHex(rightHex)));

	const CStack * corpse = battle()->getStack(preyID, false);
	ASSERT_NE(corpse, nullptr);
	EXPECT_TRUE(corpse->isGhost()) << "the corpse was eaten on the way in";
	EXPECT_EQ(devourer->getTotalAttacks(false), 1) << "and the strike it paid for was thrown in that same attack";
}

//----------------------------------------------------------------------------------------------
// Runes
//----------------------------------------------------------------------------------------------

/// Taking a defensive stance is an action that ends where it starts, so its levels are immediate.
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

/// An attack nobody answers is worth one level, handed out once the action it belongs to is over -
/// which is the same action, not the next one the unit takes.
TEST_F(HotaAbilitiesTest, UnansweredAttackGrantsOneRuneLevel)
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

/// A unit that strikes and is struck in return is credited for the blow it took, not for both -
/// which also pins that the whole action is granted once rather than blow by blow.
TEST_F(HotaAbilitiesTest, AnsweredAttackGrantsTwoRuneLevels)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));

	EXPECT_EQ(runeLevelOf(bearer), 2) << "the retaliation is part of the same action, and is worth more than the blow given";
}

/// A hero spell that damaged a unit is worth as much to it as a blow taken, and reaches it even
/// though the action is the other side's.
TEST_F(HotaAbilitiesTest, HeroSpellGrantsTwoRuneLevels)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 10, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 9999;

	startBattle();

	// the runes are the enemy hero's, so that the spell reaching them is an action of the other side
	defenderSideHero->setSecSkillLevel(skillByName("vcmi-test:runes"), 3, ChangeValueMode::ABSOLUTE);
	CStack * bearer = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), bearer));

	EXPECT_EQ(runeLevelOf(bearer), 2);
}

/// Only a hero's spell is worth anything, so the spell hit has to name the unit that cast it.
TEST_F(HotaAbilitiesTest, SpellCastByAUnitGrantsNoRuneLevel)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	CStack * caster = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testJuggernaut"), BattleHex(rightHex), bigStack);

	beginCombat();

	const int64_t healthBefore = bearer->getAvailableHealth();

	ASSERT_TRUE(castAsUnit(caster, spellByName("vcmi-test:heatStroke"), BattleHex(leftHex)));
	ASSERT_LT(bearer->getAvailableHealth(), healthBefore) << "the stroke has to reach the bearer";

	EXPECT_EQ(runeLevelOf(bearer), 0) << "the hit names a casting unit, and only a hero's spell counts";
}

//----------------------------------------------------------------------------------------------
// Heat stroke
//----------------------------------------------------------------------------------------------

namespace
{

/// One cone: the side the Juggernaut fights on, where it aims, and which hex is checked for burns.
struct HeatStrokeCase
{
	const char * name;
	BattleSide side;
	BattleHex aim;
	BattleHex victim;
	bool burns;
};

}

/// The stroke is a 120 degree cone two hexes deep: the aimed hex, the two hexes touching both the
/// Juggernaut and it, and the five - four, when it strikes straight up or down - one step further
/// out. It is measured from the half of the Juggernaut that faces the aim point, which is the hex
/// the unit stands on only when it fights on the attacker side.
class HeatStrokeTest : public HotaAbilitiesTest
{
public:
	/// The Juggernaut always stands here, and every hex of a scenario is given relative to it.
	static const BattleHex origin;

	/// Aims a stroke of a Juggernaut of the given side at `aimHex` and answers what the unit
	/// standing on `victimHex` lost to it.
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
	// the hex the stroke is aimed at is always in its own cone
	HeatStrokeCase{"aimedHexOnTheAttackerSide", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToEast(), HeatStrokeTest::origin.copyToEast(), true},

	// regression guard: the cone is measured from the half of the Juggernaut that faces the aim
	// point, not from the hex it stands on, which used to be reached with one step too many. An
	// attacker-side unit stands on the origin and covers the hex west of it as well
	HeatStrokeCase{"twoHexesWestOfTheRearHalfOnTheAttackerSide", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToWest().copyToWest(), HeatStrokeTest::origin.copyToWest().copyToWest(), true},

	// a defender-side Juggernaut carries its second hex to the east instead, so the same
	// measurement runs the other way
	HeatStrokeCase{"twoHexesEastOfTheRearHalfOnTheDefenderSide", BattleSide::DEFENDER,
		HeatStrokeTest::origin.copyToEast().copyToEast(), HeatStrokeTest::origin.copyToEast().copyToEast(), true},

	// a hex touching the cone but outside its 120 degrees stays cold
	HeatStrokeCase{"hexOutsideTheCone", BattleSide::ATTACKER,
		HeatStrokeTest::origin.copyToEast(), HeatStrokeTest::origin.copyToNorthEast().copyToNorthWest(), false}
),
	[](const ::testing::TestParamInfo<HeatStrokeCase> & info) { return info.param.name; });

/// The stroke rolls for a lucky strike on the luck the engine answers with, which is capped, rather
/// than on the sum of the bonuses granting it - a sum this large would double every single strike.
TEST_F(HeatStrokeTest, LuckIsTakenCappedRatherThanAsTheSumOfItsBonuses)
{
	const BattleHex aim = origin.copyToEast();

	startGame();
	startBattle();

	juggernaut = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testJuggernaut"), origin, bigStack);
	// far more health than either reading of the luck could take away, so that the stack does not
	// die to both of them and hide the difference
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), aim, 100 * bigStack);

	// levelled up to the attack of the caster, so that the attack-against-defense factor is 1 and
	// the damage left is the flat damage of the stack itself
	const int defenceGap = juggernaut->getAttack(false) - victim->getDefense(false);
	victim->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, defenceGap, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE)));

	// past the cap by far, and exactly the number of dice the roll uses
	juggernaut->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OTHER, 24, BonusSourceID()));
	ASSERT_LT(juggernaut->luckVal(), 24) << "the cap is what makes the two readings differ";

	beginCombat();

	const int64_t healthBefore = victim->getAvailableHealth();

	ASSERT_TRUE(castAsUnit(juggernaut, spellByName("vcmi-test:heatStroke"), aim));

	const int64_t plainStrike = bigStack * juggernaut->getMinDamage(false);
	ASSERT_EQ(juggernaut->getMinDamage(false), juggernaut->getMaxDamage(false)) << "a flat range leaves nothing to roll";

	EXPECT_EQ(healthBefore - victim->getAvailableHealth(), plainStrike)
		<< "the raw sum would roll 24 dice out of 24 and double every strike";
}
