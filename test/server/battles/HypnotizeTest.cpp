/*
 * HypnotizeTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/mapObjects/CGHeroInstance.h"

/// A hypnotized unit keeps its owner and side, but is controlled by the opposing player and fights
/// its own army. These check the places that have to tell friend from foe for such a unit.
class HypnotizeTest : public BattleTestFixture
{
public:
	/// Summoned units of the given creature, alive, in the order they were added
	std::vector<const CStack *> summoned(const CreatureID & creature) const
	{
		std::vector<const CStack *> result;
		for(const auto & stack : battle()->stacks)
			if(stack->isSummoned() && stack->alive() && stack->creatureId() == creature)
				result.push_back(stack.get());
		return result;
	}
};

TEST_F(HypnotizeTest, SummonDoesNotReinforceOwnHypnotizedElementals)
{
	startGame();
	startBattle();
	const SpellID summon = spellByName("vcmi-test:testSummonSameUnit");
	teachSpell(attackerSideHero, summon);
	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, summon, battle::Target{}));
	ASSERT_EQ(summoned(CreatureID::AIR_ELEMENTAL).size(), 1);
	hypnotize(const_cast<CStack *>(summoned(CreatureID::AIR_ELEMENTAL).front()));

	endRound();
	ASSERT_TRUE(castAsHero(attackerSideHero, summon, battle::Target{}));

	EXPECT_EQ(summoned(CreatureID::AIR_ELEMENTAL).size(), 2) << "elementals controlled by the enemy must not be reinforced";
}
