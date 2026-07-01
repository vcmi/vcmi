/*
 * RemoveObstacleTest.cpp, part of VCMI engine
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

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

namespace
{

std::shared_ptr<CObstacleInstance> makeUsualObstacle(si32 uniqueID)
{
	auto obstacle = std::make_shared<CObstacleInstance>();
	obstacle->obstacleType = CObstacleInstance::USUAL;
	obstacle->uniqueID = uniqueID;
	return obstacle;
}

std::shared_ptr<CObstacleInstance> makeUsualObstacleAt(si32 uniqueID, const BattleHex & pos, si32 obstacleTypeID = 0)
{
	auto obstacle = std::make_shared<CObstacleInstance>();
	obstacle->obstacleType = CObstacleInstance::USUAL;
	obstacle->uniqueID = uniqueID;
	obstacle->pos = pos;
	obstacle->ID = obstacleTypeID;
	return obstacle;
}

std::shared_ptr<CObstacleInstance> makeAbsoluteObstacle(si32 uniqueID)
{
	auto obstacle = std::make_shared<CObstacleInstance>();
	obstacle->obstacleType = CObstacleInstance::ABSOLUTE_OBSTACLE;
	obstacle->uniqueID = uniqueID;
	return obstacle;
}

std::shared_ptr<SpellCreatedObstacle> makeSpellObstacle(si32 uniqueID, const BattleHex & pos, SpellID spell)
{
	auto obstacle = std::make_shared<SpellCreatedObstacle>();
	obstacle->obstacleType = CObstacleInstance::SPELL_CREATED;
	obstacle->uniqueID = uniqueID;
	obstacle->pos = pos;
	obstacle->ID = spell.num;
	obstacle->customSize.insert(pos);
	return obstacle;
}

}

class RemoveObstacleTest : public Test, public EffectFixture
{
public:
	RemoveObstacleTest()
		: EffectFixture("core:removeObstacle")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(RemoveObstacleTest, ApplicableGeneralWhenMatchingObstacleExists)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	IBattleInfo::ObstacleCList obstacles = { makeUsualObstacle(1) };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).Times(0);

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(RemoveObstacleTest, NotApplicableGeneralWhenNoObstacles)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), _)).WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(RemoveObstacleTest, NotApplicableGeneralWhenFlagsDoNotMatch)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	IBattleInfo::ObstacleCList obstacles = { makeAbsoluteObstacle(1) };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), _)).WillOnce(Return(false));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(RemoveObstacleTest, ApplicableTargetNonMassive)
{
	JsonNode config;
	config["removeAllSpells"].Bool() = true;
	setupEffect(config);

	const BattleHex hex(5, 5);
	IBattleInfo::ObstacleCList obstacles = { makeSpellObstacle(1, hex, SpellID(SpellID::FIRE_WALL)) };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));

	Target target;
	target.emplace_back(hex);

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(RemoveObstacleTest, ApplicableTargetUsualNonMassive)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	const BattleHex hex(5, 5);
	IBattleInfo::ObstacleCList obstacles = { makeUsualObstacleAt(1, hex, 0) };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));

	Target target;
	target.emplace_back(hex);

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(RemoveObstacleTest, NotApplicableTargetWhenHexEmpty)
{
	JsonNode config;
	config["removeAllSpells"].Bool() = true;
	setupEffect(config);

	IBattleInfo::ObstacleCList obstacles = { makeSpellObstacle(1, BattleHex(5, 5), SpellID(SpellID::FIRE_WALL)) };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));

	Target target;
	target.emplace_back(BattleHex(10, 9));

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

class RemoveObstacleApplyTest : public Test, public EffectFixture
{
public:
	RemoveObstacleApplyTest()
		: EffectFixture("core:removeObstacle")
	{
	}

	BattleObstaclesChanged capturedPack;

	void captureObstaclePack()
	{
		EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_)))
			.WillRepeatedly(Invoke([this](const BattleObstaclesChanged & pack)
			{
				for(const auto & change : pack.changes)
					capturedPack.changes.push_back(change);
			}));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(RemoveObstacleApplyTest, RemovesUsualObstacleMassive)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	auto usual = makeUsualObstacle(1);
	auto absolute = makeAbsoluteObstacle(2);
	IBattleInfo::ObstacleCList obstacles = { usual, absolute };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].id, static_cast<uint32_t>(usual->uniqueID));
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::REMOVE);
}

TEST_F(RemoveObstacleApplyTest, RemovesAbsoluteWhenFlagSet)
{
	JsonNode config;
	config["removeAbsolute"].Bool() = true;
	setupEffect(config);

	auto usual = makeUsualObstacle(1);
	auto absolute = makeAbsoluteObstacle(2);
	IBattleInfo::ObstacleCList obstacles = { usual, absolute };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].id, static_cast<uint32_t>(absolute->uniqueID));
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::REMOVE);
}

TEST_F(RemoveObstacleApplyTest, RemovesAllSpellObstaclesWithFlag)
{
	JsonNode config;
	config["removeAllSpells"].Bool() = true;
	setupEffect(config);

	auto spellA = makeSpellObstacle(1, BattleHex(5, 5), SpellID(SpellID::FIRE_WALL));
	auto spellB = makeSpellObstacle(2, BattleHex(6, 6), SpellID(SpellID::FORCE_FIELD));
	auto usual = makeUsualObstacle(3);
	IBattleInfo::ObstacleCList obstacles = { spellA, spellB, usual };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 2u);
	std::set<uint32_t> removedIds;
	for(const auto & change : capturedPack.changes)
	{
		removedIds.insert(change.id);
		EXPECT_EQ(change.operation, BattleChanges::EOperation::REMOVE);
	}
	EXPECT_EQ(removedIds, (std::set<uint32_t>{ static_cast<uint32_t>(spellA->uniqueID), static_cast<uint32_t>(spellB->uniqueID) }));
}

TEST_F(RemoveObstacleApplyTest, RemovesNamedSpellsOnly)
{
	JsonNode config;
	JsonNode fireWallId;
	fireWallId.String() = "core:fireWall";
	config["removeSpells"].Vector().push_back(fireWallId);
	setupEffect(config);

	auto fireWall = makeSpellObstacle(1, BattleHex(5, 5), SpellID(SpellID::FIRE_WALL));
	auto forceField = makeSpellObstacle(2, BattleHex(6, 6), SpellID(SpellID::FORCE_FIELD));
	IBattleInfo::ObstacleCList obstacles = { fireWall, forceField };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	subject->apply(&serverMock, &mechanicsMock, Target());

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].id, static_cast<uint32_t>(fireWall->uniqueID));
	EXPECT_EQ(capturedPack.changes[0].operation, BattleChanges::EOperation::REMOVE);
}

TEST_F(RemoveObstacleApplyTest, NonMassiveRemovesOnlyAtTargetHex)
{
	JsonNode config;
	config["removeAllSpells"].Bool() = true;
	setupEffect(config);

	const BattleHex targetHex(5, 5);
	auto onTarget = makeSpellObstacle(1, targetHex, SpellID(SpellID::FIRE_WALL));
	auto elsewhere = makeSpellObstacle(2, BattleHex(10, 5), SpellID(SpellID::FIRE_WALL));
	IBattleInfo::ObstacleCList obstacles = { onTarget, elsewhere };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	Target target;
	target.emplace_back(targetHex);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].id, static_cast<uint32_t>(onTarget->uniqueID));
}

TEST_F(RemoveObstacleApplyTest, RemovesUsualObstacleNonMassive)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	const BattleHex targetHex(5, 5);
	auto onTarget = makeUsualObstacleAt(1, targetHex, 0);
	auto elsewhere = makeUsualObstacleAt(2, BattleHex(10, 5), 0);
	IBattleInfo::ObstacleCList obstacles = { onTarget, elsewhere };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	captureObstaclePack();

	Target target;
	target.emplace_back(targetHex);

	subject->apply(&serverMock, &mechanicsMock, target);

	ASSERT_EQ(capturedPack.changes.size(), 1u);
	EXPECT_EQ(capturedPack.changes[0].id, static_cast<uint32_t>(onTarget->uniqueID));
}

TEST_F(RemoveObstacleApplyTest, NoServerCallWhenNothingToRemove)
{
	JsonNode config;
	config["removeUsual"].Bool() = true;
	setupEffect(config);

	auto absolute = makeAbsoluteObstacle(1);
	IBattleInfo::ObstacleCList obstacles = { absolute };

	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(obstacles));
	EXPECT_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_))).Times(0);

	subject->apply(&serverMock, &mechanicsMock, Target());
}

}
