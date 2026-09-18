/*
 * CombatValueTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/BattleFake.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_UnitInfo.h"
#include "mock/mock_UnitEnvironment.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/battle/CombatValue.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/mapObjects/army/CStackInstance.h"

namespace
{

std::unique_ptr<CStackInstance> makeStack(const CreatureID & creature, TQuantity count)
{
	return std::make_unique<CStackInstance>(nullptr, creature, count, false);
}

}

TEST(CombatValueTest, stackIsWorthAsMuchAsTheCreaturesInIt)
{
	auto single = makeStack(CreatureID::ARCHER, 1);
	auto many = makeStack(CreatureID::ARCHER, 50);

	EXPECT_GT(single->estimateCombatValue(), 0u);
	EXPECT_EQ(many->estimateCombatValue(), single->estimateCombatValue() * 50);
}

TEST(CombatValueTest, stackWorthMatchesWhatTheModelAnswersForItsCreature)
{
	auto stack = makeStack(CreatureID::AZURE_DRAGON, 7);
	const auto expected = LIBRARY->combatValues->getAIValue(stack->getType()) * 7;

	EXPECT_EQ(stack->estimateCombatValue(), static_cast<ui64>(expected));
}

/// An army on the map is valued by its creature type, so that armies which exist and armies which
/// are only proposed - creatures about to be bought - stay comparable. Bonuses reaching one stack
/// and not another are only accounted for in battle, where every unit has a bearer to read.
TEST(CombatValueTest, mapWorthIgnoresBonusesOnTheStack)
{
	auto stack = makeStack(CreatureID::ARCHER, 10);
	const auto before = stack->estimateCombatValue();

	stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH,
		BonusSource::OTHER, 100, BonusSourceID()));

	EXPECT_EQ(stack->estimateCombatValue(), before);
}

namespace test
{
using namespace ::testing;

/// A unit whose stats the test chooses, so that an effect can be judged by what it does to the
/// value rather than by a number written down here
struct MeasuredUnit
{
	static constexpr int32_t count = 20;

	UnitInfoMock info;
	UnitEnvironmentMock environment;
	BonusBearerMock bonuses;
	::battle::CUnitStateDetached unit;

	MeasuredUnit(int damageMin, int damageMax, bool shooter = false)
		: unit(&info, &bonuses)
	{
		give(BonusType::STACK_HEALTH, 30);
		give(BonusType::STACKS_SPEED, 6);
		give(BonusType::PRIMARY_SKILL, 10, BonusSubtypeID(PrimarySkill::ATTACK));
		give(BonusType::PRIMARY_SKILL, 10, BonusSubtypeID(PrimarySkill::DEFENSE));
		give(BonusType::CREATURE_DAMAGE, damageMin, BonusCustomSubtype::creatureDamageMin);
		give(BonusType::CREATURE_DAMAGE, damageMax, BonusCustomSubtype::creatureDamageMax);

		if(shooter)
		{
			give(BonusType::SHOOTER, 1);
			give(BonusType::SHOTS, 16);
		}

		EXPECT_CALL(info, unitBaseAmount()).WillRepeatedly(Return(count));
		EXPECT_CALL(info, unitType()).WillRepeatedly(Return(CreatureID(CreatureID::ARCHER).toCreature()));
		EXPECT_CALL(environment, unitHasAmmoCart(_)).WillRepeatedly(Return(false));

		unit.localInit(&environment);
	}

	void give(BonusType type, int val, const BonusSubtypeID & subtype = {},
		BonusDuration::Type duration = BonusDuration::PERMANENT, int turns = 0)
	{
		auto bonus = std::make_shared<Bonus>(duration, type, BonusSource::SPELL_EFFECT, val, BonusSourceID(), subtype);
		bonus->turnsRemain = turns;
		bonuses.addNewBonus(bonus);
	}

	int64_t value() const
	{
		return value(LIBRARY->combatValues->averageBattle());
	}

	int64_t value(const CombatValueContext & context) const
	{
		return LIBRARY->combatValues->getAIValue(unit, unit.unitType(), context);
	}
};

TEST(CombatValueUnitTest, undamagedUnitIsWorthItsCreaturesTogether)
{
	MeasuredUnit subject(10, 30);
	const auto perCreature = LIBRARY->combatValues->getAIValue(subject.unit, subject.unit.unitType());

	EXPECT_GT(perCreature, 0);
	EXPECT_EQ(subject.unit.estimateCombatValue(), static_cast<uint64_t>(perCreature * MeasuredUnit::count));
}

TEST(CombatValueUnitTest, woundedUnitIsWorthLessThanAWholeOne)
{
	MeasuredUnit subject(10, 30);
	const auto whole = subject.unit.estimateCombatValue();

	// damage() reports back how much it actually took, so it needs a variable to write into
	int64_t damage = subject.unit.getMaxHealth() * 5;
	subject.unit.damage(damage);

	ASSERT_EQ(subject.unit.getCount(), MeasuredUnit::count - 5);
	EXPECT_LT(subject.unit.estimateCombatValue(), whole);
}

/// An enemy that brings none of what the bonuses below are answers to
static CombatValueContext harmlessEnemy()
{
	CombatValueContext context;

	context.meleeShare = 0;
	context.magicPower = 0;
	context.kingShare = {};
	context.allyCrowding = 0;

	return context;
}

/// Bless collapses the damage range onto its top end, so it is worth whatever that range is wide -
/// the pair of these two pins the whole effect to creature stats rather than to a fixed multiplier
TEST(CombatValueEffectTest, blessIsWorthNothingWhenDamageDoesNotVary)
{
	MeasuredUnit subject(20, 20);
	const auto before = subject.value();

	subject.give(BonusType::ALWAYS_MAXIMUM_DAMAGE, 0);

	EXPECT_EQ(subject.value(), before);
}

TEST(CombatValueEffectTest, blessIsWorthMuchWhenDamageVariesWidely)
{
	MeasuredUnit subject(10, 30);
	const auto before = subject.value();

	subject.give(BonusType::ALWAYS_MAXIMUM_DAMAGE, 0);

	EXPECT_GT(subject.value(), before);
}

TEST(CombatValueEffectTest, curseIsWorthWhatBlessIsWorthTheOtherWayRound)
{
	MeasuredUnit subject(10, 30);
	const auto before = subject.value();

	subject.give(BonusType::ALWAYS_MINIMUM_DAMAGE, 0);

	EXPECT_LT(subject.value(), before);
}

TEST(CombatValueEffectTest, blindnessLeavesItsBearerStrikingFeebly)
{
	MeasuredUnit subject(10, 30);
	const auto before = subject.value();

	subject.give(BonusType::GENERAL_ATTACK_REDUCTION, 75);

	EXPECT_LT(subject.value(), before);
}

TEST(CombatValueEffectTest, forgetfulnessSpoilsShootingAlone)
{
	MeasuredUnit shooter(10, 30, true);
	MeasuredUnit fighter(10, 30, false);

	const auto shooterBefore = shooter.value();
	const auto fighterBefore = fighter.value();

	shooter.give(BonusType::FORGETFULL, 50);
	fighter.give(BonusType::FORGETFULL, 50);

	EXPECT_LT(shooter.value(), shooterBefore);
	EXPECT_EQ(fighter.value(), fighterBefore);
}

/// A shooter spoiled past the point of shooting walks up and strikes instead, so however much
/// worse the spoiling gets beyond that, what is left of the unit stays the same
TEST(CombatValueEffectTest, aShooterSpoiledPastShootingIsWorthWhatMeleeIsWorth)
{
	MeasuredUnit spoiled(10, 30, true);
	MeasuredUnit forbidden(10, 30, true);

	spoiled.give(BonusType::FORGETFULL, 90);
	forbidden.give(BonusType::FORGETFULL, 100);

	EXPECT_GT(forbidden.value(), 0);
	EXPECT_EQ(spoiled.value(), forbidden.value());
}

TEST(CombatValueEffectTest, turningBlowsAsideMakesAUnitHarderToKill)
{
	MeasuredUnit subject(10, 30);
	const auto before = subject.value();

	subject.give(BonusType::GENERAL_DAMAGE_REDUCTION, 50, BonusCustomSubtype::damageTypeAll);

	EXPECT_GT(subject.value(), before);
}

/// The same blindness, once lasting a whole battle and once ending on the next blow
TEST(CombatValueEffectTest, aDebuffThatEndsSoonCostsLessThanOneThatStays)
{
	MeasuredUnit lasting(10, 30);
	MeasuredUnit fleeting(10, 30);

	const auto whole = lasting.value();
	ASSERT_EQ(whole, fleeting.value());

	lasting.give(BonusType::GENERAL_ATTACK_REDUCTION, 75);
	fleeting.give(BonusType::GENERAL_ATTACK_REDUCTION, 75, {}, BonusDuration::UNTIL_ATTACK);

	EXPECT_LT(lasting.value(), fleeting.value());
	EXPECT_LT(fleeting.value(), whole);
}


/// Shield turns aside a blow struck in melee, so it is worth what the enemy strikes in melee
TEST(CombatValueContextTest, aShieldIsWorthWhatTheEnemyBringsAgainstIt)
{
	MeasuredUnit subject(10, 30);
	subject.give(BonusType::GENERAL_DAMAGE_REDUCTION, 50, BonusCustomSubtype::damageTypeMelee);

	CombatValueContext closingIn;
	closingIn.meleeShare = 1.0;

	CombatValueContext shootingFromAfar;
	shootingFromAfar.meleeShare = 0.0;

	EXPECT_GT(subject.value(closingIn), subject.value(shootingFromAfar));
}

TEST(CombatValueContextTest, resistingMagicIsWorthNothingWhereNoneIsCast)
{
	MeasuredUnit subject(10, 30);
	const auto silent = harmlessEnemy();
	const auto before = subject.value(silent);

	subject.give(BonusType::MAGIC_RESISTANCE, 50);

	EXPECT_EQ(subject.value(silent), before);

	CombatValueContext caster = silent;
	caster.magicPower = 1.0;

	EXPECT_GT(subject.value(caster), before);
}

TEST(CombatValueContextTest, slayerIsWorthNothingWhereTheEnemyFieldsNoKings)
{
	MeasuredUnit subject(10, 30);
	auto commoners = harmlessEnemy();
	const auto before = subject.value(commoners);

	subject.give(BonusType::SLAYER, 8, {}, BonusDuration::N_TURNS, 3);

	EXPECT_EQ(subject.value(commoners), before);

	CombatValueContext kings = commoners;
	kings.kingShare.fill(1.0);

	EXPECT_GT(subject.value(kings), before);
}

TEST(CombatValueContextTest, berserkCostsNothingWhereTheUnitStandsAlone)
{
	MeasuredUnit subject(10, 30);
	auto alone = harmlessEnemy();
	const auto before = subject.value(alone);

	subject.give(BonusType::ATTACKS_NEAREST_CREATURE, 0, {}, BonusDuration::UNTIL_OWN_ATTACK);

	EXPECT_EQ(subject.value(alone), before);

	CombatValueContext amongAllies = alone;
	amongAllies.allyCrowding = 0.85;

	EXPECT_LT(subject.value(amongAllies), before);
}

/// A context is remembered by its id, so two of them describing the same enemy must answer the
/// same however each was built, and no two describing different ones may
TEST(CombatValueContextTest, contextIdentityFollowsWhatTheContextDescribes)
{
	const auto plain = harmlessEnemy();
	auto rebuilt = harmlessEnemy();

	EXPECT_EQ(plain.id(), rebuilt.id());

	rebuilt.magicPower = 1.0;
	EXPECT_NE(plain.id(), rebuilt.id());

	auto copied = rebuilt;
	EXPECT_EQ(copied.id(), rebuilt.id());

	copied.kingShare[2] = 0.5;
	EXPECT_NE(copied.id(), rebuilt.id());
}

/// A battle of hand-made units, so that what a context reads off one can be dictated outright
class ContextBattle
{
public:
	::test::battle::BattleFake battle;
	::test::battle::UnitsFake units;

	ContextBattle()
	{
		EXPECT_CALL(battle, getUnitsIf(_)).Times(AnyNumber())
			.WillRepeatedly(Invoke(&units, &::test::battle::UnitsFake::getUnitsIf));
		EXPECT_CALL(battle, getTacticDist()).Times(AnyNumber()).WillRepeatedly(Return(0));
		EXPECT_CALL(battle, getSideHero(_)).Times(AnyNumber()).WillRepeatedly(Return(nullptr));
	}

	/// A melee unit of the given worth, optionally a king of the given slayer mastery
	void add(BattleSide side, uint64_t worth, int kingMastery = -1)
	{
		auto & unit = units.add(side);

		unit.makeAlive();
		unit.expectAnyBonusSystemCall();
		EXPECT_CALL(unit, unitSide()).Times(AnyNumber()).WillRepeatedly(Return(side));
		EXPECT_CALL(unit, estimateCombatValue()).Times(AnyNumber()).WillRepeatedly(Return(worth));
		// battleCanShoot reads the creature behind the unit before anything else
		EXPECT_CALL(unit, unitType()).Times(AnyNumber())
			.WillRepeatedly(Return(CreatureID(CreatureID::ARCHER).toCreature()));
		EXPECT_CALL(unit, canShoot()).Times(AnyNumber()).WillRepeatedly(Return(false));

		if(kingMastery >= 0)
			unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::KING,
				BonusSource::CREATURE_ABILITY, kingMastery, BonusSourceID()));
	}
};

TEST(CombatValueContextTest, kingsAreCountedTowardsEveryMasteryThatReachesThem)
{
	ContextBattle fight;

	fight.add(BattleSide::ATTACKER, 100);
	fight.add(BattleSide::DEFENDER, 100);
	fight.add(BattleSide::DEFENDER, 300, 2);

	const auto facingKings = CombatValueContext::against(fight.battle, BattleSide::ATTACKER);

	// a slayer needs advanced mastery to reach this king, and three of the four hundred it faces are it
	EXPECT_DOUBLE_EQ(facingKings.kingShare[0], 0.0);
	EXPECT_DOUBLE_EQ(facingKings.kingShare[1], 0.0);
	EXPECT_DOUBLE_EQ(facingKings.kingShare[2], 0.75);
	EXPECT_DOUBLE_EQ(facingKings.kingShare[3], 0.75);

	// the side those kings stand on faces none of its own
	const auto facingCommoners = CombatValueContext::against(fight.battle, BattleSide::DEFENDER);

	EXPECT_DOUBLE_EQ(facingCommoners.kingShare[3], 0.0);
}

TEST(CombatValueContextTest, crowdingCountsTheAlliesOfTheSideBeingDescribed)
{
	ContextBattle alone;

	alone.add(BattleSide::ATTACKER, 100);
	alone.add(BattleSide::DEFENDER, 100);

	EXPECT_DOUBLE_EQ(CombatValueContext::against(alone.battle, BattleSide::ATTACKER).allyCrowding, 0.0);

	ContextBattle crowd;

	for(int stack = 0; stack < 4; ++stack)
		crowd.add(BattleSide::ATTACKER, 100);
	crowd.add(BattleSide::DEFENDER, 100);

	EXPECT_DOUBLE_EQ(CombatValueContext::against(crowd.battle, BattleSide::ATTACKER).allyCrowding, 0.75);
}

/// The built-in creatures include kings, so the average battle must price a slayer at all
TEST(CombatValueContextTest, theAverageBattleFieldsMoreKingsTheHigherTheMastery)
{
	const auto & average = LIBRARY->combatValues->averageBattle();

	EXPECT_GT(average.kingShare[3], 0.0);

	for(size_t mastery = 1; mastery < average.kingShare.size(); ++mastery)
		EXPECT_GE(average.kingShare[mastery], average.kingShare[mastery - 1]);
}

}
