/*
 * ObstacleTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"

#include "../../mock/mock_CStack.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class ObstacleTest : public Test, public EffectFixture
{
public:
	ObstacleTest()
		: EffectFixture("core:obstacle")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(ObstacleTest, ApplicableGeneralByDefault)
{
	setupEffect(JsonNode());

	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).Times(0);

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(ObstacleTest, ApplicableTargetWhenHexClear)
{
	setupEffect(JsonNode());
	battleFake->setupEmptyBattlefield();

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, requiresClearTiles()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	Target target;
	target.emplace_back(BattleHex(5, 5));

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(ObstacleTest, NotApplicableTargetWhenHexBlockedByUnit)
{
	setupEffect(JsonNode());
	battleFake->setupEmptyBattlefield();

	const BattleHex hex(5, 5);
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(hex));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isGhost()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, requiresClearTiles()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), _))
		.WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	Target target;
	target.emplace_back(hex);

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(ObstacleTest, NotApplicableTargetWhenEmptyTarget)
{
	setupEffect(JsonNode());

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, requiresClearTiles()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), _))
		.WillRepeatedly(Return(false));

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, Target()));
}

TEST_F(ObstacleTest, NotApplicableWhenHiddenAndHideNativeWithEnemyNativeStack)
{
	JsonNode config;
	config["hidden"].Bool() = true;
	config["hideNative"].Bool() = true;
	setupEffect(config);

	MockCStack nativeStack;
	nativeStack.makeNativeOnSide(BattleSide::DEFENDER);
	battleFake->setupNativeStacks({ &nativeStack }, TerrainId(0));

	mechanicsMock.casterSide = BattleSide::ATTACKER;
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), _))
		.WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(ObstacleTest, ApplicableWhenHiddenAndHideNativeWithoutEnemyNativeStack)
{
	JsonNode config;
	config["hidden"].Bool() = true;
	config["hideNative"].Bool() = true;
	setupEffect(config);

	battleFake->setupNativeStacks(TStacks(), TerrainId(0));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).Times(0);

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(ObstacleTest, MassiveSkipsHexCheck)
{
	setupEffect(JsonNode());

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, Target()));
}

class ObstacleApplyTest : public Test, public EffectFixture
{
public:
	ObstacleApplyTest()
		: EffectFixture("core:obstacle")
	{
	}

	BattleObstaclesChanged capturedPack;

	void captureObstaclePack()
	{
		EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_)))
			.Times(AnyNumber())
			.WillRepeatedly(Invoke([this](const BattleObstaclesChanged & pack)
			{
				for(const auto & change : pack.changes)
					capturedPack.changes.push_back(change);
			}));
	}

	void setDefaultApplyExpectations()
	{
		EXPECT_CALL(mechanicsMock, getEffectPower()).WillRepeatedly(Return(5));
		EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(2));
		EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
		EXPECT_CALL(spellStub, getId()).WillRepeatedly(Return(SpellID(13)));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(ObstacleApplyTest, PlacesSingleObstacleAtTarget)
{
	JsonNode config;
	config["turnsRemaining"].Integer() = 5;
	setupEffect(config);

	battleFake->setupEmptyBattlefield();
	setDefaultApplyExpectations();

	const BattleHex hex(7, 5);
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	captureObstaclePack();

	Target target;
	target.emplace_back(hex);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::ADD);

	SpellCreatedObstacle deserialized;
	deserialized.fromInfo(capturedPack.changes[0]);
	EXPECT_EQ(deserialized.pos, hex);
	EXPECT_EQ(deserialized.casterSide, BattleSide::ATTACKER);
	EXPECT_EQ(deserialized.turnsRemaining, 5);
	EXPECT_EQ(deserialized.customSize.size(), 1u);
	EXPECT_TRUE(deserialized.customSize.contains(hex));
}

TEST_F(ObstacleApplyTest, PlacesObstacleWithMultiHexShape)
{
	JsonNode config;
	JsonNode shape;
	JsonNode firstShapeEntry;
	firstShapeEntry.Vector().emplace_back();  // empty direction list ⇒ NONE
	JsonNode secondShapeEntry;
	JsonNode trDir;
	trDir.String() = "TR";
	secondShapeEntry.Vector().push_back(trDir);
	shape.Vector().push_back(firstShapeEntry);
	shape.Vector().push_back(secondShapeEntry);
	config["attacker"]["shape"] = shape;
	setupEffect(config);

	battleFake->setupEmptyBattlefield();
	setDefaultApplyExpectations();

	const BattleHex hex(7, 5);
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	captureObstaclePack();

	Target target;
	target.emplace_back(hex);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 1u);

	SpellCreatedObstacle deserialized;
	deserialized.fromInfo(capturedPack.changes[0]);

	BattleHex trNeighbour = hex;
	trNeighbour.moveInDirection(BattleHex::EDir::TOP_RIGHT, false);

	EXPECT_EQ(deserialized.customSize.size(), 2u);
	EXPECT_TRUE(deserialized.customSize.contains(hex));
	EXPECT_TRUE(deserialized.customSize.contains(trNeighbour));
}

TEST_F(ObstacleApplyTest, PlacesOneObstaclePerTargetHex)
{
	setupEffect(JsonNode());

	battleFake->setupEmptyBattlefield();
	setDefaultApplyExpectations();

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	captureObstaclePack();

	Target target;
	target.emplace_back(BattleHex(5, 5));
	target.emplace_back(BattleHex(6, 5));
	target.emplace_back(BattleHex(7, 5));

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 3u);
	for(const auto & change : capturedPack.changes)
		EXPECT_EQ(change.operation, BattleChanges::EOperation::ADD);
}

TEST_F(ObstacleApplyTest, PatchCountLimitedByAvailableTiles)
{
	JsonNode config;
	config["patchCount"].Integer() = 10;
	setupEffect(config);

	battleFake->setupEmptyBattlefield();
	setDefaultApplyExpectations();

	const BattleHex blockedHex(5, 5);
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(blockedHex));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isGhost()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));
	setupDefaultRNG();
	captureObstaclePack();

	Target target;
	target.emplace_back(blockedHex);
	target.emplace_back(BattleHex(6, 5));
	target.emplace_back(BattleHex(7, 5));

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 2u);
}

TEST_F(ObstacleApplyTest, NoServerCallWhenNoAvailableTiles)
{
	JsonNode config;
	config["patchCount"].Integer() = 4;
	setupEffect(config);

	battleFake->setupEmptyBattlefield();

	const BattleHex blockedHex(5, 5);
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(blockedHex));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isGhost()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));
	setupDefaultRNG();
	EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_))).Times(0);

	Target target;
	target.emplace_back(blockedHex);

	subject->apply(&serverMock, &mechanicsMock, target);
}

TEST_F(ObstacleApplyTest, UsesDefenderSideOptions)
{
	JsonNode config;
	config["attacker"]["shape"].Vector().emplace_back();
	config["attacker"]["shape"].Vector()[0].Vector().emplace_back();
	config["attacker"]["shape"].Vector()[0].Vector()[0].String() = "TL";

	config["defender"]["shape"].Vector().emplace_back();
	config["defender"]["shape"].Vector()[0].Vector().emplace_back();
	config["defender"]["shape"].Vector()[0].Vector()[0].String() = "TR";

	setupEffect(config);

	battleFake->setupEmptyBattlefield();
	setDefaultApplyExpectations();

	mechanicsMock.casterSide = BattleSide::DEFENDER;

	const BattleHex hex(7, 5);
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	captureObstaclePack();

	Target target;
	target.emplace_back(hex);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::ADD);

	SpellCreatedObstacle deserialized;
	deserialized.fromInfo(capturedPack.changes[0]);

	BattleHex defenderShapeNeighbour = hex;
	defenderShapeNeighbour.moveInDirection(BattleHex::EDir::TOP_RIGHT, false);

	EXPECT_EQ(deserialized.casterSide, BattleSide::DEFENDER);
	EXPECT_EQ(deserialized.customSize.size(), 1u);
	EXPECT_TRUE(deserialized.customSize.contains(defenderShapeNeighbour));
}

}
