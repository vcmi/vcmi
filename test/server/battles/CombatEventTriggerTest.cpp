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

#include "../../lib/bonuses/BonusParameters.h"

namespace
{

/// Marker the reaction to one event grants, so that the value accumulated on a unit says exactly
/// which of its events fired. Luck is used because nothing else in the scenario grants any.
enum MarkerValue
{
	BEFORE_ATTACK = 1,
	AFTER_ATTACK = 2,
	BEFORE_ATTACKED = 4,
	AFTER_ATTACKED = 8,
};

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
	ASSERT_NE(defender, nullptr);
	ASSERT_NE(attacker, nullptr);

	// a retaliation would fire the same events again, with the roles swapped
	blockRetaliation(attacker);

	reactWithMarker(attacker, CombatEventType::BEFORE_ATTACK, BEFORE_ATTACK);
	reactWithMarker(attacker, CombatEventType::AFTER_ATTACK, AFTER_ATTACK);
	reactWithMarker(defender, CombatEventType::BEFORE_ATTACKED, BEFORE_ATTACKED);
	reactWithMarker(defender, CombatEventType::AFTER_ATTACKED, AFTER_ATTACKED);

	ASSERT_EQ(markersOf(attacker), 0);
	ASSERT_EQ(markersOf(defender), 0);

	ASSERT_TRUE(attack(attacker, BattleHex(leftHex)));
	ASSERT_TRUE(defender->alive()) << "the victim has to survive to be checked";

	EXPECT_EQ(markersOf(attacker), BEFORE_ATTACK + AFTER_ATTACK);
	EXPECT_EQ(markersOf(defender), BEFORE_ATTACKED + AFTER_ATTACKED);
}
