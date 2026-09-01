/*
 * DemonSummonTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include "../../../lib/json/JsonNode.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class DemonSummonTest : public Test, public EffectFixture
{
public:
	StrictMock<CreatureMock> demonType;
	const std::string demonId = "core:airElemental";
	const int32_t demonMaxHealth = 100;

	DemonSummonTest()
		: EffectFixture("core:demonSummon")
	{}

	void setCreatureExpectations()
	{
		EXPECT_CALL(creatureServiceMock, getByName(demonId)).WillRepeatedly(Return(&demonType));
		EXPECT_CALL(demonType, getMaxHealth()).WillRepeatedly(Return(demonMaxHealth));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		JsonNode options;
		options["id"].String() = demonId;
		EffectFixture::setupEffect(options);
	}
};

TEST_F(DemonSummonTest, NoCorpses_NotApplicable)
{
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), Ref(problemMock)))
		.WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(DemonSummonTest, LivingUnit_NotApplicable)
{
	auto & unit = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), Ref(problemMock)))
		.WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(DemonSummonTest, ValidCorpse_Applicable)
{
	const int64_t effectValue = 200;

	auto & corpse = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(corpse, alive()).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, isGhost()).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, getPosition()).WillRepeatedly(Return(BattleHex(5, 5)));
	EXPECT_CALL(corpse, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, unitBaseAmount()).WillRepeatedly(Return(10));
	EXPECT_CALL(corpse, getTotalHealth()).WillRepeatedly(Return(500));

	setCreatureExpectations();

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(effectValue), Eq(&corpse))).WillRepeatedly(Return(200));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&corpse))).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

class DemonSummonApplyTest : public TestWithParam<bool>, public EffectFixture
{
public:
	StrictMock<CreatureMock> demonType;
	const std::string demonId = "core:airElemental";
	const uint32_t newUnitId = 42;
	const uint32_t corpseId = 4242;
	const int32_t demonMaxHealth = 200;
	const int32_t corpseBaseAmount = 10;
	const int64_t corpseTotalHealth = 1000;
	const int64_t effectValue = 400;
	const BattleHex corpsePosition = BattleHex(5, 5);
	const int32_t expectedAmount = 2;

	bool permanent;
	std::shared_ptr<::battle::UnitInfo> unitAddInfo;

	DemonSummonApplyTest()
		: EffectFixture("core:demonSummon")
	{}

	void onUnitAdded(uint32_t id, const JsonNode & data)
	{
		unitAddInfo->load(id, data);
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		permanent = GetParam();

		JsonNode options;
		options["id"].String() = demonId;
		options["permanent"].Bool() = permanent;
		EffectFixture::setupEffect(options);

		unitAddInfo = std::make_shared<::battle::UnitInfo>();
	}

	void TearDown() override
	{
		unitAddInfo.reset();
	}
};

TEST_P(DemonSummonApplyTest, SpawnsNewUnitAndRemovesCorpse)
{
	auto & corpse = unitsFake.add(BattleSide::DEFENDER);
	EXPECT_CALL(corpse, alive()).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, isGhost()).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, getPosition()).WillRepeatedly(Return(corpsePosition));
	EXPECT_CALL(corpse, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(corpse, unitBaseAmount()).WillRepeatedly(Return(corpseBaseAmount));
	EXPECT_CALL(corpse, getTotalHealth()).WillRepeatedly(Return(corpseTotalHealth));
	EXPECT_CALL(corpse, unitId()).WillRepeatedly(Return(corpseId));

	EXPECT_CALL(creatureServiceMock, getByName(demonId)).WillRepeatedly(Return(&demonType));
	EXPECT_CALL(demonType, getId()).WillRepeatedly(Return(CreatureID::AIR_ELEMENTAL));
	EXPECT_CALL(demonType, getMaxHealth()).WillRepeatedly(Return(demonMaxHealth));
	EXPECT_CALL(demonType, getJsonKey()).WillRepeatedly(Return(demonId));
	EXPECT_CALL(demonType, isDoubleWide()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(effectValue), Eq(&corpse))).WillRepeatedly(Return(effectValue));

	battleFake->setupEmptyBattlefield();

	EXPECT_CALL(*battleFake, nextUnitId()).WillOnce(Return(newUnitId));
	EXPECT_CALL(*battleFake, addUnit(Eq(newUnitId), _)).WillOnce(Invoke(this, &DemonSummonApplyTest::onUnitAdded));
	EXPECT_CALL(*battleFake, removeUnit(Eq(corpseId))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(2);

	Target target;
	target.emplace_back(&corpse, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(unitAddInfo->count, expectedAmount);
	EXPECT_EQ(unitAddInfo->side, mechanicsMock.casterSide);
	EXPECT_EQ(unitAddInfo->position, corpsePosition);
	EXPECT_EQ(unitAddInfo->summoned, !permanent);
}

INSTANTIATE_TEST_SUITE_P
(
	ByPermanentFlag,
	DemonSummonApplyTest,
	Values(false, true)
);

}
