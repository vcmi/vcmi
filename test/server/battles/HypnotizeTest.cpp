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

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/CPlayerBattleCallback.h"
#include "../../../lib/battle/ReachabilityInfo.h"
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

TEST_F(HypnotizeTest, ControllerCanComputeReachabilityOfHypnotizedUnit)
{
	startGame();
	startBattle();
	CStack * hypnotized = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex), 10);
	addQuicksand(BattleHex(rightHex + 1));
	hypnotize(hypnotized);
	beginCombat();

	// the player that controls the unit sees the battle from its own side, which knows nothing of the defender's quicksand
	CPlayerBattleCallback controller(battle(), battle()->sideToPlayer(BattleSide::ATTACKER));
	ReachabilityInfo reachability;
	ASSERT_NO_THROW(reachability = controller.getReachability(ReachabilityInfo::Parameters(hypnotized, hypnotized->getPosition())));
	EXPECT_TRUE(reachability.isReachable(BattleHex(rightHex + 2)));
}

TEST_F(HypnotizeTest, HypnotizedRandomCasterBuffsUnitsOfItsController)
{
	startGame();
	startBattle();
	CStack * genie = addStack(BattleSide::DEFENDER, creatureByName("core:masterGenie"), BattleHex(rightHex), 10);
	CStack * formerAlly = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex + 1), 10);
	CStack * controllerUnit = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(leftHex), 10);
	hypnotize(genie);
	beginCombat();

	EXPECT_FALSE(act(BattleAction::makeRandomSpellcast(genie, formerAlly->getPosition())));
	EXPECT_TRUE(act(BattleAction::makeRandomSpellcast(genie, controllerUnit->getPosition())));
}
