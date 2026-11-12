/*
 * CreatureConditionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetConditionItemFixture.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/CCreatureHandler.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;

class CreatureConditionTest : public TargetConditionItemTest
{
public:
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _)).Times(0);
		EXPECT_CALL(unitMock, getTreeVersion()).Times(0);
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::create(*LIBRARY->identifiers())->createConfigurable("core", "creature", "ammoCart");
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(CreatureConditionTest, ReceptiveIfMatchesType)
{
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillOnce(Return(CreatureID::AMMO_CART));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(CreatureConditionTest, ImmuneIfTypeMismatch)
{
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillOnce(Return(CreatureID::BALLISTA));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

class CreatureConditionFromJsonTest : public CreatureConditionTest
{
public:
	const CreatureID pikeman = CreatureID(0);
	const CreatureID halberdier = CreatureID(1);

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
	}

	void createSubject(const std::string & creature, bool includeUpgrades)
	{
		JsonNode config;
		config["type"].String() = "creature";
		config["mode"].String() = "normal";
		config["creature"].String() = creature;
		config["includeUpgrades"].Bool() = includeUpgrades;
		config.setModScope("core");

		subject = TargetConditionItemFactory::create(*LIBRARY->identifiers())->createFromJson(config);
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(CreatureConditionFromJsonTest, ReceptiveIfUpgradeIncluded)
{
	createSubject("pikeman", true);
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillRepeatedly(Return(halberdier));
	EXPECT_CALL(unitMock, unitType()).WillRepeatedly(Return(halberdier.toCreature()));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(CreatureConditionFromJsonTest, ImmuneIfUpgradeNotIncluded)
{
	createSubject("pikeman", false);
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillRepeatedly(Return(halberdier));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(CreatureConditionFromJsonTest, ImmuneIfBaseCreatureOfUpgrade)
{
	createSubject("halberdier", true);
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillRepeatedly(Return(pikeman));
	EXPECT_CALL(unitMock, unitType()).WillRepeatedly(Return(pikeman.toCreature()));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

class FactionConditionTest : public TargetConditionItemTest
{
public:
	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		JsonNode config;
		config["type"].String() = "faction";
		config["mode"].String() = "normal";
		config["faction"].String() = "castle";
		config.setModScope("core");

		subject = TargetConditionItemFactory::create(*LIBRARY->identifiers())->createFromJson(config);
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(FactionConditionTest, ReceptiveIfMatchesFaction)
{
	EXPECT_CALL(unitMock, getFactionID()).WillOnce(Return(FactionID::CASTLE));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(FactionConditionTest, ImmuneIfFactionMismatch)
{
	EXPECT_CALL(unitMock, getFactionID()).WillOnce(Return(FactionID::RAMPART));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}

