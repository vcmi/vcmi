/*
 * TimedTest.cpp, part of VCMI engine
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
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/bonuses/BonusParameters.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

// -----------------------------------------------------------------------
// TimedApplyTest — parameterized on cumulative flag
// -----------------------------------------------------------------------

class TimedApplyTest : public TestWithParam<bool>, public EffectFixture
{
public:
	bool cumulative = false;

	// Use a real game spell so SpellID::encode() round-trips correctly.
	const SpellID testSpellId = SpellID::CURSE;
	const int32_t duration = 57;

	TimedApplyTest()
		: EffectFixture("core:timed")
	{
	}

	void setDefaultExpectations()
	{
		unitsFake.setDefaultBonusExpectations();
		EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
		EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
		EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(duration));
		EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		mechanicsMock.caster = nullptr;
		cumulative = GetParam();
	}
};

TEST_P(TimedApplyTest, ChangesBonuses)
{
	Bonus testBonus1(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::KNOWLEDGE));

	Bonus testBonus2(BonusDuration::N_TURNS, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::KNOWLEDGE));
	testBonus2.turnsRemain = 4;

	JsonNode options;
	options["cumulative"].Bool() = cumulative;
	options["bonus"]["test1"] = testBonus1.toJsonNode();
	options["bonus"]["test2"] = testBonus2.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	targetUnit.makeAlive();
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, creatureLevel()).WillRepeatedly(Return(1));

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	std::vector<Bonus> actualBonus;
	std::vector<Bonus> expectedBonus;

	testBonus1.turnsRemain = duration;
	expectedBonus.push_back(testBonus1);
	expectedBonus.push_back(testBonus2);

	for(auto & bonus : expectedBonus)
	{
		bonus.source = BonusSource::SPELL_EFFECT;
		bonus.sid = BonusSourceID(testSpellId);
	}

	auto accumulate = [&actualBonus](uint32_t, const std::vector<Bonus> & b)
	{
		actualBonus.insert(actualBonus.end(), b.begin(), b.end());
	};

	if(cumulative)
	{
		EXPECT_CALL(*battleFake, addUnitBonus(Eq(unitId), _)).Times(2).WillRepeatedly(Invoke(accumulate));
	}
	else
	{
		EXPECT_CALL(*battleFake, updateUnitBonus(Eq(unitId), _)).Times(2).WillRepeatedly(Invoke(accumulate));
	}

	setDefaultExpectations();

	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(2);

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_THAT(actualBonus, UnorderedElementsAreArray(expectedBonus));
}

INSTANTIATE_TEST_SUITE_P
(
	ByCumulative,
	TimedApplyTest,
	Values(false, true)
);

// -----------------------------------------------------------------------
// TimedTest — unit effect tests
// -----------------------------------------------------------------------

class TimedTest : public Test, public EffectFixture
{
public:
	const SpellID testSpellId = SpellID::CURSE;

	TimedTest()
		: EffectFixture("core:timed")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		mechanicsMock.caster = nullptr;
	}
};

TEST_F(TimedTest, ApplySkipsDeadUnit)
{
	Bonus b(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK));
	JsonNode options;
	options["bonus"]["test"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit, alive()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(0);

	subject->apply(&serverMock, &mechanicsMock, target);
}

TEST_F(TimedTest, ApplyMultipleTargets)
{
	Bonus b(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK));
	JsonNode options;
	options["bonus"]["test"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	const uint32_t unitId0 = 10;
	const uint32_t unitId1 = 20;

	auto & unit0 = unitsFake.add(BattleSide::ATTACKER);
	unit0.makeAlive();
	EXPECT_CALL(unit0, unitId()).WillRepeatedly(Return(unitId0));
	EXPECT_CALL(unit0, creatureLevel()).WillRepeatedly(Return(1));

	auto & unit1 = unitsFake.add(BattleSide::DEFENDER);
	unit1.makeAlive();
	EXPECT_CALL(unit1, unitId()).WillRepeatedly(Return(unitId1));
	EXPECT_CALL(unit1, creatureLevel()).WillRepeatedly(Return(1));

	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit0, BattleHex());
	target.emplace_back(&unit1, BattleHex());

	EXPECT_CALL(*battleFake, updateUnitBonus(Eq(unitId0), _)).Times(1);
	EXPECT_CALL(*battleFake, updateUnitBonus(Eq(unitId1), _)).Times(1);
	// Lua sends one SetStackEffect per unit, not batched
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(2);

	subject->apply(&serverMock, &mechanicsMock, target);
}

TEST_F(TimedTest, ConvertBonusUsesSpellDurationWhenTurnsRemainIsZero)
{
	Bonus b(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK));
	// b.turnsRemain == 0 by default; should be replaced by spell's effect duration
	JsonNode options;
	options["bonus"]["test"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	const int32_t spellDuration = 57;

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(1));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(spellDuration));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].turnsRemain, spellDuration);
}

TEST_F(TimedTest, ConvertBonusKeepsExplicitTurnsRemain)
{
	const si16 customDuration = 4;
	Bonus b(BonusDuration::N_TURNS, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK));
	b.turnsRemain = customDuration;
	JsonNode options;
	options["bonus"]["test"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(1));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(57)); // different from customDuration
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].turnsRemain, customDuration);
}

TEST_F(TimedTest, ConvertBonusInvertsShieldDamageReduction)
{
	Bonus b(BonusDuration::PERMANENT, BonusType::GENERAL_DAMAGE_REDUCTION, BonusSource::OTHER, 30, BonusSourceID());
	JsonNode options;
	options["bonus"]["shield"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(1));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(SpellID(SpellID::SHIELD).getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].val, 70); // 100 - 30
}

TEST_F(TimedTest, ConvertBonusBindSetsCasterUnitId)
{
	Bonus b(BonusDuration::PERMANENT, BonusType::BIND_EFFECT, BonusSource::OTHER, 0, BonusSourceID());
	JsonNode options;
	options["bonus"]["bind"] = b.toJsonNode();
	options.setModScope(ModScope::scopeBuiltin());
	setupEffect(options);

	const int32_t casterUnitId = 99;
	auto & casterUnit = unitsFake.add(BattleSide::ATTACKER);
	mechanicsMock.caster = &casterUnit;
	EXPECT_CALL(casterUnit, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(casterUnitId)));
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&casterUnit));

	auto & targetUnit = unitsFake.add(BattleSide::DEFENDER);
	targetUnit.makeAlive();
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(targetUnit, creatureLevel()).WillRepeatedly(Return(1));

	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(SpellID(SpellID::BIND).getNum())));
	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	ASSERT_TRUE(actualBonus[0].parameters);
	EXPECT_EQ(actualBonus[0].parameters->toNumber(), casterUnitId);
}

// -----------------------------------------------------------------------
// TimedHeroSpecialtyTest — hero caster specialty tests
// -----------------------------------------------------------------------

class TimedHeroSpecialtyTest : public Test, public EffectFixture
{
public:
	std::unique_ptr<CGHeroInstance> hero;
	const SpellID testSpellId = SpellID::BLESS;

	TimedHeroSpecialtyTest()
		: EffectFixture("core:timed")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		hero = std::make_unique<CGHeroInstance>(nullptr);
		mechanicsMock.caster = hero.get();
		EXPECT_CALL(mechanicsMock, getHeroCaster()).WillRepeatedly(Return(hero.get()));
		EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
		EXPECT_CALL(spellStub, getJsonKey()).WillRepeatedly(Return(SpellID::encode(testSpellId.getNum())));
	}

	void setupSingleBonusEffect(int val = 10)
	{
		Bonus b(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, val, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK));
		JsonNode options;
		options["bonus"]["test"] = b.toJsonNode();
		options.setModScope(ModScope::scopeBuiltin());
		setupEffect(options);
	}

	void addPeculiarEnchant(int32_t mode = 0)
	{
		auto specialtyBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPECIAL_PECULIAR_ENCHANT, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(testSpellId));
		specialtyBonus->parameters = std::make_shared<BonusParameters>(mode);
		hero->addNewBonus(specialtyBonus);
	}

	void addAddValueEnchant(int32_t addValue)
	{
		auto specialtyBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPECIAL_ADD_VALUE_ENCHANT, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(testSpellId));
		specialtyBonus->parameters = std::make_shared<BonusParameters>(addValue);
		hero->addNewBonus(specialtyBonus);
	}

	void addFixedValueEnchant(int32_t fixedValue)
	{
		auto specialtyBonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPECIAL_FIXED_VALUE_ENCHANT, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(testSpellId));
		specialtyBonus->parameters = std::make_shared<BonusParameters>(fixedValue);
		hero->addNewBonus(specialtyBonus);
	}

	std::vector<Bonus> applyAndCapture(int32_t creatureLevel)
	{
		auto & unit = unitsFake.add(BattleSide::ATTACKER);
		unit.makeAlive();
		EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
		EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(creatureLevel));
		unitsFake.setDefaultBonusExpectations();

		EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
		EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

		Target target;
		target.emplace_back(&unit, BattleHex());

		std::vector<Bonus> actualBonus;
		EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
		EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

		subject->apply(&serverMock, &mechanicsMock, target);
		return actualBonus;
	}
};

TEST_F(TimedHeroSpecialtyTest, PeculiarEnchantAddsPowerForTier1And2)
{
	setupSingleBonusEffect(10);
	addPeculiarEnchant(0); // normal mode

	EXPECT_CALL(mechanicsMock, isNegativeSpell()).WillRepeatedly(Return(false));
	auto bonuses = applyAndCapture(/*creatureLevel=*/1);

	ASSERT_EQ(bonuses.size(), 1u);
	EXPECT_EQ(bonuses[0].val, 13); // 10 + 3
}

TEST_F(TimedHeroSpecialtyTest, PeculiarEnchantAddsPowerForTier5And6)
{
	setupSingleBonusEffect(10);
	addPeculiarEnchant(0);

	EXPECT_CALL(mechanicsMock, isNegativeSpell()).WillRepeatedly(Return(false));
	auto bonuses = applyAndCapture(/*creatureLevel=*/5);

	ASSERT_EQ(bonuses.size(), 1u);
	EXPECT_EQ(bonuses[0].val, 11); // 10 + 1
}

TEST_F(TimedHeroSpecialtyTest, PeculiarEnchantNegatesPowerForNegativeSpell)
{
	setupSingleBonusEffect(-10); // negative base for a debuff
	addPeculiarEnchant(0);

	EXPECT_CALL(mechanicsMock, isNegativeSpell()).WillRepeatedly(Return(true));
	auto bonuses = applyAndCapture(/*creatureLevel=*/1); // tier 1-2 → power 3, negated to -3

	ASSERT_EQ(bonuses.size(), 1u);
	EXPECT_EQ(bonuses[0].val, -13); // -10 + (-3)
}

TEST_F(TimedHeroSpecialtyTest, AddValueEnchantAddsToBonus)
{
	setupSingleBonusEffect(10);
	addAddValueEnchant(5);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(1));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].val, 15); // 10 + 5
}

TEST_F(TimedHeroSpecialtyTest, FixedValueEnchantOverridesBonus)
{
	setupSingleBonusEffect(10);
	addFixedValueEnchant(7);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(42u));
	EXPECT_CALL(unit, creatureLevel()).WillRepeatedly(Return(1));
	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(1));
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex());

	std::vector<Bonus> actualBonus;
	EXPECT_CALL(*battleFake, updateUnitBonus(_, _)).WillOnce(SaveArg<1>(&actualBonus));
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(actualBonus.size(), 1u);
	EXPECT_EQ(actualBonus[0].val, 7);
}

}
