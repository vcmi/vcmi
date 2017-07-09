/*
 * CHealthTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include <boost/test/unit_test.hpp>
#include "../../lib/CStack.h"

class CUnitHealthInfoMock : public IUnitHealthInfo
{
public:
	CUnitHealthInfoMock():
		maxHealth(123),
		baseAmount(300)
	{}

	int32_t maxHealth;
	int32_t baseAmount;

	int32_t unitMaxHealth() const override
	{
		return maxHealth;
	};

	int32_t unitBaseAmount() const override
	{
		return baseAmount;
	};
};

static void checkTotal(const CHealth & health, const CUnitHealthInfoMock & mock)
{
	BOOST_CHECK_EQUAL(health.total(), mock.maxHealth * mock.baseAmount);
}

static void checkEmptyHealth(const CHealth & health, const CUnitHealthInfoMock & mock)
{
	checkTotal(health, mock);
	BOOST_CHECK_EQUAL(health.getCount(), 0);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), 0);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);
	BOOST_CHECK_EQUAL(health.available(), 0);
}

static void checkFullHealth(const CHealth & health, const CUnitHealthInfoMock & mock)
{
	checkTotal(health, mock);
	BOOST_CHECK_EQUAL(health.getCount(), mock.baseAmount);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), mock.maxHealth);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);
	BOOST_CHECK_EQUAL(health.available(), mock.maxHealth * mock.baseAmount);
}

static void checkDamage(CHealth & health, const int32_t initialDamage, const int32_t expectedDamage)
{
	int32_t damage = initialDamage;
	health.damage(damage);
	BOOST_CHECK_EQUAL(damage, expectedDamage);
}

static void checkNormalDamage(CHealth & health, const int32_t initialDamage)
{
	checkDamage(health, initialDamage, initialDamage);
}

static void checkNoDamage(CHealth & health, const int32_t initialDamage)
{
	checkDamage(health, initialDamage, 0);
}

static void checkHeal(CHealth & health, EHealLevel level, EHealPower power, const int32_t initialHeal, const int32_t expectedHeal)
{
	int32_t heal = initialHeal;
	health.heal(heal, level, power);
	BOOST_CHECK_EQUAL(heal, expectedHeal);
}

BOOST_AUTO_TEST_SUITE(CHealthTest_Suite)

BOOST_AUTO_TEST_CASE(empty)
{
	CUnitHealthInfoMock uhi;
	CHealth health(&uhi);
	checkEmptyHealth(health, uhi);

	health.init();
	checkFullHealth(health, uhi);

	health.reset();
	checkEmptyHealth(health, uhi);
}

BOOST_AUTO_TEST_CASE(damage)
{
	CUnitHealthInfoMock uhi;
	CHealth health(&uhi);
	health.init();

	checkNormalDamage(health, 0);
	checkFullHealth(health, uhi);

	checkNormalDamage(health, uhi.maxHealth - 1);
	BOOST_CHECK_EQUAL(health.getCount(), uhi.baseAmount);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), 1);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);

	checkNormalDamage(health, 1);
	BOOST_CHECK_EQUAL(health.getCount(), uhi.baseAmount - 1);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), uhi.maxHealth);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);

	checkNormalDamage(health, uhi.maxHealth * (uhi.baseAmount - 1));
	checkEmptyHealth(health, uhi);

	checkNoDamage(health, 1337);
	checkEmptyHealth(health, uhi);
}

BOOST_AUTO_TEST_CASE(heal)
{
	CUnitHealthInfoMock uhi;
	CHealth health(&uhi);
	health.init();
	checkNormalDamage(health, 99);
	BOOST_CHECK_EQUAL(health.getCount(), uhi.baseAmount);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), uhi.maxHealth-99);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);

	checkHeal(health, EHealLevel::HEAL, EHealPower::PERMANENT, 9, 9);
	BOOST_CHECK_EQUAL(health.getCount(), uhi.baseAmount);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), uhi.maxHealth-90);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);

	checkHeal(health, EHealLevel::RESURRECT, EHealPower::ONE_BATTLE, 40, 40);
	BOOST_CHECK_EQUAL(health.getCount(), uhi.baseAmount);
	BOOST_CHECK_EQUAL(health.getFirstHPleft(), uhi.maxHealth-50);
	BOOST_CHECK_EQUAL(health.getResurrected(), 0);

	checkHeal(health, EHealLevel::OVERHEAL, EHealPower::PERMANENT, 50, 50);
	checkFullHealth(health, uhi);
}

BOOST_AUTO_TEST_SUITE_END()

