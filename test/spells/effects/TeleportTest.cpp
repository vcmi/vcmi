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

TEST_F(TeleportApplyTest, MovesUnit)
{
	battleFake->setupEmptyBattlefield();

	uint32_t unitId = 42;
	BattleHex initial(1, 1);
	BattleHex destination(10, 10);

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(initial));
	EXPECT_CALL(unit, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitId));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));

	EXPECT_CALL(*battleFake, moveUnit(Eq(unitId), Eq(destination)));
	EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(0));
	EXPECT_CALL(serverMock, apply(Matcher<BattleStackMoved *>(_))).Times(1);

	Target target;
	target.emplace_back(&unit, BattleHex());
	target.emplace_back(destination);

    subject->apply(&serverMock, &mechanicsMock, target);
}

}

