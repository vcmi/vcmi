/*
 * TargetConditionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vstd/RNG.h>

#include "../../lib/NetPacksBase.h"
#include "../../lib/spells/TargetCondition.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include "mock/mock_spells_Mechanics.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_battle_Unit.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;

class ItemMock : public TargetConditionItem
{
public:
	MOCK_CONST_METHOD2(isReceptive, bool(const Mechanics *, const battle::Unit *));

	MOCK_METHOD1(setInverted, void(bool));
	MOCK_METHOD1(setExclusive, void(bool));

	MOCK_CONST_METHOD0(isExclusive, bool());
};

class FactoryMock : public TargetConditionItemFactory
{
public:
	MOCK_CONST_METHOD0(createAbsoluteLevel, Object());
	MOCK_CONST_METHOD0(createAbsoluteSpell, Object());
	MOCK_CONST_METHOD0(createElemental, Object());
	MOCK_CONST_METHOD0(createResistance, Object());
	MOCK_CONST_METHOD0(createNormalLevel, Object());
	MOCK_CONST_METHOD0(createNormalSpell, Object());
	MOCK_CONST_METHOD1(createFromJsonStruct, Object(const JsonNode &));
	MOCK_CONST_METHOD3(createConfigurable, Object(std::string, std::string, std::string));
	MOCK_CONST_METHOD0(createReceptiveFeature, Object());
	MOCK_CONST_METHOD0(createImmunityNegation, Object());
};

class TargetConditionTest : public Test
{
public:
	TargetCondition subject;

	StrictMock<MechanicsMock> mechanicsMock;
	StrictMock<UnitMock> targetMock;
	NiceMock<FactoryMock> factoryMock;

	void setupSubject(const JsonNode & configuration)
	{
		JsonDeserializer handler(nullptr, configuration);

		subject.serializeJson(handler, &factoryMock);
	}

	void redirectFactoryToStub()
	{
		auto itemStub = std::make_shared<StrictMock<ItemMock>>();
		EXPECT_CALL(*itemStub, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillRepeatedly(Return(true));
		EXPECT_CALL(*itemStub, isExclusive()).WillRepeatedly(Return(false));

		ON_CALL(factoryMock, createAbsoluteLevel()).WillByDefault(Return(itemStub));
		ON_CALL(factoryMock, createAbsoluteSpell()).WillByDefault(Return(itemStub));
		ON_CALL(factoryMock, createElemental()).WillByDefault(Return(itemStub));
		ON_CALL(factoryMock, createResistance()).WillByDefault(Return(itemStub));
		ON_CALL(factoryMock, createNormalLevel()).WillByDefault(Return(itemStub));
		ON_CALL(factoryMock, createNormalSpell()).WillByDefault(Return(itemStub));

		auto negationItemStub = std::make_shared<StrictMock<ItemMock>>();
		EXPECT_CALL(*negationItemStub, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillRepeatedly(Return(false));

		ON_CALL(factoryMock, createReceptiveFeature()).WillByDefault(Return(negationItemStub));
		ON_CALL(factoryMock, createImmunityNegation()).WillByDefault(Return(negationItemStub));
	}

	TargetConditionTest(){}
	~TargetConditionTest(){}
protected:
	void SetUp() override
	{
	}
private:
};

TEST_F(TargetConditionTest, SerializesCorrectly)
{
	redirectFactoryToStub();

	auto normalInvertedItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*normalInvertedItem, isReceptive(_,_)).WillRepeatedly(Return(true));
	EXPECT_CALL(*normalInvertedItem, setInverted(Eq(true))).Times(1);
	EXPECT_CALL(*normalInvertedItem, setExclusive(Eq(true))).Times(1);
	EXPECT_CALL(*normalInvertedItem, isExclusive()).WillRepeatedly(Return(true));

	EXPECT_CALL(factoryMock, createConfigurable(Eq(""), Eq("bonus"), Eq("NON_LIVING"))).WillOnce(Return(normalInvertedItem));

	auto absoluteItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*absoluteItem, isReceptive(_,_)).WillRepeatedly(Return(true));
	EXPECT_CALL(*absoluteItem, setInverted(Eq(false))).Times(1);
	EXPECT_CALL(*absoluteItem, setExclusive(Eq(false))).Times(1);
	EXPECT_CALL(*absoluteItem, isExclusive()).WillRepeatedly(Return(true));

	EXPECT_CALL(factoryMock, createConfigurable(Eq(""), Eq("bonus"), Eq("SIEGE_WEAPON"))).WillOnce(Return(absoluteItem));

	auto normalItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*normalItem, isReceptive(_,_)).WillRepeatedly(Return(true));
	EXPECT_CALL(*normalItem, setInverted(Eq(false))).Times(1);
	EXPECT_CALL(*normalItem, setExclusive(Eq(true))).Times(1);
	EXPECT_CALL(*normalItem, isExclusive()).WillRepeatedly(Return(true));

	EXPECT_CALL(factoryMock, createConfigurable(Eq(""), Eq("bonus"), Eq("UNDEAD"))).WillOnce(Return(normalItem));

	JsonNode config(JsonNode::JsonType::DATA_STRUCT);
	config["noneOf"]["bonus.NON_LIVING"].String() = "normal";
	config["anyOf"]["bonus.SIEGE_WEAPON"].String() = "absolute";
	config["allOf"]["bonus.UNDEAD"].String() = "normal";

	setupSubject(config);

	EXPECT_THAT(subject.normal, Contains(normalInvertedItem));
	EXPECT_THAT(subject.normal, Contains(normalItem));
	EXPECT_THAT(subject.absolute, Contains(absoluteItem));
}

TEST_F(TargetConditionTest, CreatesSpecialConditions)
{
	redirectFactoryToStub();
	EXPECT_CALL(factoryMock, createAbsoluteLevel()).Times(1);
	EXPECT_CALL(factoryMock, createAbsoluteSpell()).Times(1);
	EXPECT_CALL(factoryMock, createElemental()).Times(1);
	EXPECT_CALL(factoryMock, createResistance()).Times(1);
	EXPECT_CALL(factoryMock, createNormalLevel()).Times(1);
	EXPECT_CALL(factoryMock, createNormalSpell()).Times(1);

	EXPECT_CALL(factoryMock, createReceptiveFeature()).Times(1);
	EXPECT_CALL(factoryMock, createImmunityNegation()).Times(1);

	setupSubject(JsonNode());
}

TEST_F(TargetConditionTest, StoresAbsoluteLevelCondition)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createAbsoluteLevel()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.absolute, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresAbsoluteSpellCondition)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createAbsoluteSpell()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.absolute, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresElementalCondition)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createElemental()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.normal, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresNormalSpellCondition)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createNormalSpell()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.normal, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresNormalLevelCondition)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createNormalLevel()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.normal, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresReceptiveFeature)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createReceptiveFeature()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.negation, Contains(itemStub));
}

TEST_F(TargetConditionTest, StoresImmunityNegation)
{
	redirectFactoryToStub();
	auto itemStub = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(factoryMock, createImmunityNegation()).WillOnce(Return(itemStub));
	setupSubject(JsonNode());
	EXPECT_THAT(subject.negation, Contains(itemStub));
}

TEST_F(TargetConditionTest, ChecksAbsoluteCondition)
{
	auto item = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*item, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*item, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));

	subject.absolute.push_back(item);

    EXPECT_TRUE(subject.isReceptive(&mechanicsMock, &targetMock));
}

TEST_F(TargetConditionTest, ChecksNegationCondition)
{
	auto item = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*item, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));

	subject.negation.push_back(item);

    EXPECT_TRUE(subject.isReceptive(&mechanicsMock, &targetMock));
}

TEST_F(TargetConditionTest, ChecksNormalCondition)
{
	auto item = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*item, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*item, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));

	subject.normal.push_back(item);

    EXPECT_TRUE(subject.isReceptive(&mechanicsMock, &targetMock));
}

TEST_F(TargetConditionTest, ChecksAbsoluteConditionFirst)
{
	redirectFactoryToStub();
	setupSubject(JsonNode());

	auto absoluteItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*absoluteItem, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*absoluteItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(false));
	subject.absolute.push_back(absoluteItem);

	auto negationItem = std::make_shared<StrictMock<ItemMock>>();
	subject.negation.push_back(negationItem);

	auto normalItem = std::make_shared<StrictMock<ItemMock>>();
	subject.normal.push_back(normalItem);

	EXPECT_FALSE(subject.isReceptive(&mechanicsMock, &targetMock));
}

TEST_F(TargetConditionTest, ChecksNegationConditionSecond)
{
	redirectFactoryToStub();
	setupSubject(JsonNode());

	auto absoluteItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*absoluteItem, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*absoluteItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));
	subject.absolute.push_back(absoluteItem);

	auto negationItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*negationItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));
	subject.negation.push_back(negationItem);

	auto normalItem = std::make_shared<StrictMock<ItemMock>>();
	subject.normal.push_back(normalItem);

	EXPECT_TRUE(subject.isReceptive(&mechanicsMock, &targetMock));
}

TEST_F(TargetConditionTest, ChecksNormalConditionThird)
{
	redirectFactoryToStub();
	setupSubject(JsonNode());

	auto absoluteItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*absoluteItem, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*absoluteItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(true));
	subject.absolute.push_back(absoluteItem);

	auto negationItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*negationItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(false));
	subject.negation.push_back(negationItem);

	auto normalItem = std::make_shared<StrictMock<ItemMock>>();
	EXPECT_CALL(*normalItem, isExclusive()).WillOnce(Return(true));
	EXPECT_CALL(*normalItem, isReceptive(Eq(&mechanicsMock), Eq(&targetMock))).WillOnce(Return(false));
	subject.normal.push_back(normalItem);

	EXPECT_FALSE(subject.isReceptive(&mechanicsMock, &targetMock));
}

}

