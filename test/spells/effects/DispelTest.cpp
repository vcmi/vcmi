/*
 * DispelTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

bool operator==(const Bonus & b1, const Bonus & b2)
{
	return b1.duration == b2.duration
		&& b1.type == b2.type
		&& b1.subtype == b2.subtype
		&& b1.source == b2.source
		&& b1.val == b2.val
		&& b1.sid == b2.sid
		&& b1.valType == b2.valType
		&& b1.additionalInfo == b2.additionalInfo
		&& b1.effectRange == b2.effectRange
		&& b1.description == b2.description;
}

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class DispelTest : public Test, public EffectFixture
{
public:
	DispelTest()
		: EffectFixture("core:dispel")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(DispelTest, ApplicableToAliveUnitWithTimedEffect)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["dispelNegative"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, SpellID::CURSE));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillOnce(Return(true));
	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(424242));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, IgnoresOwnEffects)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, SpellID::ANTI_MAGIC));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(SpellID::ANTI_MAGIC));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableToUnitWithNoTimedEffect)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableToDeadUnit)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

class DispelApplyTest : public Test, public EffectFixture
{
public:
	DispelApplyTest()
		: EffectFixture("core:dispel")
	{
	}

	std::array<std::vector<Bonus>, 2> expectedBonus;
	std::array<std::vector<Bonus>, 2> actualBonus;

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(DispelApplyTest, RemovesEffects)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const std::array<uint32_t, 2> unitIds = {567, 765};

	auto & unit0 = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit0, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitIds[0]));

	auto & unit1 = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit1, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitIds[1]));

	{
		auto bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, SpellID::CURSE);
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, SpellID::CURSE);
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::N_TURNS, Bonus::GENERAL_ATTACK_REDUCTION, Bonus::SPELL_EFFECT, 3, SpellID::CURSE);
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 5, SpellID::ANTI_MAGIC);
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::N_TURNS, Bonus::GENERAL_ATTACK_REDUCTION, Bonus::SPELL_EFFECT, 3, SpellID::ANTI_MAGIC);
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);
	}

	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[0]),_)).WillOnce(SaveArg<1>(&actualBonus[0]));
	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[1]),_)).WillOnce(SaveArg<1>(&actualBonus[1]));

	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(424242));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit0, BattleHex());
	target.emplace_back(&unit1, BattleHex());

	subject->apply(battleProxy.get(), rngMock, &mechanicsMock, target);

	EXPECT_THAT(actualBonus[0], UnorderedElementsAreArray(expectedBonus[0]));
	EXPECT_THAT(actualBonus[1], UnorderedElementsAreArray(expectedBonus[1]));
}

}
