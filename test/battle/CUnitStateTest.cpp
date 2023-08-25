/*
 * CUnitStateTest.cpp, part of VCMI engine
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
#include "../../lib/battle/CUnitState.h"
#include "../../lib/CCreatureHandler.h"

namespace test
{
using namespace ::testing;

static const int32_t DEFAULT_HP = 123;
static const int32_t DEFAULT_AMOUNT = 100;
static const int32_t DEFAULT_SPEED = 10;
static const BattleHex DEFAULT_POSITION = BattleHex(5, 5);
static const int DEFAULT_ATTACK = 58;
static const int DEFAULT_DEFENCE = 63;

class UnitStateTest : public Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;

	const CCreature * pikeman;

	battle::CUnitStateDetached subject;

	bool hasAmmoCart;

	UnitStateTest()
		:infoMock(),
		envMock(),
		bonusMock(),
		subject(&infoMock, &bonusMock),
		hasAmmoCart(false)
	{
		pikeman = CreatureID(0).toCreature();
	}

	void setDefaultExpectations()
	{
		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACKS_SPEED, BonusSource::CREATURE_ABILITY, DEFAULT_SPEED, 0));

		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CREATURE_ABILITY, DEFAULT_ATTACK, 0, static_cast<int>(PrimarySkill::ATTACK)));
		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CREATURE_ABILITY, DEFAULT_DEFENCE, 0, static_cast<int>(PrimarySkill::DEFENSE)));

		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, DEFAULT_HP, 0));

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(DEFAULT_AMOUNT));
		EXPECT_CALL(infoMock, unitType()).WillRepeatedly(Return(pikeman));

		EXPECT_CALL(envMock, unitHasAmmoCart(_)).WillRepeatedly(Return(hasAmmoCart));
	}

	void makeShooter(int32_t ammo)
	{
		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SHOOTER, BonusSource::CREATURE_ABILITY, 1, 0));
		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SHOTS, BonusSource::CREATURE_ABILITY, ammo, 0));
	}

	void initUnit()
	{
		subject.localInit(&envMock);
		subject.position = DEFAULT_POSITION;
	}
};

TEST_F(UnitStateTest, initialRegular)
{
	setDefaultExpectations();
	initUnit();

	EXPECT_TRUE(subject.alive());
	EXPECT_TRUE(subject.ableToRetaliate());
	EXPECT_FALSE(subject.isGhost());
	EXPECT_FALSE(subject.isDead());
	EXPECT_FALSE(subject.isTurret());
	EXPECT_TRUE(subject.isValidTarget(true));
	EXPECT_TRUE(subject.isValidTarget(false));

	EXPECT_FALSE(subject.isClone());
	EXPECT_FALSE(subject.hasClone());

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_FALSE(subject.canShoot());
	EXPECT_FALSE(subject.isShooter());

	EXPECT_EQ(subject.getCount(), DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getFirstHPleft(), DEFAULT_HP);
	EXPECT_EQ(subject.getKilled(), 0);
	EXPECT_EQ(subject.getAvailableHealth(), DEFAULT_HP * DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getTotalHealth(), subject.getAvailableHealth());

	EXPECT_EQ(subject.getPosition(), DEFAULT_POSITION);

	EXPECT_EQ(subject.getInitiative(), DEFAULT_SPEED);
	EXPECT_EQ(subject.getInitiative(123456), DEFAULT_SPEED);

	EXPECT_TRUE(subject.canMove());
	EXPECT_TRUE(subject.canMove(123456));
	EXPECT_FALSE(subject.defended());
	EXPECT_FALSE(subject.defended(123456));
	EXPECT_FALSE(subject.moved());
	EXPECT_FALSE(subject.moved(123456));
	EXPECT_TRUE(subject.willMove());
	EXPECT_TRUE(subject.willMove(123456));
	EXPECT_FALSE(subject.waited());
	EXPECT_FALSE(subject.waited(123456));

	EXPECT_EQ(subject.getTotalAttacks(true), 1);
	EXPECT_EQ(subject.getTotalAttacks(false), 1);
}

TEST_F(UnitStateTest, canShoot)
{
	setDefaultExpectations();
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_FALSE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}

TEST_F(UnitStateTest, canShootWithAmmoCart)
{
	hasAmmoCart = true;
	setDefaultExpectations();
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}

TEST_F(UnitStateTest, getAttack)
{
	setDefaultExpectations();

	EXPECT_EQ(subject.getAttack(false), DEFAULT_ATTACK);
	EXPECT_EQ(subject.getAttack(true), DEFAULT_ATTACK);
}

TEST_F(UnitStateTest, getDefense)
{
	setDefaultExpectations();

	EXPECT_EQ(subject.getDefense(false), DEFAULT_DEFENCE);
	EXPECT_EQ(subject.getDefense(true), DEFAULT_DEFENCE);
}

TEST_F(UnitStateTest, attackWithFrenzy)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::IN_FRENZY, BonusSource::SPELL_EFFECT, 50, 0));

	int expectedAttack = static_cast<int>(DEFAULT_ATTACK + 0.5 * DEFAULT_DEFENCE);

	EXPECT_EQ(subject.getAttack(false), expectedAttack);
	EXPECT_EQ(subject.getAttack(true), expectedAttack);
}

TEST_F(UnitStateTest, defenceWithFrenzy)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::IN_FRENZY, BonusSource::SPELL_EFFECT, 50, 0));

	int expectedDefence = 0;

	EXPECT_EQ(subject.getDefense(false), expectedDefence);
	EXPECT_EQ(subject.getDefense(true), expectedDefence);
}

TEST_F(UnitStateTest, additionalAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::SPELL_EFFECT, 41, 0);

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.getTotalAttacks(false), 42);
	EXPECT_EQ(subject.getTotalAttacks(true), 42);
}

TEST_F(UnitStateTest, additionalMeleeAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::SPELL_EFFECT, 41, 0);
		bonus->effectRange = BonusLimitEffect::ONLY_MELEE_FIGHT;

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.getTotalAttacks(false), 42);
	EXPECT_EQ(subject.getTotalAttacks(true), 1);
}

TEST_F(UnitStateTest, additionalRangedAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::SPELL_EFFECT, 41, 0);
		bonus->effectRange = BonusLimitEffect::ONLY_DISTANCE_FIGHT;

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.getTotalAttacks(false), 1);
	EXPECT_EQ(subject.getTotalAttacks(true), 42);
}

TEST_F(UnitStateTest, getMinDamage)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_DAMAGE, BonusSource::SPELL_EFFECT, 30, 0, 0);
		bonusMock.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_DAMAGE, BonusSource::SPELL_EFFECT, -20, 0, 1);
		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.getMinDamage(false), 10);
	EXPECT_EQ(subject.getMinDamage(true), 10);
}

TEST_F(UnitStateTest, getMaxDamage)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_DAMAGE, BonusSource::SPELL_EFFECT, 30, 0, 0);
		bonusMock.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_DAMAGE, BonusSource::SPELL_EFFECT, -20, 0, 2);
		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.getMaxDamage(false), 10);
	EXPECT_EQ(subject.getMaxDamage(true), 10);
}

}
