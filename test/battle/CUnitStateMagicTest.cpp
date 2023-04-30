/*
 * CUnitStateMagicTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_BonusBearer.h"
#include "mock/mock_spells_Spell.h"
#include "mock/mock_UnitInfo.h"
#include "mock/mock_UnitEnvironment.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/CCreatureHandler.h"

class UnitStateMagicTest : public ::testing::Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;
	::testing::StrictMock<spells::SpellMock> spellMock;

	battle::CUnitStateDetached subject;

	const int32_t DEFAULT_AMOUNT = 100;
	const int32_t DEFAULT_SPELL_INDEX = 42000000; //could be anything but should be far out of spellhandler bounds
	const int32_t DEFAULT_SCHOOL_LEVEL = 2;

	UnitStateMagicTest()
		:infoMock(),
		envMock(),
		bonusMock(),
		spellMock(),
		subject(&infoMock, &bonusMock)
	{
	}

	void setDefaultExpectations()
	{
		using namespace testing;

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(DEFAULT_AMOUNT));

		EXPECT_CALL(spellMock, getIndex()).WillRepeatedly(Return(DEFAULT_SPELL_INDEX));
	}

	void initUnit()
	{
		subject.localInit(&envMock);
	}

	void makeNormalCaster()
	{
		bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELLCASTER, BonusSource::CREATURE_ABILITY, DEFAULT_SCHOOL_LEVEL, 0, DEFAULT_SPELL_INDEX));
	}
};

TEST_F(UnitStateMagicTest, initialNormal)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CASTS, BonusSource::CREATURE_ABILITY, 567, 0));

	initUnit();

	EXPECT_TRUE(subject.canCast());
	EXPECT_TRUE(subject.isCaster());

	EXPECT_EQ(subject.casts.total(), 567);
	EXPECT_EQ(subject.casts.available(), 567);
}

TEST_F(UnitStateMagicTest, schoolLevelByDefault)
{
	setDefaultExpectations();
	initUnit();

	EXPECT_EQ(subject.getSpellSchoolLevel(&spellMock, nullptr), 0);
}

TEST_F(UnitStateMagicTest, schoolLevelForNormalCaster)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();
	EXPECT_EQ(subject.getSpellSchoolLevel(&spellMock, nullptr), DEFAULT_SCHOOL_LEVEL);
}

TEST_F(UnitStateMagicTest, effectLevelForNormalCaster)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();
	EXPECT_EQ(subject.getEffectLevel(&spellMock), DEFAULT_SCHOOL_LEVEL);
}

TEST_F(UnitStateMagicTest, spellBonus)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();
	EXPECT_EQ(subject.getSpellBonus(&spellMock, 12345, &subject), 12345);
}

TEST_F(UnitStateMagicTest, specificSpellBonus)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();
	EXPECT_EQ(subject.getSpecificSpellBonus(&spellMock, 12345), 12345);
}

TEST_F(UnitStateMagicTest, effectPower)
{
	setDefaultExpectations();
	initUnit();

	const int32_t EFFECT_POWER = 12 * 100;

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_SPELL_POWER, BonusSource::CREATURE_ABILITY, EFFECT_POWER, 0));

	makeNormalCaster();
	EXPECT_EQ(subject.getEffectPower(&spellMock), 12 * DEFAULT_AMOUNT);
}

TEST_F(UnitStateMagicTest, enchantPowerByDefault)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();

	EXPECT_EQ(subject.getEnchantPower(&spellMock), 3);
}

TEST_F(UnitStateMagicTest, enchantPower)
{
	setDefaultExpectations();
	initUnit();

	const int32_t ENCHANT_POWER = 42;

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CREATURE_ENCHANT_POWER, BonusSource::CREATURE_ABILITY, ENCHANT_POWER, 0));

	makeNormalCaster();

	EXPECT_EQ(subject.getEnchantPower(&spellMock), ENCHANT_POWER);
}

TEST_F(UnitStateMagicTest, effectValueByDefault)
{
	setDefaultExpectations();
	initUnit();

	makeNormalCaster();
	EXPECT_EQ(subject.getEffectValue(&spellMock), 0);
}

TEST_F(UnitStateMagicTest, effectValue)
{
	setDefaultExpectations();
	initUnit();

	const int32_t EFFECT_VALUE = 456;

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPECIFIC_SPELL_POWER, BonusSource::CREATURE_ABILITY, EFFECT_VALUE, 0, DEFAULT_SPELL_INDEX));

	makeNormalCaster();
	EXPECT_EQ(subject.getEffectValue(&spellMock), EFFECT_VALUE * DEFAULT_AMOUNT);
}

TEST_F(UnitStateMagicTest, getOwner)
{
	using namespace testing;

	setDefaultExpectations();
	initUnit();

	PlayerColor player(123);

	PlayerColor otherPlayer(124);

	EXPECT_CALL(infoMock, unitOwner()).WillRepeatedly(Return(player));

	ON_CALL(envMock, unitEffectiveOwner(Eq(&subject))).WillByDefault(Return(otherPlayer));

	EXPECT_CALL(envMock, unitEffectiveOwner(_));

	EXPECT_EQ(subject.getCasterOwner(), otherPlayer);
}

TEST_F(UnitStateMagicTest, spendMana)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CASTS, BonusSource::CREATURE_ABILITY, 1, 0));

	initUnit();

	EXPECT_TRUE(subject.canCast());

	subject.spendMana(nullptr, 1);

	EXPECT_FALSE(subject.canCast());
}




//TODO:getCasterName
//TODO:getCastDescription
//TODO:spendMana
