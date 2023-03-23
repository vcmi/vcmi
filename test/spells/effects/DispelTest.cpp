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

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class DispelFixture : public Test, public EffectFixture
{
public:
	const SpellID positiveID = SpellID(4242);
	const SpellID negativeID = SpellID(4243);
	const SpellID neutralID = SpellID(4244);

	StrictMock<SpellMock> positiveSpell;
	StrictMock<SpellMock> negativeSpell;
	StrictMock<SpellMock> neutralSpell;

	DispelFixture()
		: EffectFixture("core:dispel")
	{
	}

	void setDefaultExpectaions()
	{
		EXPECT_CALL(mechanicsMock, spells()).Times(AnyNumber());

		EXPECT_CALL(spellServiceMock, getById(Eq(positiveID))).WillRepeatedly(Return(&positiveSpell));
		EXPECT_CALL(positiveSpell, getIndex()).WillRepeatedly(Return(positiveID.toEnum()));
		EXPECT_CALL(positiveSpell, getPositiveness()).WillRepeatedly(Return(true));
		EXPECT_CALL(positiveSpell, isAdventure()).WillRepeatedly(Return(false));

		EXPECT_CALL(spellServiceMock, getById(Eq(negativeID))).WillRepeatedly(Return(&negativeSpell));
		EXPECT_CALL(negativeSpell, getIndex()).WillRepeatedly(Return(negativeID.toEnum()));
		EXPECT_CALL(negativeSpell, getPositiveness()).WillRepeatedly(Return(false));
		EXPECT_CALL(negativeSpell, isAdventure()).WillRepeatedly(Return(false));

		EXPECT_CALL(spellServiceMock, getById(Eq(neutralID))).WillRepeatedly(Return(&neutralSpell));
		EXPECT_CALL(neutralSpell, getIndex()).WillRepeatedly(Return(neutralID.toEnum()));
		EXPECT_CALL(neutralSpell, getPositiveness()).WillRepeatedly(Return(boost::logic::indeterminate));
		EXPECT_CALL(neutralSpell, isAdventure()).WillRepeatedly(Return(false));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

class DispelTest : public DispelFixture
{
};

TEST_F(DispelTest, ApplicableToAliveUnitWithTimedEffect)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["dispelNegative"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, negativeID.toEnum()));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(neutralID.toEnum()));

	setDefaultExpectaions();
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

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, neutralID.toEnum()));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(neutralID.toEnum()));

	setDefaultExpectaions();
	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableToUnitWithNoTimedEffect)
{
	EffectFixture::setupEffect(JsonNode());
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
	EffectFixture::setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

class DispelApplyTest : public DispelFixture
{
public:
	std::array<std::vector<Bonus>, 2> expectedBonus;
	std::array<std::vector<Bonus>, 2> actualBonus;
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
		auto bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, negativeID.toEnum());
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 1, negativeID.toEnum());
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::N_TURNS, Bonus::GENERAL_ATTACK_REDUCTION, Bonus::SPELL_EFFECT, 3, negativeID.toEnum());
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, 5, positiveID.toEnum());
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(Bonus::N_TURNS, Bonus::GENERAL_ATTACK_REDUCTION, Bonus::SPELL_EFFECT, 3, positiveID.toEnum());
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);
	}

	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[0]),_)).WillOnce(SaveArg<1>(&actualBonus[0]));
	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[1]),_)).WillOnce(SaveArg<1>(&actualBonus[1]));

	EXPECT_CALL(mechanicsMock, getSpellIndex()).Times(AtLeast(1)).WillRepeatedly(Return(neutralID.toEnum()));

	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect *>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setDefaultExpectaions();
	unitsFake.setDefaultBonusExpectations();
	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&unit0, BattleHex());
	target.emplace_back(&unit1, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_THAT(actualBonus[0], UnorderedElementsAreArray(expectedBonus[0]));
	EXPECT_THAT(actualBonus[1], UnorderedElementsAreArray(expectedBonus[1]));
}

}
