/*
 * BindTest.cpp, part of VCMI engine
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
#include "../../../server/battles/BattleProcessor.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/networkPacks/PacksForClient.h"

namespace
{
// creatures
const CreatureID dendroid(22); // binds its target on attack
const CreatureID pikeman(0);
const CreatureID harpy(72); // returns to its own hex after attacking
const CreatureID titan(54); // kills anything of these tests in one blow
}

/// Dendroids bind the unit they attack: it can not move until every dendroid that bound it is gone
/// from its side. Everything here happens in one row of the battlefield, where units are placed by
/// their column, so that "adjacent" and "away" are plain to read.
class BindTest : public BattleTestFixture
{
public:
	static constexpr int row = 5;

	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
	}

	CStack * place(BattleSide side, const CreatureID & creature, int column, int count)
	{
		CStack * unit = addStack(side, creature, BattleHex(column, row), count);
		EXPECT_NE(unit, nullptr);
		return unit;
	}

	/// Unit to be bound. Large enough to survive being attacked - a dead unit can not be bound - and
	/// does not retaliate, which would kill the dendroid that is meant to keep holding it
	CStack * victim(int column)
	{
		CStack * unit = place(BattleSide::ATTACKER, pikeman, column, 1000);
		blockRetaliation(unit);
		return unit;
	}

	CStack * binder(int column)
	{
		return place(BattleSide::DEFENDER, dendroid, column, 100);
	}

	static bool isBound(const CStack * unit)
	{
		return unit->hasBonusOfType(BonusType::BIND_EFFECT);
	}

	/// Attack made from the attacker's own hex, which leaves it where it stands
	bool strike(const CStack * attacker, const CStack * target)
	{
		return attack(attacker, target->getPosition());
	}

	/// Attack made after walking up to the target, which is what sends a harpy back home again
	bool strikeFrom(const CStack * attacker, const CStack * target, int fromColumn)
	{
		battle()->activeStack = attacker->unitId();
		BattleAction action = BattleAction::makeMeleeAttack(attacker, target->getPosition(), BattleHex(fromColumn, row));
		return gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(attacker->unitSide()), action);
	}

	/// Dendroids walk three hexes at most, and the rows next to the one of the tests are empty,
	/// so `rowsAway` is how a unit boxed in by its own victims gets out of their reach
	bool walk(const CStack * unit, int column, int rowsAway = 0)
	{
		battle()->activeStack = unit->unitId();
		BattleAction action = BattleAction::makeMove(unit, BattleHex(column, row + rowsAway));
		return gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(unit->unitSide()), action);
	}

	/// Kills the unit outright, by way of an attack, so that the death runs through the same code
	/// a death in a real battle does
	void kill(CStack * unit)
	{
		CStack * killer = place(battle()->otherSide(unit->unitSide()), titan, unit->getPosition().getX() - 1, 100);
		forceMaximumDamage(killer);
		EXPECT_TRUE(strike(killer, unit));
		EXPECT_FALSE(unit->alive());
	}

	/// Hero spellcasting, which these tests only need in order to teleport units around
	void allowSpellcasting(CGHeroInstance * hero, const SpellID & spell)
	{
		giveArtifact(hero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

		ChangeSpells learn;
		learn.hid = hero->id;
		learn.learn = true;
		learn.spells = {spell};
		gameHandler->sendAndApply(learn);

		SetMana mana;
		mana.hid = hero->id;
		mana.val = 999;
		mana.mode = ChangeValueMode::ABSOLUTE;
		gameHandler->sendAndApply(mana);
	}

	bool teleport(CGHeroInstance * hero, const CStack * unit, int toColumn)
	{
		allowSpellcasting(hero, SpellID::TELEPORT);

		// a hero can only cast while a unit of its own side is the active one
		for(const auto * stack : battle()->battleGetAllStacks())
			if(stack->alive() && battle()->battleGetOwner(stack) == hero->getOwner())
				battle()->activeStack = stack->unitId();

		BattleAction action;
		action.side = battle()->whatSide(hero->getOwner());
		action.actionType = EActionType::HERO_SPELL;
		action.spell = SpellID::TELEPORT;

		battle::Target target;
		target.emplace_back(unit);
		target.emplace_back(BattleHex(toColumn, row));
		action.setTarget(target);

		return gameHandler->battles->makePlayerBattleAction(BattleID(0), hero->getOwner(), action);
	}
};

TEST_F(BindTest, attackBindsTargetInPlace)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	EXPECT_FALSE(isBound(target));
	ASSERT_TRUE(strike(dendroidStack, target));
	EXPECT_TRUE(isBound(target));
	EXPECT_EQ(target->getMovementRange(), 0u);
	EXPECT_FALSE(walk(target, 4)) << "bound unit must not be able to walk away";
}

TEST_F(BindTest, boundUnitCanStillAttack)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	EXPECT_TRUE(strike(target, dendroidStack)) << "bound unit keeps its attack against an adjacent enemy";
}

TEST_F(BindTest, deathOfBinderReleasesTarget)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	kill(dendroidStack);

	EXPECT_FALSE(isBound(target));
	EXPECT_GT(target->getMovementRange(), 0u);
}

TEST_F(BindTest, binderWalkingAwayReleasesTarget)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	ASSERT_TRUE(walk(dendroidStack, 10));

	EXPECT_FALSE(isBound(target));
}

TEST_F(BindTest, binderStayingAdjacentKeepsTargetBound)
{
	// victim stands in the middle of the row, with both hexes next to it free for the binder
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	ASSERT_TRUE(walk(dendroidStack, 5)) << "stepping around the target, still next to it";

	EXPECT_TRUE(isBound(target));
}

/// The binding is permanent: it is not a timed effect that a new round would wear off
TEST_F(BindTest, bindOutlivesTheRoundItWasCastIn)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	beginCombat();
	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	endRound();

	EXPECT_TRUE(isBound(target));
	EXPECT_EQ(target->getMovementRange(), 0u);
}

/// Disabled: fails due to https://github.com/vcmi/vcmi/issues/4349 - binding the same unit a second
/// time only refreshes the effect of the first dendroid instead of recording the second one, so the
/// victim is released as soon as the first dendroid is gone
TEST_F(BindTest, DISABLED_twoBindersHoldTargetUntilBothAreGone)
{
	CStack * target = victim(6);
	CStack * first = binder(7);
	CStack * second = binder(5);

	ASSERT_TRUE(strike(first, target));
	ASSERT_TRUE(strike(second, target));
	ASSERT_TRUE(isBound(target));

	kill(first);
	EXPECT_TRUE(isBound(target)) << "second dendroid is still next to the victim";

	ASSERT_TRUE(walk(second, 5, 2));
	EXPECT_FALSE(isBound(target));
}

/// Disabled: fails due to https://github.com/vcmi/vcmi/issues/4349, see above
TEST_F(BindTest, DISABLED_rebindingByAnotherDendroidOutlivesTheFirstOne)
{
	CStack * target = victim(6);
	CStack * first = binder(7);
	CStack * second = binder(5);

	ASSERT_TRUE(strike(first, target));
	ASSERT_TRUE(strike(second, target));
	ASSERT_TRUE(isBound(target));

	// the first dendroid leaves, the one that bound the victim later stays
	ASSERT_TRUE(walk(first, 10));

	EXPECT_TRUE(isBound(target)) << "victim is still held by the second dendroid";
}

TEST_F(BindTest, oneBinderHoldsSeveralTargets)
{
	CStack * left = victim(6);
	CStack * right = victim(8);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, left));
	ASSERT_TRUE(strike(dendroidStack, right));
	EXPECT_TRUE(isBound(left));
	EXPECT_TRUE(isBound(right));

	kill(dendroidStack);

	EXPECT_FALSE(isBound(left));
	EXPECT_FALSE(isBound(right));
}

TEST_F(BindTest, oneBinderReleasesSeveralTargetsWhenItWalksAway)
{
	CStack * left = victim(6);
	CStack * right = victim(8);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, left));
	ASSERT_TRUE(strike(dendroidStack, right));

	ASSERT_TRUE(walk(dendroidStack, 7, 2)) << "out of reach of both victims";

	EXPECT_FALSE(isBound(left));
	EXPECT_FALSE(isBound(right));
}

TEST_F(BindTest, retaliationBindsTheHarpyInPlace)
{
	CStack * flyer = place(BattleSide::ATTACKER, harpy, 3, 1000);
	CStack * dendroidStack = binder(8);

	const BattleHex home = flyer->getPosition();
	ASSERT_TRUE(strikeFrom(flyer, dendroidStack, 7));

	EXPECT_TRUE(isBound(flyer)) << "dendroid binds whoever it strikes, retaliation included";
	EXPECT_NE(flyer->getPosition(), home) << "a bound harpy can not fly back to where it came from";
	EXPECT_EQ(flyer->getPosition(), BattleHex(7, row));
}

TEST_F(BindTest, teleportingTargetAwayReleasesIt)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	ASSERT_TRUE(teleport(attackerSideHero, target, 1));
	ASSERT_EQ(target->getPosition(), BattleHex(1, row));

	EXPECT_FALSE(isBound(target));
}

TEST_F(BindTest, teleportingBinderAwayReleasesTarget)
{
	CStack * target = victim(6);
	CStack * dendroidStack = binder(7);

	ASSERT_TRUE(strike(dendroidStack, target));
	ASSERT_TRUE(isBound(target));

	ASSERT_TRUE(teleport(defenderSideHero, dendroidStack, 14));
	ASSERT_EQ(dendroidStack->getPosition(), BattleHex(14, row));

	EXPECT_FALSE(isBound(target));
}
