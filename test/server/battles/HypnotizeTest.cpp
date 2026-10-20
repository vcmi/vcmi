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
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/bonuses/Bonus.h"
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

TEST_F(HypnotizeTest, HypnotizedShooterIsBlockedByItsFormerAllies)
{
	startGame();
	startBattle();
	CStack * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:woodElf"), BattleHex(rightHex), 10);
	addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex + 1), 10);
	hypnotize(shooter);
	beginCombat();

	EXPECT_FALSE(battle()->battleIsUnitBlocked(shooter)) << "a unit of the controller does not block";

	addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex - 1), 10);
	EXPECT_TRUE(battle()->battleIsUnitBlocked(shooter));
}

TEST_F(HypnotizeTest, HypnotizedShooterPrefersShootingItsFormerAllies)
{
	startGame();
	startBattle();
	CStack * shooter = addStack(BattleSide::DEFENDER, creatureByName("core:woodElf"), BattleHex(rightHex), 10);
	CStack * formerAlly = addStack(BattleSide::DEFENDER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(rightHex + 3), 10);
	CStack * controllerUnit = addStack(BattleSide::ATTACKER, creatureByName("vcmi-test:testSoulStealer"), BattleHex(leftHex - 3), 10);
	hypnotize(shooter);
	beginCombat();

	const ActionOption shoot = optionOf(shooter, EActionType::SHOOT);
	const int emptyHexPriority = shoot.type->getPriority(*battle(), shoot, *shooter, nullptr);

	EXPECT_LT(shoot.type->getPriority(*battle(), shoot, *shooter, formerAlly), emptyHexPriority);
	EXPECT_EQ(shoot.type->getPriority(*battle(), shoot, *shooter, controllerUnit), emptyHexPriority);
}

TEST_F(HypnotizeTest, HypnotizedAutomaticUnitAttacksItsFormerAllies)
{
	startGame();
	startBattle();
	CStack * automatic = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 10);
	CStack * formerAlly = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 1), 10);
	CStack * controllerUnit = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(rightHex - 1), 10);
	automatic->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CPU_CONTROLLED, BonusSource::OTHER, 0, BonusSourceID()));
	hypnotize(automatic);
	// nothing else on the field, so that the only units next to it are the two candidates
	for(const auto & stack : battle()->stacks)
		if(stack.get() != automatic && stack.get() != formerAlly && stack.get() != controllerUnit)
			injure(stack.get(), stack->getAvailableHealth());
	blockRetaliation(formerAlly);
	blockRetaliation(controllerUnit);
	beginCombat();

	const int64_t formerAllyHealth = formerAlly->getAvailableHealth();
	const int64_t controllerUnitHealth = controllerUnit->getAvailableHealth();
	endRound();

	EXPECT_LT(formerAlly->getAvailableHealth(), formerAllyHealth);
	EXPECT_EQ(controllerUnit->getAvailableHealth(), controllerUnitHealth);
}

TEST_F(HypnotizeTest, HypnotizedFamiliarChannelsManaToItsController)
{
	startGame();
	startBattle();
	CStack * familiar = addStack(BattleSide::DEFENDER, creatureByName("core:familiar"), BattleHex(rightHex), 10);
	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), 10);
	hypnotize(familiar);
	teachSpell(defenderSideHero, SpellID::BLESS);
	attackerSideHero->mana = 0;
	beginCombat();

	ASSERT_TRUE(castAsHero(defenderSideHero, SpellID::BLESS, target));

	EXPECT_GT(attackerSideHero->mana, 0) << "the familiar now serves the attacker, who gains mana from the defender's spell";
}

TEST_F(HypnotizeTest, HypnotizedUnicornProtectsUnitsOfItsController)
{
	startGame();
	startBattle();
	CStack * unicorn = addStack(BattleSide::DEFENDER, creatureByName("core:unicorn"), BattleHex(rightHex), 10);
	CStack * formerAlly = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), 10);
	CStack * controllerUnit = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(rightHex - 1), 10);
	hypnotize(unicorn);
	beginCombat();

	EXPECT_GT(controllerUnit->magicResistance(), 0);
	EXPECT_EQ(formerAlly->magicResistance(), 0);
}

TEST_F(HypnotizeTest, HypnotizedWraithDrainsManaOfItsOwner)
{
	startGame();
	startBattle();
	CStack * wraith = addStack(BattleSide::DEFENDER, creatureByName("core:wraith"), BattleHex(rightHex), 10);
	hypnotize(wraith);
	attackerSideHero->mana = 20;
	defenderSideHero->mana = 20;
	beginCombat();

	endRound();

	EXPECT_LT(defenderSideHero->mana, 20);
	EXPECT_EQ(attackerSideHero->mana, 20);
}

TEST_F(HypnotizeTest, GateLetsThroughUnitsControlledByTheDefender)
{
	startGame();
	startSiege();
	const BattleHex insideGate = BattleHex(BattleHex::GATE_INNER).cloneInDirection(BattleHex::RIGHT);
	CStack * hypnotizedAttacker = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), insideGate, 10);
	// the town would lose the battle without a unit of its own
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(rightHex), 10);
	hypnotize(hypnotizedAttacker);

	ASSERT_TRUE(act(BattleAction::makeMove(hypnotizedAttacker, BattleHex(BattleHex::GATE_OUTER))));
	EXPECT_EQ(hypnotizedAttacker->getPosition(), BattleHex(BattleHex::GATE_OUTER));
}

TEST_F(HypnotizeTest, GateStopsDefenderUnitsControlledByTheAttacker)
{
	startGame();
	startSiege();
	const BattleHex insideGate = BattleHex(BattleHex::GATE_INNER).cloneInDirection(BattleHex::RIGHT);
	CStack * hypnotizedDefender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), insideGate, 10);
	hypnotize(hypnotizedDefender);

	EXPECT_FALSE(act(BattleAction::makeMove(hypnotizedDefender, BattleHex(BattleHex::GATE_OUTER))));
	EXPECT_EQ(hypnotizedDefender->getPosition(), insideGate);
}
