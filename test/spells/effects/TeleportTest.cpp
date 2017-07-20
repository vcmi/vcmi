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
	}
};

TEST_F(TeleportApplyTest, MovesUnit)
{
	uint32_t unitId = 42;
	BattleHex initial(1, 1);
	BattleHex destination(10, 10);
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	ON_CALL(unit, getPosition()).WillByDefault(Return(initial));
	EXPECT_CALL(unit, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(unitId));

	EXPECT_CALL(*battleFake, moveUnit(Eq(unitId), Eq(destination)));

	Target target;
	target.emplace_back(&unit, BattleHex());
	target.emplace_back(destination);

    subject->apply(battleProxy.get(), rngMock, &mechanicsMock, target);
}

}

