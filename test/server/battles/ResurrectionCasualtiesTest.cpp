/*
 * ResurrectionCasualtiesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../server/CGameHandler.h"
#include "../../../server/battles/BattleResultProcessor.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"

/// Units resurrected only for one battle die after it, but a hero whose whole army consists
/// of them keeps one unit, so that a won battle does not remove him from the map.
class ResurrectionCasualtiesTest : public BattleTestFixture
{
public:
	const CreatureID first{0};
	const CreatureID second{1};

	CStack * findStack(CreatureID creature) const
	{
		for(const auto & unit : battle()->stacks)
			if(unit->unitSide() == BattleSide::ATTACKER && unit->creatureId() == creature)
				return unit.get();
		return nullptr;
	}

	/// Kills the stack and brings part of it back with basic Resurrection
	void killAndResurrect(const CStack * stack)
	{
		injure(stack, stack->getAvailableHealth());
		ASSERT_FALSE(stack->alive());

		const auto * spell = SpellID(SpellID::RESURRECTION).toSpell();
		spells::BattleCast cast(battle(), attackerSideHero, spells::Mode::HERO, spell);
		spells::Target destination;
		destination.emplace_back(stack->getPosition());
		cast.cast(gameHandler->spellEnv.get(), destination);

		ASSERT_TRUE(stack->alive());
		ASSERT_GT(stack->getResurrected(), 0);
	}

	void applyCasualties()
	{
		CasualtiesAfterBattle casualties(*battle(), BattleSide::ATTACKER);
		casualties.updateArmy(gameHandler.get());
	}
};

TEST_F(ResurrectionCasualtiesTest, LastStackOfResurrectedUnitsKeepsOneUnit)
{
	startGame();
	attackerSideHero->setCreature(SlotID(0), first, 10);
	teachSpell(attackerSideHero, SpellID::RESURRECTION);
	startBattle();

	killAndResurrect(findStack(first));
	applyCasualties();

	EXPECT_EQ(attackerSideHero->stacksCount(), 1);
	EXPECT_EQ(attackerSideHero->getStackCount(SlotID(0)), 1);
}

TEST_F(ResurrectionCasualtiesTest, ResurrectedUnitsDieWhenOtherStackSurvives)
{
	startGame();
	attackerSideHero->setCreature(SlotID(0), first, 10);
	attackerSideHero->setCreature(SlotID(1), second, 10);
	teachSpell(attackerSideHero, SpellID::RESURRECTION);
	startBattle();

	killAndResurrect(findStack(first));
	applyCasualties();

	EXPECT_TRUE(attackerSideHero->slotEmpty(SlotID(0)));
	EXPECT_EQ(attackerSideHero->getStackCount(SlotID(1)), 10);
}
