/*
 * CombatEventTriggerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/bonuses/BonusParameters.h"

namespace
{

/// Marker the reaction to one event grants, so that the value accumulated on a unit says exactly
/// which of its events fired. Luck is used because nothing else in the scenario grants any.
constexpr int markerBeforeAttack = 1;
constexpr int markerAfterAttack = 2;
constexpr int markerBeforeAttacked = 4;
constexpr int markerAfterAttacked = 8;
constexpr int markerDeath = 16;
constexpr int markerActionFinished = 32;
constexpr int markerMove = 64;
constexpr int markerSpellHit = 128;

}

/// The ON_COMBAT_EVENT bonus reacts to events of the unit carrying it with a predefined action -
/// granting a bonus or casting a spell. This pins that the four events of an attack reach it, on
/// both sides of that attack.
class CombatEventTriggerTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 100;

	/// Makes `unit` grant itself a luck bonus of `marker` whenever `event` happens to it.
	static void reactWithMarker(CStack * unit, CombatEventType event, int marker)
	{
		BonusParametersOnCombatEvent::CombatEffectBonus effect;
		effect.bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OTHER, marker, BonusSourceID());
		effect.targetEnemy = false;

		BonusParametersOnCombatEvent reaction;
		reaction.effects.emplace_back(effect);

		auto trigger = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ON_COMBAT_EVENT, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(BonusCustomSubtype(static_cast<int>(event))));
		trigger->parameters = std::make_shared<BonusParameters>(reaction);

		unit->addNewBonus(trigger);
	}

	static int markersOf(const CStack * unit)
	{
		return unit->valOfBonuses(BonusType::LUCK);
	}
};

TEST_F(CombatEventTriggerTest, everyAttackEventReachesItsUnit)
{
	startGame();
	startBattle();

	CStack * defender = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), stackCount);
	CStack * attacker = addStack(BattleSide::DEFENDER, creatureByName("core:blackDragon"), BattleHex(rightHex), stackCount);

	// a retaliation would fire the same events again, with the roles swapped
	blockRetaliation(attacker);

	reactWithMarker(attacker, CombatEventType::BEFORE_ATTACK, markerBeforeAttack);
	reactWithMarker(attacker, CombatEventType::AFTER_ATTACK, markerAfterAttack);
	reactWithMarker(defender, CombatEventType::BEFORE_ATTACKED, markerBeforeAttacked);
	reactWithMarker(defender, CombatEventType::AFTER_ATTACKED, markerAfterAttacked);

	ASSERT_EQ(markersOf(attacker), 0);
	ASSERT_EQ(markersOf(defender), 0);

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));
	ASSERT_TRUE(defender->alive()) << "the victim has to survive to be checked";

	EXPECT_EQ(markersOf(attacker), markerBeforeAttack + markerAfterAttack);
	EXPECT_EQ(markersOf(defender), markerBeforeAttacked + markerAfterAttacked);
}

/// Verifies one ACTION_FINISHED event per participant after UNIT_DEATH
TEST_F(CombatEventTriggerTest, deathAndTheEndOfAnActionReachEvenTheUnitTheActionKilled)
{
	startGame();
	startBattle();

	CStack * victim = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 1);
	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), stackCount);

	// Two attacks must produce one ACTION_FINISHED event.
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::OTHER, 1, BonusSourceID()));
	ASSERT_EQ(attacker->getTotalAttacks(false), 2);

	reactWithMarker(victim, CombatEventType::UNIT_DEATH, markerDeath);
	reactWithMarker(victim, CombatEventType::ACTION_FINISHED, markerActionFinished);
	reactWithMarker(attacker, CombatEventType::ACTION_FINISHED, markerActionFinished);

	beginCombat();

	ASSERT_TRUE(attack(attacker, BattleHex(rightHex)));
	ASSERT_FALSE(victim->alive()) << "the victim has to die for the scenario to say anything";

	EXPECT_EQ(markersOf(victim), markerDeath + markerActionFinished);
	EXPECT_EQ(markersOf(attacker), markerActionFinished) << "two attacks, one action";
}

/// Verifies separate movement events for approach and RETURN_AFTER_STRIKE
TEST_F(CombatEventTriggerTest, theStepBackAfterStrikingIsAnnouncedAsAMove)
{
	constexpr int attackFromHex = leftHex + 3;
	constexpr int targetHex = leftHex + 4;

	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), stackCount);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), stackCount);

	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::RETURN_AFTER_STRIKE, BonusSource::OTHER, 0, BonusSourceID()));

	// Keep the attacker alive for the return movement.
	blockRetaliation(attacker);

	reactWithMarker(attacker, CombatEventType::AFTER_MOVE, markerMove);

	beginCombat();

	ASSERT_TRUE(attackFrom(attacker, BattleHex(targetHex), BattleHex(attackFromHex)));
	ASSERT_EQ(attacker->getPosition(), BattleHex(leftHex)) << "the scenario is about the step back";

	EXPECT_EQ(markersOf(attacker), 2 * markerMove) << "the walk in and the step back are both moves";
}

/// Verifies that script-applied spells do not generate SPELL_HIT
TEST_F(CombatEventTriggerTest, aScriptedCastIsNoSpellHit)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::MAGIC_ARROW));
	attackerSideHero->mana = 9999;

	startBattle();

	CStack * automaton = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testAutomaton"), BattleHex(rightHex), 1);
	CStack * bystander = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), stackCount);
	CStack * killer = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), stackCount);

	reactWithMarker(bystander, CombatEventType::SPELL_HIT, markerSpellHit);

	beginCombat();

	const int64_t healthBefore = bystander->getAvailableHealth();

	// The detonation must execute without recursively generating SPELL_HIT.
	ASSERT_TRUE(attack(killer, BattleHex(rightHex)));
	ASSERT_FALSE(automaton->alive());
	ASSERT_LT(bystander->getAvailableHealth(), healthBefore) << "detonation did not damage the bystander";

	EXPECT_EQ(markersOf(bystander), 0);

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::MAGIC_ARROW), bystander));

	EXPECT_EQ(markersOf(bystander), markerSpellHit) << "a cast someone made is a spell hit";
}
