/*
 * TeleportTest.cpp, part of VCMI engine
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
#include "../../../lib/battle/AccessibilityInfo.h"
#include <vstd/RNG.h>

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class TeleportTest : public Test, public EffectFixture
{
public:
	TeleportTest()
		: EffectFixture("core:teleport")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupEffect(JsonNode());
	}
};

class TeleportApplyTest : public Test, public EffectFixture
{
public:
	TeleportApplyTest()
		: EffectFixture("core:teleport")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupEffect(JsonNode());
	}
};

// --- adjustTargetTypes ---

TEST_F(TeleportTest, AdjustTargetTypes_Creature_AddsLocation)
{
	std::vector<AimType> types = { AimType::CREATURE };
	subject->adjustTargetTypes(types, &mechanicsMock);
	ASSERT_EQ(types.size(), 2u);
	EXPECT_EQ(types[0], AimType::CREATURE);
	EXPECT_EQ(types[1], AimType::LOCATION);
}

TEST_F(TeleportTest, AdjustTargetTypes_CreatureLocation_Unchanged)
{
	std::vector<AimType> types = { AimType::CREATURE, AimType::LOCATION };
	subject->adjustTargetTypes(types, &mechanicsMock);
	ASSERT_EQ(types.size(), 2u);
	EXPECT_EQ(types[0], AimType::CREATURE);
	EXPECT_EQ(types[1], AimType::LOCATION);
}

TEST_F(TeleportTest, AdjustTargetTypes_WrongFirst_Clears)
{
	std::vector<AimType> types = { AimType::LOCATION };
	subject->adjustTargetTypes(types, &mechanicsMock);
	EXPECT_TRUE(types.empty());
}

TEST_F(TeleportTest, AdjustTargetTypes_WrongSecond_Clears)
{
	std::vector<AimType> types = { AimType::CREATURE, AimType::CREATURE };
	subject->adjustTargetTypes(types, &mechanicsMock);
	EXPECT_TRUE(types.empty());
}

// --- applicableTarget ---

TEST_F(TeleportTest, ApplicableTarget_InvalidDestHex_Rejected)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::WRONG_SPELL_TARGET), Ref(problemMock)))
		.WillOnce(Return(false));

	Target target;
	target.emplace_back(&unit, BattleHex(5, 5));
	target.emplace_back(BattleHex()); // default-constructed → invalid

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(TeleportTest, ApplicableTarget_AccessibleHex_Accepted)
{
	battleFake->setupEmptyBattlefield();

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	BattleHex unitHex(3, 3);
	BattleHex destHex(8, 3);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));
	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(unitHex));
	EXPECT_CALL(unit, unitSide()).WillRepeatedly(Return(BattleSide::ATTACKER));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	Target target;
	target.emplace_back(&unit, unitHex);
	target.emplace_back(destHex);

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(TeleportTest, ApplicableTarget_OccupiedDestHex_Rejected)
{
	battleFake->setupEmptyBattlefield();

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	auto & blocker = unitsFake.add(BattleSide::DEFENDER);
	BattleHex unitHex(3, 3);
	BattleHex destHex(8, 3);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(unitHex));
	EXPECT_CALL(unit, unitSide()).WillRepeatedly(Return(BattleSide::ATTACKER));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(blocker, getPosition()).WillRepeatedly(Return(destHex));
	EXPECT_CALL(blocker, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(blocker, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(blocker, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::WRONG_SPELL_TARGET), Ref(problemMock)))
		.WillOnce(Return(false));

	Target target;
	target.emplace_back(&unit, unitHex);
	target.emplace_back(destHex);

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

// --- transformTarget ---

TEST_F(TeleportTest, TransformTarget_ValidUnit_DestinationPreserved)
{
	BattleHex unitHex(3, 3);
	BattleHex destHex(8, 3);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isNegativeSpell()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, alwaysHitFirstTarget()).WillRepeatedly(Return(false));

	Target aimPoint;
	aimPoint.emplace_back(&unit, unitHex);
	aimPoint.emplace_back(destHex);

	Target result = subject->transformTarget(&mechanicsMock, aimPoint, Target());

	ASSERT_EQ(result.size(), 2u);
	EXPECT_EQ(result[0].unitValue, &unit);
	EXPECT_EQ(result[1].hexValue, destHex);
}

TEST_F(TeleportTest, TransformTarget_ImmuneUnit_EmptyResult)
{
	BattleHex unitHex(3, 3);
	BattleHex destHex(8, 3);

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isNegativeSpell()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, alwaysHitFirstTarget()).WillRepeatedly(Return(false));

	Target aimPoint;
	aimPoint.emplace_back(&unit, unitHex);
	aimPoint.emplace_back(destHex);

	Target result = subject->transformTarget(&mechanicsMock, aimPoint, Target());

	EXPECT_TRUE(result.empty());
}

// --- apply ---

TEST_F(TeleportApplyTest, Apply_MovesUnit)
{
	battleFake->setupEmptyBattlefield();

	uint32_t unitId = 42;
	BattleHex initial(1, 1);
	BattleHex destination(10, 3);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(initial));
	EXPECT_CALL(unit, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitId));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	EXPECT_CALL(*battleFake, moveUnit(Eq(unitId), Eq(destination)));
	EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(0));
	EXPECT_CALL(serverMock, apply(Matcher<BattleStackMoved &>(_))).Times(1);

	Target target;
	target.emplace_back(&unit, BattleHex());
	target.emplace_back(destination);

	subject->apply(&serverMock, &mechanicsMock, target);
}

}
