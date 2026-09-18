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

/// A battle unit of a real creature, so that the model has actual stats to read
class CombatValueUnitTest : public Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;
	battle::CUnitStateDetached subject;

	static constexpr int32_t count = 20;

	CombatValueUnitTest()
		: subject(&infoMock, &bonusMock)
	{
		const auto * archer = CreatureID(CreatureID::ARCHER).toCreature();

		// a detached unit carries no creature node, so the stats the model reads are supplied here
		const auto giveBonus = [this](BonusType type, int value, const BonusSubtypeID & subtype = {})
		{
			bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, type,
				BonusSource::CREATURE_ABILITY, value, BonusSourceID(), subtype));
		};

		giveBonus(BonusType::STACK_HEALTH, archer->getBaseHitPoints());
		giveBonus(BonusType::STACKS_SPEED, archer->getBaseSpeed());
		giveBonus(BonusType::PRIMARY_SKILL, archer->getBaseAttack(), BonusSubtypeID(PrimarySkill::ATTACK));
		giveBonus(BonusType::PRIMARY_SKILL, archer->getBaseDefense(), BonusSubtypeID(PrimarySkill::DEFENSE));
		giveBonus(BonusType::CREATURE_DAMAGE, archer->getBaseDamageMin(), BonusCustomSubtype::creatureDamageMin);
		giveBonus(BonusType::CREATURE_DAMAGE, archer->getBaseDamageMax(), BonusCustomSubtype::creatureDamageMax);

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(count));
		EXPECT_CALL(infoMock, unitType()).WillRepeatedly(Return(archer));
		EXPECT_CALL(envMock, unitHasAmmoCart(_)).WillRepeatedly(Return(false));

		subject.localInit(&envMock);
	}
};

TEST_F(CombatValueUnitTest, undamagedUnitIsWorthItsCreaturesTogether)
{
	const auto perCreature = LIBRARY->combatValues->getAIValue(subject, subject.unitType());

	EXPECT_GT(perCreature, 0);
	EXPECT_EQ(subject.estimateCombatValue(), static_cast<uint64_t>(perCreature * count));
}

TEST_F(CombatValueUnitTest, woundedUnitIsWorthLessThanAWholeOne)
{
	const auto whole = subject.estimateCombatValue();

	int64_t damage = subject.getMaxHealth() * 5;
	subject.damage(damage);

	ASSERT_EQ(subject.getCount(), count - 5);
	EXPECT_LT(subject.estimateCombatValue(), whole);
}

/// A unit whose stats the test chooses, so that a spell effect can be judged by what it does to the
/// value rather than by a number written down here
struct MeasuredUnit
{
	UnitInfoMock info;
	UnitEnvironmentMock environment;
	BonusBearerMock bonuses;
	battle::CUnitStateDetached unit;

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

		EXPECT_CALL(info, unitBaseAmount()).WillRepeatedly(Return(20));
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

TEST(CombatValueEffectTest, turningBlowsAsideMakesAUnitHarderToKill)
{
	MeasuredUnit subject(10, 30);
	const auto before = subject.value();

	subject.give(BonusType::GENERAL_DAMAGE_REDUCTION, 50, BonusCustomSubtype::damageTypeAll);

	EXPECT_GT(subject.value(), before);
}

/// The same effect, once lasting a whole battle and once ending on the next blow
TEST(CombatValueEffectTest, anEffectThatEndsSoonIsWorthLessThanOneThatStays)
{
	MeasuredUnit lasting(10, 30);
	MeasuredUnit fleeting(10, 30);

	ASSERT_EQ(lasting.value(), fleeting.value());

	lasting.give(BonusType::GENERAL_ATTACK_REDUCTION, 75);
	fleeting.give(BonusType::GENERAL_ATTACK_REDUCTION, 75, {}, BonusDuration::UNTIL_ATTACK);

	EXPECT_LT(lasting.value(), fleeting.value());
	EXPECT_LT(fleeting.value(), lasting.value() * 2);
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

}
