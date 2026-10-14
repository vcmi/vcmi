/*
 * CatapultTest.cpp, part of VCMI engine
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

#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/json/JsonNode.h"
#include "../../mock/TownFake.h"


namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

namespace
{
	int midpointRng(int low, int high) { return (low + high) / 2; }
}

class CatapultTest : public Test, public EffectFixture
{
public:
	CatapultTest()
		:EffectFixture("core:catapult")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupEffect(JsonNode());
	}
};

TEST_F(CatapultTest, NotApplicableWithoutTown)
{
	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(nullptr));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, NotApplicableInVillage)
{
	TownFake fakeTown;

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, NotApplicableForDefenderIfSmart)
{
	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);
	mechanicsMock.casterSide = BattleSide::DEFENDER;

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(CatapultTest, ApplicableInTown)
{
	TownFake fakeTown;
	fakeTown.withBuilding(BuildingID::FORT);

	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
	EXPECT_CALL(mechanicsMock, adaptProblem(_, _)).Times(0);
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::INTACT));

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

class CatapultApplyTest : public Test, public EffectFixture
{
public:
	CatapultApplyTest()
		: EffectFixture("core:catapult")
	{
	}

	void setDefaultExpectations()
	{
		EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(fakeTown.get()));
		EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
		setupDefaultRNG();
		EXPECT_CALL(rngMock, nextInt(Matcher<int>(_), Matcher<int>(_)))
			.WillRepeatedly(Invoke(&midpointRng));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		fakeTown.withBuilding(BuildingID::FORT);
	}

	TownFake fakeTown;
};

TEST_F(CatapultApplyTest, DamageToIntactPart)
{
	{
		JsonNode config;
		config["targetsToAttack"].Integer() = 1;
		config["chanceToNormalHit"].Integer() = 100;
		EffectFixture::setupEffect(config);
	}

	setDefaultExpectations();

	const EWallPart targetPart = EWallPart::BELOW_GATE;
	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::DESTROYED));
	EXPECT_CALL(*battleFake, getWallState(Eq(targetPart))).WillRepeatedly(Return(EWallState::INTACT));
	EXPECT_CALL(*battleFake, setWallState(Eq(targetPart), Eq(EWallState::DAMAGED))).Times(1);

	CatapultAttack capturedPack;
	EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
		.WillOnce(Invoke([this, &capturedPack](CatapultAttack & pack)
		{
			capturedPack = pack;
			BattleStatePackVisitor visitor(*battleFake);
			pack.visit(visitor);
		}));

	Target target;
	target.emplace_back();

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(capturedPack.attackedPart, targetPart);
	EXPECT_EQ(capturedPack.killedTowerShooter, -1);
}

TEST_F(CatapultApplyTest, TargetedHitOnSpecifiedPart)
{
	{
		JsonNode config;
		config["targetsToAttack"].Integer() = 1;
		config["chanceToNormalHit"].Integer() = 100;
		config["chanceToHitWall"].Integer() = 100;
		EffectFixture::setupEffect(config);
	}

	setDefaultExpectations();

	const EWallPart targetPart = EWallPart::BELOW_GATE;
	const BattleHex targetHex(130); // maps to BELOW_GATE
	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::DESTROYED));
	EXPECT_CALL(*battleFake, getWallState(Eq(targetPart))).WillRepeatedly(Return(EWallState::INTACT));
	EXPECT_CALL(*battleFake, setWallState(Eq(targetPart), Eq(EWallState::DAMAGED))).Times(1);

	CatapultAttack capturedPack;
	EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
		.WillOnce(Invoke([this, &capturedPack](CatapultAttack & pack)
		{
			capturedPack = pack;
			BattleStatePackVisitor visitor(*battleFake);
			pack.visit(visitor);
		}));

	Target target;
	target.emplace_back(targetHex);

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(capturedPack.attackedPart, targetPart);
	EXPECT_EQ(capturedPack.killedTowerShooter, -1);
}

TEST_F(CatapultApplyTest, TargetedMissRedirectsToPotentialTarget)
{
	{
		JsonNode config;
		config["targetsToAttack"].Integer() = 1;
		config["chanceToNormalHit"].Integer() = 100;
		config["chanceToHitWall"].Integer() = 0; // always miss the desired part
		EffectFixture::setupEffect(config);
	}

	setDefaultExpectations();

	const EWallPart desiredPart = EWallPart::BELOW_GATE;
	const EWallPart fallbackPart = EWallPart::BOTTOM_WALL;
	const BattleHex desiredHex(130);
	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::DESTROYED));
	EXPECT_CALL(*battleFake, getWallState(Eq(desiredPart))).WillRepeatedly(Return(EWallState::INTACT));
	EXPECT_CALL(*battleFake, getWallState(Eq(fallbackPart))).WillRepeatedly(Return(EWallState::INTACT));
	EXPECT_CALL(*battleFake, setWallState(Eq(fallbackPart), _)).Times(1);
	EXPECT_CALL(*battleFake, setWallState(Eq(desiredPart), _)).Times(0);

	CatapultAttack capturedPack;
	EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
		.WillOnce(Invoke([this, &capturedPack](CatapultAttack & pack)
		{
			capturedPack = pack;
			BattleStatePackVisitor visitor(*battleFake);
			pack.visit(visitor);
		}));

	Target target;
	target.emplace_back(desiredHex);

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(capturedPack.attackedPart, fallbackPart);
	EXPECT_EQ(capturedPack.killedTowerShooter, -1);
}

TEST_F(CatapultApplyTest, RemovesTowerShooterOnKeepDestroyed)
{
	{
		JsonNode config;
		config["targetsToAttack"].Integer() = 1;
		config["chanceToCrit"].Integer() = 100; // damage = 2, takes DAMAGED -> DESTROYED
		EffectFixture::setupEffect(config);
	}

	setDefaultExpectations();

	const EWallPart targetPart = EWallPart::KEEP;
	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);
	auto & towerShooter = unitsFake.add(BattleSide::DEFENDER);
	const uint32_t shooterId = 99;
	EXPECT_CALL(towerShooter, getPosition()).WillRepeatedly(Return(BattleHex(BattleHex::CASTLE_CENTRAL_TOWER)));
	EXPECT_CALL(towerShooter, isGhost()).WillRepeatedly(Return(false));
	EXPECT_CALL(towerShooter, unitId()).WillRepeatedly(Return(shooterId));
	EXPECT_CALL(towerShooter, doubleWide()).WillRepeatedly(Return(false));

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	// All wall parts DESTROYED except KEEP which is DAMAGED so it becomes the only attackable.
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::DESTROYED));
	EXPECT_CALL(*battleFake, getWallState(Eq(targetPart))).WillRepeatedly(Return(EWallState::DAMAGED));
	EXPECT_CALL(*battleFake, setWallState(Eq(targetPart), Eq(EWallState::DESTROYED))).Times(1);
	EXPECT_CALL(*battleFake, removeUnit(Eq(shooterId))).Times(1);

	CatapultAttack capturedPack;
	EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
		.WillOnce(Invoke([this, &capturedPack](CatapultAttack & pack)
		{
			capturedPack = pack;
			BattleStatePackVisitor visitor(*battleFake);
			pack.visit(visitor);
		}));

	Target target;
	target.emplace_back();

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(capturedPack.attackedPart, targetPart);
	EXPECT_EQ(capturedPack.damageDealt, 2u);
	EXPECT_EQ(capturedPack.killedTowerShooter, static_cast<int32_t>(shooterId));
}

TEST_F(CatapultApplyTest, MassiveAttacksMultipleParts)
{
	{
		JsonNode config;
		config["targetsToAttack"].Integer() = 3;
		config["chanceToNormalHit"].Integer() = 100;
		EffectFixture::setupEffect(config);
	}

	setDefaultExpectations();

	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, unitId()).WillRepeatedly(Return(static_cast<uint32_t>(-1)));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(true));
	EXPECT_CALL(*battleFake, getWallState(_)).WillRepeatedly(Return(EWallState::INTACT));
	EXPECT_CALL(*battleFake, setWallState(_, _)).Times(AtLeast(1));

	std::vector<CatapultAttack> capturedPacks;
	EXPECT_CALL(serverMock, apply(Matcher<CatapultAttack &>(_)))
		.WillRepeatedly(Invoke([this, &capturedPacks](CatapultAttack & pack)
		{
			capturedPacks.push_back(pack);
			BattleStatePackVisitor visitor(*battleFake);
			pack.visit(visitor);
		}));

	Target target;
	target.emplace_back();

	subject->apply(&serverMock, &mechanicsMock, target);

	const int distinctParts = static_cast<int>(capturedPacks.size());
	EXPECT_GE(distinctParts, 1);
	EXPECT_LE(distinctParts, 3);

	std::set<EWallPart> seen;
	for(const auto & p : capturedPacks)
		seen.insert(p.attackedPart);
	EXPECT_EQ(seen.size(), capturedPacks.size()); // each pack hits a distinct part
}

}
