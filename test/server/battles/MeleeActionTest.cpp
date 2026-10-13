/*
 * MeleeActionTest.cpp, part of VCMI engine
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
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/bonuses/Bonus.h"

namespace
{
// creatures
constexpr int pikeman = 0; // walks 4 hexes
constexpr int harpy = 72; // flies 6 hexes, returns after striking
constexpr int greenDragon = 26; // two hexes wide, its breath also strikes the unit behind the target
}

/// What a WALK_AND_ATTACK action does on the server: who strikes in which order, where the
/// attacker ends up, and what cancels or adds to the attack. Attackers and targets share one row.
class MeleeActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex attackerStart = BattleHex(4, 5);
	static inline const BattleHex attackFrom = BattleHex(7, 5);
	static inline const BattleHex targetHex = BattleHex(8, 5);
};

TEST_F(MeleeActionTest, targetRetaliatesAfterBeingHit)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto attackerHealth = attacker->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_LT(attacker->getAvailableHealth(), attackerHealth);
}

TEST_F(MeleeActionTest, targetWithFirstStrikeHitsBeforeAttacker)
{
	startGame();
	startBattle();

	// one attacker against a large stack: the strike that comes first kills it before it can hit
	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, 1);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, 100);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FIRST_STRIKE, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(BonusCustomSubtype::damageTypeAll)));

	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_FALSE(attacker->alive());
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
}

TEST_F(MeleeActionTest, returningAttackerFliesBackToStart)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(harpy), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackerStart);
}

TEST_F(MeleeActionTest, returningAttackerCanStayAfterStrike)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(harpy), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom, false)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackFrom);
}

TEST_F(MeleeActionTest, longWeaponStrikesOverFreeHexWithoutRetaliation)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), twoHexesAway, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));

	const auto attackerHealth = attacker->getAvailableHealth();
	const auto targetHealth = target->getAvailableHealth();

	ASSERT_TRUE(act(BattleAction::makeLongWeaponAttack(attacker, targetHex, twoHexesAway)));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getAvailableHealth(), attackerHealth);
	EXPECT_EQ(attacker->getPosition(), twoHexesAway);
}

TEST_F(MeleeActionTest, longWeaponAttackOfAdjacentTargetIsRefused)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));

	const auto targetHealth = target->getAvailableHealth();

	EXPECT_FALSE(act(BattleAction::makeLongWeaponAttack(attacker, targetHex, attackFrom)));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
}

TEST_F(MeleeActionTest, targetTwoHexesAwayIsRefusedWithoutLongWeapon)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), twoHexesAway, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	EXPECT_FALSE(attack(attacker, targetHex));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), twoHexesAway);
}

TEST_F(MeleeActionTest, attackFromHexAwayFromTargetIsRefusedWithoutMoving)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	EXPECT_FALSE(act(BattleAction::makeMeleeAttack(attacker, targetHex, twoHexesAway)));
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackerStart);
}

TEST_F(MeleeActionTest, unitOfOwnSideIsRefusedAsTarget)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), targetHex, stackCount);
	const auto allyHealth = ally->getAvailableHealth();

	EXPECT_FALSE(attack(attacker, targetHex));
	EXPECT_EQ(ally->getAvailableHealth(), allyHealth);
}

TEST_F(MeleeActionTest, berserkUnitAttacksUnitOfOwnSide)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ATTACKS_NEAREST_CREATURE, BonusSource::OTHER, 0, BonusSourceID()));
	const auto allyHealth = ally->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, targetHex));
	EXPECT_LT(ally->getAvailableHealth(), allyHealth);
}

TEST_F(MeleeActionTest, quicksandOnTheWayCancelsAttack)
{
	startGame();
	startBattle();

	const BattleHex halfway = BattleHex(5, 5);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(3, 5), stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	addQuicksand(halfway);

	const auto targetHealth = target->getAvailableHealth();

	// stepping into a trap is a legal move that ends the action early
	ASSERT_TRUE(act(BattleAction::makeMeleeAttack(attacker, targetHex, attackFrom)));
	EXPECT_EQ(attacker->getPosition(), halfway);
	EXPECT_EQ(target->getAvailableHealth(), targetHealth);
}

/// What the client shows and sends for the melee options of a unit on a hovered hex.
class MeleeOptionTest : public MeleeActionTest
{
public:
	ActionOption optionOf(const CStack * unit, EActionType actionType) const
	{
		battle()->activeStack = unit->unitId();
		std::vector<ActionOption> options;
		BattleActionType::collectAllOptions(*battle(), *unit, options);

		for(const auto & option : options)
			if(option.type == BattleActionType::find(actionType))
				return option;

		ADD_FAILURE() << "no such option";
		return {};
	}

	/// Context of the hex approached from the direction, without an approach if the option does not offer the direction
	ActionContext contextOf(const ActionOption & option, const CStack * unit, const BattleHex & hex, BattleHex::EDir direction) const
	{
		ActionContext context{unit, hex, std::nullopt};
		for(const auto & approach : option.type->getApproaches(*battle(), option, *unit, hex))
			if(approach.direction == direction)
				context.approach = approach;
		return context;
	}
};

TEST_F(MeleeOptionTest, attackPreviewShowsDirectionDamageAndRetaliation)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	const auto targetHealth = target->getAvailableHealth();

	const auto option = optionOf(attacker, EActionType::WALK_AND_ATTACK);
	const auto context = contextOf(option, attacker, targetHex, BattleHex::LEFT);
	ASSERT_TRUE(context.approach.has_value());
	EXPECT_EQ(context.approach->from, attackFrom);

	const auto shown = option.type->preview(*gameState(), *battle(), option, context);
	EXPECT_EQ(shown.cursor, "combatHitEast");
	EXPECT_THAT(shown.statusText.toString(LIBRARY->staticTexts()), ::testing::MatchesRegex("Attack Pikemen \\([0-9]+ - [0-9]+ damage.*\\)\\.\nWill retaliate \\(.*damage.*\\)\\."));
	EXPECT_THAT(shown.shadedHexes, ::testing::ElementsAre(attackFrom));
	EXPECT_THAT(shown.affectedUnits, ::testing::IsEmpty());

	ASSERT_TRUE(act(option.type->build(*battle(), option, context).value()));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), attackFrom);
}

TEST_F(MeleeOptionTest, sideOutOfReachIsNotOffered)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackerStart, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	// four hexes of movement reach the near side of the target, but not its far side
	const auto option = optionOf(attacker, EActionType::WALK_AND_ATTACK);
	const auto context = contextOf(option, attacker, targetHex, BattleHex::RIGHT);

	EXPECT_FALSE(context.approach.has_value());
	EXPECT_FALSE(option.type->isLegal(*gameState(), *battle(), option, context));
	EXPECT_EQ(option.type->preview(*gameState(), *battle(), option, context).cursor, "combatBlocked");
}

TEST_F(MeleeOptionTest, returningUnitPrefersToReturn)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(harpy), attackerStart, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);

	const auto returning = optionOf(attacker, EActionType::WALK_AND_ATTACK);
	const auto staying = optionOf(attacker, EActionType::WALK_AND_ATTACK_NO_RETURN);

	EXPECT_LT(returning.type->getPriority(*battle(), returning, *attacker, target), staying.type->getPriority(*battle(), staying, *attacker, target));
	EXPECT_NE(returning.button, staying.button);

	ASSERT_TRUE(act(staying.type->build(*battle(), staying, contextOf(staying, attacker, targetHex, BattleHex::LEFT)).value()));
	EXPECT_EQ(attacker->getPosition(), attackFrom);
}

TEST_F(MeleeOptionTest, longWeaponOptionStrikesFromTwoHexesAway)
{
	startGame();
	startBattle();

	const BattleHex twoHexesAway = attackFrom.cloneInDirection(BattleHex::LEFT);

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), twoHexesAway, stackCount);
	CStack * target = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));
	const auto targetHealth = target->getAvailableHealth();

	const auto option = optionOf(attacker, EActionType::LONG_WEAPON_ATTACK);
	const auto context = contextOf(option, attacker, targetHex, BattleHex::LEFT);
	ASSERT_TRUE(context.approach.has_value());
	EXPECT_EQ(context.approach->from, twoHexesAway);

	const auto shown = option.type->preview(*gameState(), *battle(), option, context);
	EXPECT_THAT(shown.statusText.toString(LIBRARY->staticTexts()), ::testing::HasSubstr("Will not retaliate"));

	ASSERT_TRUE(act(option.type->build(*battle(), option, context).value()));
	EXPECT_LT(target->getAvailableHealth(), targetHealth);
	EXPECT_EQ(attacker->getPosition(), twoHexesAway);
}

TEST_F(MeleeOptionTest, longWeaponOptionIsNotOfferedForAdjacentTarget)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), attackFrom, stackCount);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LONG_WEAPON, BonusSource::OTHER, 0, BonusSourceID()));

	const auto longWeapon = optionOf(attacker, EActionType::LONG_WEAPON_ATTACK);
	const auto adjacent = optionOf(attacker, EActionType::WALK_AND_ATTACK);

	// the hex two hexes away is reachable, but the attacker already stands next to the target on that line
	EXPECT_FALSE(contextOf(longWeapon, attacker, targetHex, BattleHex::LEFT).approach.has_value());
	EXPECT_TRUE(contextOf(adjacent, attacker, targetHex, BattleHex::LEFT).approach.has_value());
}

TEST_F(MeleeOptionTest, breathPreviewNamesUnitBehindTarget)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, CreatureID(greenDragon), attackFrom, 1);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex, stackCount);
	CStack * behind = addStack(BattleSide::DEFENDER, CreatureID(pikeman), targetHex.cloneInDirection(BattleHex::RIGHT), stackCount);

	const auto option = optionOf(attacker, EActionType::WALK_AND_ATTACK);
	const auto shown = option.type->preview(*gameState(), *battle(), option, contextOf(option, attacker, targetHex, BattleHex::LEFT));

	EXPECT_THAT(shown.affectedUnits, ::testing::ElementsAre(behind->unitId()));
	EXPECT_THAT(shown.shadedHexes, ::testing::UnorderedElementsAre(attackFrom, attackFrom.cloneInDirection(BattleHex::LEFT)));
}
