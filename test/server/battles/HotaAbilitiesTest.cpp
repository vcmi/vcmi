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

#include "../../../lib/GameLibrary.h"
#include "../../../lib/bonuses/BonusCustomTypes.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"

namespace
{

/// Damage one detonation of a single automaton deals, from the 90 + 5 * N of the ability.
constexpr int64_t detonationDamage = 95;

SpellID spellByName(const std::string & name)
{
	auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "spell", name);
	EXPECT_TRUE(identifier.has_value()) << "unknown spell " << name;

	return identifier ? SpellID(*identifier) : SpellID::NONE;
}

SecondarySkill skillByName(const std::string & name)
{
	auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "secondarySkill", name);
	EXPECT_TRUE(identifier.has_value()) << "unknown skill " << name;

	return identifier ? SecondarySkill(*identifier) : SecondarySkill::NONE;
}

}

/// The abilities Horn of the Abyss implements in Lua, on the engine features they need. The scripts
/// themselves are the ones that mod ships, copied into the test fixtures so that a change here that
/// breaks them is noticed before the mod is.
class HotaAbilitiesTest : public BattleTestFixture
{
public:
	static constexpr int32_t bigStack = 1000;

	int64_t healthOf(const CStack * unit) const { return unit->getAvailableHealth(); }

	/// The runes of an expert hero, which every stack of its army inherits.
	CStack * addRuneBearer()
	{
		attackerSideHero->setSecSkillLevel(skillByName("vcmi-test:runes"), 3, ChangeValueMode::ABSOLUTE);
		return addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(leftHex), bigStack);
	}

	int runeLevelOf(const CStack * unit) const
	{
		return unit->valOfBonuses(Selector::type()(static_cast<BonusType>(BonusTypeID::decode("RUNE_LEVEL_COUNTER"))));
	}
};

//----------------------------------------------------------------------------------------------
// Detonation
//----------------------------------------------------------------------------------------------

/// The automaton has to be told to detonate before it dies; arming it is a spell cast at itself.
TEST_F(HotaAbilitiesTest, DetonationDamagesEveryAdjacentUnit)
{
	startGame();
	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), bigStack);
	CStack * killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	ASSERT_NE(automaton, nullptr);
	ASSERT_NE(victim, nullptr);
	ASSERT_NE(killer, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsUnit(automaton, spellByName("abilityDetonation")));

	const int64_t healthBefore = healthOf(victim);

	ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
	ASSERT_FALSE(automaton->alive()) << "the automaton has to die for it to detonate";

	EXPECT_EQ(healthBefore - healthOf(victim), detonationDamage);
}

/// Regression guard: a target whose incoming damage is capped used to lower the blast for every
/// target the loop reached after it.
TEST_F(HotaAbilitiesTest, DetonationCapAppliesToTheCappedTargetAlone)
{
	startGame();
	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * capped = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex + 1), bigStack);
	CStack * uncapped = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex - GameConstants::BFIELD_WIDTH), bigStack);
	CStack * killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	ASSERT_NE(capped, nullptr);
	ASSERT_NE(uncapped, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsUnit(automaton, spellByName("abilityDetonation")));

	const int64_t cappedBefore = healthOf(capped);
	const int64_t uncappedBefore = healthOf(uncapped);

	ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
	ASSERT_FALSE(automaton->alive());

	// the cap is 10% of a creature with 100 health
	EXPECT_EQ(cappedBefore - healthOf(capped), 10);
	EXPECT_EQ(uncappedBefore - healthOf(uncapped), detonationDamage);
}

/// A clone leaves nothing behind that could explode.
TEST_F(HotaAbilitiesTest, DetonationIsNotInheritedByAClone)
{
	startGame();
	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), bigStack);
	CStack * killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);

	beginCombat();

	ASSERT_TRUE(castAsUnit(automaton, spellByName("abilityDetonation")));

	// the automaton itself is what the scenario kills; marking it a clone is the shortest way to
	// the state a cloned one would be in
	makeClone(automaton);

	const int64_t healthBefore = healthOf(victim);

	ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
	ASSERT_FALSE(automaton->alive());

	EXPECT_EQ(healthBefore - healthOf(victim), 0);
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
	ASSERT_NE(devourer, nullptr);
	ASSERT_NE(prey, nullptr);

	beginCombat();

	const uint32_t preyID = prey->unitId();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_FALSE(prey->alive()) << "a corpse is what the scenario is about";
	ASSERT_EQ(devourer->getTotalAttacks(false), 1);

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
	ASSERT_NE(next, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(devourer, BattleHex(rightHex)));
	ASSERT_TRUE(move(devourer, BattleHex(rightHex)));
	ASSERT_EQ(devourer->getTotalAttacks(false), 2);

	// the dragons survive every blow, so both of them are thrown and the banked one is spent
	blockRetaliation(next);
	ASSERT_TRUE(attack(devourer, BattleHex(rightHex + 1)));

	EXPECT_EQ(devourer->getTotalAttacks(false), 1);
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
	ASSERT_NE(bearer, nullptr);

	beginCombat();

	const int attackBefore = bearer->getAttack(false);

	ASSERT_TRUE(defend(bearer));

	EXPECT_EQ(runeLevelOf(bearer), 3);
	EXPECT_EQ(bearer->getAttack(false) - attackBefore, 2);
}

/// An attack nobody answers is worth one level, handed out once the action it belongs to is over.
TEST_F(HotaAbilitiesTest, UnansweredAttackGrantsOneRuneLevel)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	ASSERT_NE(target, nullptr);
	blockRetaliation(bearer);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));

	// moving is the next action of the bearer, which is what closes the one before it
	ASSERT_TRUE(move(bearer, BattleHex(leftHex - 1)));

	EXPECT_EQ(runeLevelOf(bearer), 1);
}

/// A unit that strikes and is struck in return is credited for the blow it took, not for both.
TEST_F(HotaAbilitiesTest, AnsweredAttackGrantsTwoRuneLevels)
{
	startGame();
	startBattle();

	CStack * bearer = addRuneBearer();
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	ASSERT_NE(target, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));
	ASSERT_TRUE(move(bearer, BattleHex(leftHex - 1)));

	EXPECT_EQ(runeLevelOf(bearer), 2);
}

//----------------------------------------------------------------------------------------------
// Heat stroke
//----------------------------------------------------------------------------------------------

/// The stroke is a 120 degree cone two hexes deep: the aimed hex, the two hexes touching both the
/// Juggernaut and it, and the five - four, when it strikes straight up or down - one step further
/// out. It is measured from the half of the Juggernaut that faces the aim point, which is the hex
/// the unit stands on only when it fights on the attacker side.
class HeatStrokeTest : public HotaAbilitiesTest
{
public:
	/// Aims a stroke of a Juggernaut of the given side at `aimHex` and answers what the unit
	/// standing on `victimHex` lost to it.
	int64_t damageAt(BattleSide side, const BattleHex & juggernautHex, const BattleHex & aimHex, const BattleHex & victimHex)
	{
		startGame();
		startBattle();

		const BattleSide otherSide = side == BattleSide::ATTACKER ? BattleSide::DEFENDER : BattleSide::ATTACKER;

		CStack * juggernaut = addStack(side, creatureByName("vcmi-test:testJuggernaut"), juggernautHex, bigStack);
		CStack * victim = addStack(otherSide, creatureByName("vcmi-test:testDamageCapped"), victimHex, bigStack);
		EXPECT_NE(juggernaut, nullptr);
		EXPECT_NE(victim, nullptr);

		beginCombat();

		const int64_t healthBefore = victim->getAvailableHealth();

		EXPECT_TRUE(castAsUnit(juggernaut, spellByName("heatStroke"), aimHex));

		return healthBefore - victim->getAvailableHealth();
	}
};

TEST_F(HeatStrokeTest, ReachesTheAimedHexFromTheAttackerSide)
{
	const BattleHex juggernaut(leftHex);

	EXPECT_GT(damageAt(BattleSide::ATTACKER, juggernaut, juggernaut.copyToEast(), juggernaut.copyToEast()), 0);
}

/// Regression guard: a defender-side Juggernaut carries its second hex to the east instead of the
/// west, so a cone anchored on its position alone reached the wrong way.
TEST_F(HeatStrokeTest, ReachesTheAimedHexFromTheDefenderSide)
{
	const BattleHex juggernaut(leftHex);

	EXPECT_GT(damageAt(BattleSide::DEFENDER, juggernaut, juggernaut.copyToWest(), juggernaut.copyToWest()), 0);
}

/// The cone is two hexes deep, so the hex behind the aimed one burns as well.
TEST_F(HeatStrokeTest, ReachesTwoHexesDeep)
{
	const BattleHex juggernaut(leftHex);
	const BattleHex aim = juggernaut.copyToEast();

	EXPECT_GT(damageAt(BattleSide::ATTACKER, juggernaut, aim, aim.copyToEast()), 0);
}

/// Regression guard: striking west is measured from the far half of the Juggernaut, which used to
/// be reached with one step too many.
TEST_F(HeatStrokeTest, ReachesWestFromTheRearHalfOnTheAttackerSide)
{
	// the attacker-side unit stands on `leftHex` and covers the hex west of it as well
	const BattleHex juggernaut(leftHex);
	const BattleHex aim = juggernaut.copyToWest().copyToWest();

	EXPECT_GT(damageAt(BattleSide::ATTACKER, juggernaut, aim, aim), 0);
}

/// And it is two hexes deep that way as well.
TEST_F(HeatStrokeTest, ReachesTwoHexesDeepToTheWest)
{
	const BattleHex juggernaut(leftHex);
	const BattleHex aim = juggernaut.copyToWest().copyToWest();

	EXPECT_GT(damageAt(BattleSide::ATTACKER, juggernaut, aim, aim.copyToWest()), 0);
}

/// Striking east from the defender side is measured from its own far half, the same way.
TEST_F(HeatStrokeTest, ReachesEastFromTheRearHalfOnTheDefenderSide)
{
	const BattleHex juggernaut(leftHex);
	const BattleHex aim = juggernaut.copyToEast().copyToEast();

	EXPECT_GT(damageAt(BattleSide::DEFENDER, juggernaut, aim, aim), 0);
}

/// A hex touching the cone but outside its 120 degrees stays cold.
TEST_F(HeatStrokeTest, SparesWhatIsOutsideTheCone)
{
	const BattleHex juggernaut(leftHex);
	const BattleHex outside = juggernaut.copyToNorthEast().copyToNorthWest();

	EXPECT_EQ(damageAt(BattleSide::ATTACKER, juggernaut, juggernaut.copyToEast(), outside), 0);
}
