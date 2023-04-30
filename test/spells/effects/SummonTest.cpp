/*
 * SummonTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include <vstd/RNG.h>

#include "../../../lib/CCreatureHandler.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

static const CreatureID creature1(CreatureID::AIR_ELEMENTAL);
static const CreatureID creature2(CreatureID::FIRE_ELEMENTAL);

class SummonTest : public TestWithParam<::testing::tuple<CreatureID, bool, bool>>, public EffectFixture
{
public:
	CreatureID toSummon;
	CreatureID otherSummoned;
	bool exclusive;
	bool summonSameUnit;

	const battle::Unit * otherSummonedUnit;

	SummonTest()
		: EffectFixture("core:summon"),
		otherSummonedUnit(nullptr)
	{
	}

	void addOtherSummoned(bool expectAlive)
	{
		auto & unit = unitsFake.add(BattleSide::ATTACKER);

		EXPECT_CALL(unit, unitOwner()).WillRepeatedly(Return(PlayerColor(5)));
		EXPECT_CALL(unit, isClone()).WillRepeatedly(Return(false));
		EXPECT_CALL(unit, creatureId()).WillRepeatedly(Return(otherSummoned));
		EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
		EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
		EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
		EXPECT_CALL(unit, unitSide()).Times(AnyNumber());
		EXPECT_CALL(unit, unitSlot()).WillRepeatedly(Return(SlotID::SUMMONED_SLOT_PLACEHOLDER));

		if(expectAlive)
			EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));
		else
			EXPECT_CALL(unit, alive()).Times(0);

		otherSummonedUnit = &unit;
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		otherSummoned = ::testing::get<0>(GetParam());
		exclusive = ::testing::get<1>(GetParam());
		summonSameUnit = ::testing::get<2>(GetParam());

		toSummon = creature1;

		JsonNode options(JsonNode::JsonType::DATA_STRUCT);
		options["id"].String() = "airElemental";
		options["exclusive"].Bool() = exclusive;
		options["summonSameUnit"].Bool() = summonSameUnit;

		EffectFixture::setupEffect(options);
	}
};

TEST_P(SummonTest, Applicable)
{
	const bool expectedApplicable = !exclusive || otherSummoned == CreatureID() || otherSummoned == toSummon;

	if(otherSummoned != CreatureID())
		addOtherSummoned(false);

	if(otherSummoned != CreatureID() && !expectedApplicable)
		EXPECT_CALL(problemMock, add(_));

	if(exclusive)
		EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));
	else
		EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(0);

	EXPECT_CALL(mechanicsMock, getCasterColor()).WillRepeatedly(Return(PlayerColor(5)));

	EXPECT_EQ(expectedApplicable, subject->applicable(problemMock, &mechanicsMock));
}

TEST_P(SummonTest, Transform)
{
	if(otherSummoned != CreatureID())
		addOtherSummoned(true);

	battleFake->setupEmptyBattlefield();
	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

	EXPECT_CALL(mechanicsMock, getCasterColor()).WillRepeatedly(Return(PlayerColor(5)));

	EffectTarget transformed = subject->transformTarget(&mechanicsMock, Target(), Target());

	EffectTarget expected;

	if(otherSummoned == toSummon && summonSameUnit)
	{
		expected.emplace_back(otherSummonedUnit);
	}
	else
	{
		expected.emplace_back(BattleHex(1));
	}

	EXPECT_THAT(transformed, ContainerEq(expected));
}

INSTANTIATE_TEST_SUITE_P
(
	ByConfig,
	SummonTest,
	Combine
	(
		Values(CreatureID(), creature1, creature2),
		Values(false, true),
		Values(false, true)
	)
);

class SummonApplyTest : public TestWithParam<::testing::tuple<bool, bool>>, public EffectFixture
{
public:
	const CreatureID toSummon = CreatureID::AIR_ELEMENTAL;
	bool permanent = false;
	bool summonByHealth = false;

	const uint32_t unitId = 42;
	const int32_t unitAmount = 123;
	const int32_t unitHealth = 479;
	const int64_t unitTotalHealth = unitAmount * unitHealth;

	const BattleHex unitPosition = BattleHex(5,5);

	std::shared_ptr<::battle::UnitInfo> unitAddInfo;
	std::shared_ptr<battle::UnitFake> acquired;

	StrictMock<CreatureMock> toSummonType;

	SummonApplyTest()
		: EffectFixture("core:summon")
	{
	}

	void setDefaultExpectaions()
	{
		EXPECT_CALL(mechanicsMock, creatures()).Times(AnyNumber());
		EXPECT_CALL(creatureServiceMock, getById(Eq(toSummon))).WillRepeatedly(Return(&toSummonType));
		EXPECT_CALL(toSummonType, getMaxHealth()).WillRepeatedly(Return(unitHealth));

		expectAmountCalculation();
	}

	void expectAmountCalculation()
	{
		InSequence local;
		EXPECT_CALL(mechanicsMock, getEffectPower()).WillOnce(Return(38));
		EXPECT_CALL(mechanicsMock, calculateRawEffectValue(Eq(0), Eq(38))).WillOnce(Return(567));

		if(summonByHealth)
		{
			EXPECT_CALL(mechanicsMock, applySpecificSpellBonus(Eq(567))).WillOnce(Return(unitTotalHealth));
		}
		else
		{
			EXPECT_CALL(mechanicsMock, applySpecificSpellBonus(Eq(567))).WillOnce(Return(unitAmount));
		}
	}

    void onUnitAdded(uint32_t id, const JsonNode & data)
	{
		unitAddInfo->load(id, data);
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		permanent = ::testing::get<0>(GetParam());
		summonByHealth = ::testing::get<1>(GetParam());

		JsonNode options(JsonNode::JsonType::DATA_STRUCT);
		options["id"].String() = "airElemental";
		options["permanent"].Bool() = permanent;
		options["summonByHealth"].Bool() = summonByHealth;

		EffectFixture::setupEffect(options);

		unitAddInfo = std::make_shared<::battle::UnitInfo>();
	}

	void TearDown() override
	{
		acquired.reset();
		unitAddInfo.reset();
	}
};

TEST_P(SummonApplyTest, SpawnsNewUnit)
{
	setDefaultExpectaions();

	EXPECT_CALL(*battleFake, nextUnitId()).WillOnce(Return(unitId));
	EXPECT_CALL(*battleFake, addUnit(Eq(unitId), _)).WillOnce(Invoke(this, &SummonApplyTest::onUnitAdded));
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).Times(1);

	EffectTarget target;
	target.emplace_back(unitPosition);

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(unitAddInfo->count, unitAmount);
	EXPECT_EQ(unitAddInfo->id, unitId);
	EXPECT_EQ(unitAddInfo->position, unitPosition);
	EXPECT_EQ(unitAddInfo->side, mechanicsMock.casterSide);
	EXPECT_EQ(unitAddInfo->summoned, !permanent);
	EXPECT_EQ(unitAddInfo->type, toSummon);
}

TEST_P(SummonApplyTest, UpdatesOldUnit)
{
	setDefaultExpectaions();

	acquired = std::make_shared<battle::UnitFake>();
	acquired->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHealth, 0));
	acquired->redirectBonusesToFake();
	acquired->expectAnyBonusSystemCall();

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHealth, 0));

	{
		EXPECT_CALL(unit, acquire()).WillOnce(Return(acquired));
		EXPECT_CALL(*acquired, heal(Eq(unitTotalHealth), Eq(EHealLevel::OVERHEAL), Eq(permanent ? EHealPower::PERMANENT : EHealPower::ONE_BATTLE)));
		EXPECT_CALL(*acquired, save(_));
		EXPECT_CALL(*battleFake, setUnitState(Eq(unitId), _, _));
	}

	EXPECT_CALL(unit, unitId()).WillOnce(Return(unitId));

	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).Times(1);

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);
}

INSTANTIATE_TEST_SUITE_P
(
	ByConfig,
	SummonApplyTest,
	Combine
	(
		Values(false, true),
		Values(false, true)
	)
);

}
