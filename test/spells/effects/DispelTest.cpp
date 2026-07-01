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
#include "../../../lib/json/JsonNode.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class DispelFixture : public Test, public EffectFixture
{
public:
	// Real game spell IDs so SpellID::encode() round-trips through LIBRARY correctly.
	const SpellID positiveID  = SpellID::BLESS;
	const SpellID negativeID  = SpellID::CURSE;
	const SpellID neutralID   = SpellID::BLIND;
	const SpellID persistentID = SpellID::HASTE;
	const SpellID adventureID = SpellID::SLOW;
	const SpellID currentID   = SpellID::DISPEL;

	StrictMock<SpellMock> positiveSpell;
	StrictMock<SpellMock> negativeSpell;
	StrictMock<SpellMock> neutralSpell;
	StrictMock<SpellMock> persistentSpell;
	StrictMock<SpellMock> adventureSpell;
	StrictMock<SpellMock> currentSpell;

	DispelFixture()
		: EffectFixture("core:dispel")
	{
	}

	// Called by every test that has at least one SPELL_EFFECT bonus on a unit,
	// so the Lua filter's LIBRARY:getSpellByName() calls are satisfied.
	void setDefaultExpectations()
	{
		EXPECT_CALL(spellServiceMock, getByName(Eq(SpellID::encode(positiveID.getNum())))).WillRepeatedly(Return(&positiveSpell));
		EXPECT_CALL(positiveSpell, isPersistent()).WillRepeatedly(Return(false));
		EXPECT_CALL(positiveSpell, isAdventure()).WillRepeatedly(Return(false));
		EXPECT_CALL(positiveSpell, isPositive()).WillRepeatedly(Return(true));
		EXPECT_CALL(positiveSpell, isNegative()).WillRepeatedly(Return(false));
		EXPECT_CALL(positiveSpell, isNeutral()).WillRepeatedly(Return(false));

		EXPECT_CALL(spellServiceMock, getByName(Eq(SpellID::encode(negativeID.getNum())))).WillRepeatedly(Return(&negativeSpell));
		EXPECT_CALL(negativeSpell, isPersistent()).WillRepeatedly(Return(false));
		EXPECT_CALL(negativeSpell, isAdventure()).WillRepeatedly(Return(false));
		EXPECT_CALL(negativeSpell, isPositive()).WillRepeatedly(Return(false));
		EXPECT_CALL(negativeSpell, isNegative()).WillRepeatedly(Return(true));
		EXPECT_CALL(negativeSpell, isNeutral()).WillRepeatedly(Return(false));

		EXPECT_CALL(spellServiceMock, getByName(Eq(SpellID::encode(neutralID.getNum())))).WillRepeatedly(Return(&neutralSpell));
		EXPECT_CALL(neutralSpell, isPersistent()).WillRepeatedly(Return(false));
		EXPECT_CALL(neutralSpell, isAdventure()).WillRepeatedly(Return(false));
		EXPECT_CALL(neutralSpell, isPositive()).WillRepeatedly(Return(false));
		EXPECT_CALL(neutralSpell, isNegative()).WillRepeatedly(Return(false));
		EXPECT_CALL(neutralSpell, isNeutral()).WillRepeatedly(Return(true));

		EXPECT_CALL(spellServiceMock, getByName(Eq(SpellID::encode(persistentID.getNum())))).WillRepeatedly(Return(&persistentSpell));
		EXPECT_CALL(persistentSpell, isPersistent()).WillRepeatedly(Return(true));

		EXPECT_CALL(spellServiceMock, getByName(Eq(SpellID::encode(adventureID.getNum())))).WillRepeatedly(Return(&adventureSpell));
		EXPECT_CALL(adventureSpell, isPersistent()).WillRepeatedly(Return(false));
		EXPECT_CALL(adventureSpell, isAdventure()).WillRepeatedly(Return(true));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		// getDispelableBonuses() always reads the current spell key, even when the
		// bonus list is empty, so set this up once for all tests in the fixture.
		EXPECT_CALL(mechanicsMock, getSpell()).Times(AnyNumber()).WillRepeatedly(Return(&currentSpell));
		EXPECT_CALL(currentSpell, getJsonKey()).Times(AnyNumber())
			.WillRepeatedly(Return(SpellID::encode(currentID.getNum())));
	}
};

class DispelTest : public DispelFixture
{
};

TEST_F(DispelTest, ApplicableToAliveUnitWithTimedEffect)
{
	{
		JsonNode config;
		config["dispelNegative"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(negativeID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, IgnoresOwnEffects)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	// Bonus from the currently-cast spell — must be excluded by the filter.
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(currentID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, IgnoresPersistentEffects)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(persistentID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableToUnitWithNoTimedEffect)
{
	EffectFixture::setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableToDeadUnit)
{
	EffectFixture::setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, IgnoresAdventureSpellEffects)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(adventureID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, IgnoresNonSpellEffectSourceBonuses)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, ApplicableToUnitWithPositiveEffect)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(positiveID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, ApplicableToUnitWithNeutralEffect)
{
	{
		JsonNode config;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(neutralID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableWhenDispelNegativeAndOnlyPositiveEffect)
{
	{
		JsonNode config;
		config["dispelNegative"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(positiveID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DispelTest, NotApplicableWhenDispelPositiveAndOnlyNegativeEffect)
{
	{
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(negativeID)));

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtMost(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
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
		JsonNode config;
		config["dispelPositive"].Bool() = true;
		config["dispelNegative"].Bool() = true;
		config["dispelNeutral"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const std::array<uint32_t, 2> unitIds = {567, 765};

	auto & unit0 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit0, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitIds[0]));
	EXPECT_CALL(unit0, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	auto & unit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit1, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitIds[1]));
	EXPECT_CALL(unit1, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(negativeID));
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 1, BonusSourceID(negativeID));
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::N_TURNS, BonusType::GENERAL_ATTACK_REDUCTION, BonusSource::SPELL_EFFECT, 3, BonusSourceID(negativeID));
		expectedBonus[0].emplace_back(*bonus);
		unit0.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::SPELL_EFFECT, 5, BonusSourceID(positiveID));
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);

		bonus = std::make_shared<Bonus>(BonusDuration::N_TURNS, BonusType::GENERAL_ATTACK_REDUCTION, BonusSource::SPELL_EFFECT, 3, BonusSourceID(positiveID));
		expectedBonus[1].emplace_back(*bonus);
		unit1.addNewBonus(bonus);
	}

	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[0]),_)).WillOnce(SaveArg<1>(&actualBonus[0]));
	EXPECT_CALL(*battleFake, removeUnitBonus(Eq(unitIds[1]),_)).WillOnce(SaveArg<1>(&actualBonus[1]));

	// Lua script sends one SetStackEffect per unit (unlike the batched C++ version).
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(2);
	EXPECT_CALL(serverMock, describeChanges()).Times(AnyNumber()).WillRepeatedly(Return(false));

	setDefaultExpectations();
	unitsFake.setDefaultBonusExpectations();
	setupDefaultRNG();

	Target target;
	target.emplace_back(&unit0, BattleHex());
	target.emplace_back(&unit1, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_THAT(actualBonus[0], UnorderedElementsAreArray(expectedBonus[0]));
	EXPECT_THAT(actualBonus[1], UnorderedElementsAreArray(expectedBonus[1]));
}

}
