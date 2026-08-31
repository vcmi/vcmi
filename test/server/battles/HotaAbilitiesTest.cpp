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

/// A spell can kill the automaton just as an attack can, and it detonates either way.
TEST_F(HotaAbilitiesTest, DetonationAnswersADeathBySpell)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 50, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 9999;

	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), bigStack);
	ASSERT_NE(automaton, nullptr);
	ASSERT_NE(victim, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsUnit(automaton, spellByName("abilityDetonation")));

	const int64_t healthBefore = healthOf(victim);

	ASSERT_TRUE(castOn(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), automaton));
	ASSERT_FALSE(automaton->alive()) << "the spell has to kill it for it to detonate";

	EXPECT_EQ(healthBefore - healthOf(victim), detonationDamage);
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

/// A corpse eaten on the way into an attack pays for a blow of that same attack, rather than of
/// the next one - the walk of a walk-and-attack happens before the number of blows is settled.
TEST_F(HotaAbilitiesTest, CorpseDevouredOnTheWayInFeedsTheAttackItWalkedInto)
{
	startGame();
	startBattle();

	CStack * devourer = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testDevourer"), BattleHex(leftHex), bigStack);
	CStack * prey = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);
	CStack * next = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex + 1), bigStack);
	ASSERT_NE(devourer, nullptr);
	ASSERT_NE(next, nullptr);

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
	ASSERT_NE(bearer, nullptr);

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
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	ASSERT_NE(target, nullptr);
	blockRetaliation(bearer);

	beginCombat();

	ASSERT_TRUE(attack(bearer, BattleHex(rightHex)));

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

	EXPECT_EQ(runeLevelOf(bearer), 2) << "the retaliation is part of the same action, and is worth more than the blow given";
}

/// A hero spell that damaged a unit is worth as much to it as a blow taken.
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
	ASSERT_NE(bearer, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), bearer));

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

//----------------------------------------------------------------------------------------------
// Scripting API added for these abilities
//----------------------------------------------------------------------------------------------

/// What a script can ask about a unit, answered from inside a real battle. The probe creature
/// writes every answer into a bonus of its own, which is what these scenarios read back.
class ScriptApiTest : public HotaAbilitiesTest
{
public:
	static constexpr int flagCanRetaliate = 1;
	static constexpr int flagSpellIdentified = 2;
	static constexpr int flagSpellOnAttack = 4;

	CStack * addProbe(BattleSide side, const BattleHex & hex)
	{
		return addStack(side, creatureByName("vcmi-test:testApiProbe"), hex, bigStack);
	}

	static void grant(CStack * unit, BonusType type, int value)
	{
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, type, BonusSource::OTHER, value, BonusSourceID()));
	}

	static int probed(const CStack * unit, const std::string & bonusType)
	{
		return unit->valOfBonuses(Selector::type()(static_cast<BonusType>(BonusTypeID::decode(bonusType))));
	}
};

/// Luck is capped by the game rather than summed, which is the whole reason a script should ask
/// for it rather than add up the bonuses granting it.
TEST_F(ScriptApiTest, LuckIsCappedTheWayTheGameCapsIt)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	ASSERT_NE(probe, nullptr);

	grant(probe, BonusType::LUCK, 10);

	beginCombat();

	EXPECT_EQ(probed(probe, "PROBE_LUCK"), probe->luckVal());
	EXPECT_LT(probed(probe, "PROBE_LUCK"), 10) << "the raw sum of the bonuses would be 10";
	EXPECT_GT(probed(probe, "PROBE_LUCK"), 0);
}

/// Morale is not merely capped - a mechanical unit has none at all, however much of it is granted.
TEST_F(ScriptApiTest, MoraleIsZeroForAUnitThatHasNone)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	grant(probe, BonusType::MORALE, 5);

	beginCombat();

	EXPECT_EQ(probed(probe, "PROBE_MORALE"), 0) << "the probe is mechanical, and the raw sum would be 5";
}

/// A unit that has its retaliation left says so, and an attack names no spell.
TEST_F(ScriptApiTest, RetaliationIsVisibleBeforeTheBlowLands)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex));
	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	ASSERT_NE(attacker, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(attacker, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_FLAGS") & flagCanRetaliate, flagCanRetaliate);
	EXPECT_EQ(probed(probe, "PROBE_FLAGS") & flagSpellOnAttack, 0) << "an attack is no spellcast";
}

/// A unit that cannot answer says that too.
TEST_F(ScriptApiTest, AUnitThatCannotRetaliateSaysSo)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex));
	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);

	grant(probe, BonusType::NO_RETALIATION, 0);

	beginCombat();

	ASSERT_TRUE(attack(attacker, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_FLAGS") & flagCanRetaliate, 0);
}

/// The spellcast event names the spell that was cast.
TEST_F(ScriptApiTest, SpellcastNamesItsSpell)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testDamageCapped"), BattleHex(rightHex), bigStack);
	ASSERT_NE(victim, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsUnit(probe, spellByName("heatStroke"), BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_FLAGS") & flagSpellIdentified, flagSpellIdentified);
}

/// A walk-and-attack walks, and says so. That the walk is announced before the number of blows is
/// settled is what lets an ability that reacts to it add one to the very attack it walked into.
TEST_F(ScriptApiTest, WalkAndAttackAnnouncesItsWalk)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex + 2), bigStack);
	ASSERT_NE(target, nullptr);

	beginCombat();

	ASSERT_TRUE(attackFrom(probe, BattleHex(rightHex + 2), BattleHex(rightHex + 1)));

	EXPECT_EQ(probed(probe, "PROBE_MOVES"), 1);
}

/// An attack that reaches its target from where it already stands walks nowhere, and says nothing.
TEST_F(ScriptApiTest, AnAttackWithoutAWalkAnnouncesNoWalk)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex), bigStack);
	ASSERT_NE(target, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(probe, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_MOVES"), 0);
}

/// A hero spell reaching a unit is reported to it, names itself, and names no casting unit.
TEST_F(ScriptApiTest, HeroSpellIsReportedToWhatItHits)
{
	startGame();

	giveArtifact(defenderSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	defenderSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	defenderSideHero->mana = 9999;

	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	ASSERT_NE(probe, nullptr);

	beginCombat();

	const int64_t healthBefore = probe->getAvailableHealth();

	ASSERT_TRUE(castOn(defenderSideHero, SpellID(SpellID::MAGIC_ARROW), probe));

	EXPECT_EQ(probed(probe, "PROBE_SPELL_HITS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_HERO_CASTS"), 1) << "a hero cast it, so no unit is named";
	EXPECT_EQ(probed(probe, "PROBE_SPELL_NAMED"), 1) << "and the spell that hit is named";
	EXPECT_EQ(probed(probe, "PROBE_HEALTH_BEFORE"), healthBefore) << "the captured state is the one from before the spell";
	EXPECT_EQ(probed(probe, "PROBE_SPELL_DAMAGE"), healthBefore - probe->getAvailableHealth());
	EXPECT_GT(probed(probe, "PROBE_SPELL_DAMAGE"), 0);
}

/// A spell a unit cast names that unit, so that the two kinds of cast are told apart.
TEST_F(ScriptApiTest, UnitSpellNamesItsCaster)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testJuggernaut"), BattleHex(rightHex), bigStack);
	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(rightHex - 1));
	ASSERT_NE(caster, nullptr);
	ASSERT_NE(probe, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsUnit(caster, spellByName("heatStroke"), BattleHex(rightHex - 1)));

	EXPECT_EQ(probed(probe, "PROBE_SPELL_HITS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_HERO_CASTS"), 0) << "a unit cast it, and is named";
}

/// Only a cast someone chose to make is a spell hit. What a script applies on its own is not, which
/// is also what keeps a script that casts from re-entering itself.
TEST_F(ScriptApiTest, AScriptedCastIsNoSpellHit)
{
	startGame();
	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex + 1));
	CStack * killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	ASSERT_NE(automaton, nullptr);
	ASSERT_NE(killer, nullptr);

	beginCombat();

	const int64_t healthBefore = probe->getAvailableHealth();

	// the detonation script damages the probe and writes its own spell into the combat log
	ASSERT_TRUE(castAsUnit(automaton, spellByName("abilityDetonation")));
	ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
	ASSERT_FALSE(automaton->alive());
	ASSERT_LT(probe->getAvailableHealth(), healthBefore) << "the detonation has to reach the probe";

	EXPECT_EQ(probed(probe, "PROBE_SPELL_HITS"), 0);
}

/// An action is reported as finished once it is wholly over, to every unit it reached - which is
/// what lets a script bank something over an action and hand it out when the action ends.
TEST_F(ScriptApiTest, AnActionIsReportedToTheUnitThatTookIt)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::ATTACKER, BattleHex(leftHex));
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex), bigStack);
	ASSERT_NE(target, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(probe, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_ACTIONS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_OWN_ACTIONS"), 1) << "the probe is the one that acted";
}

/// And to a unit that only stood in the way of one.
TEST_F(ScriptApiTest, AnActionIsReportedToWhatItReached)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex));
	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), bigStack);
	ASSERT_NE(attacker, nullptr);

	beginCombat();

	ASSERT_TRUE(attack(attacker, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_ACTIONS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_OWN_ACTIONS"), 0) << "somebody else acted";
}

/// One report per action, however many blows the action was made of. This is what a flag on the
/// last blow would have to work out in advance, and what the end of the action simply knows.
TEST_F(ScriptApiTest, AnActionOfSeveralBlowsIsReportedOnce)
{
	startGame();
	startBattle();

	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex));
	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), bigStack);

	grant(attacker, BonusType::ADDITIONAL_ATTACK, 1);
	ASSERT_EQ(attacker->getTotalAttacks(false), 2);

	beginCombat();

	ASSERT_TRUE(attack(attacker, BattleHex(rightHex)));

	EXPECT_EQ(probed(probe, "PROBE_ACTIONS"), 1) << "two blows, one action";
}

/// A hero spell is an action of its own, and finishes as one.
TEST_F(ScriptApiTest, AHeroSpellFinishesAsItsOwnAction)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	attackerSideHero->mana = 9999;

	startBattle();

	CStack * probe = addProbe(BattleSide::DEFENDER, BattleHex(rightHex));
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), bigStack);
	ASSERT_NE(probe, nullptr);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), probe));

	EXPECT_EQ(probed(probe, "PROBE_SPELL_HITS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_ACTIONS"), 1);
	EXPECT_EQ(probed(probe, "PROBE_OWN_ACTIONS"), 0) << "a hero acted, so no unit did";
}
