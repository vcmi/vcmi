/*
 * EffectFixture.cpp, part of VCMI engine
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

#include "../../../lib/serializer/JsonDeserializer.h"

bool battle::operator==(const Destination& left, const Destination& right)
{
	return left.unitValue == right.unitValue && left.hexValue == right.hexValue;
}

namespace test
{

using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

void EffectFixture::UnitFake::addNewBonus(const std::shared_ptr<Bonus> & b)
{
	bonusFake.addNewBonus(b);
}

void EffectFixture::UnitFake::makeAlive()
{
	EXPECT_CALL(*this, alive()).Times(AtLeast(1)).WillRepeatedly(Return(true));
}

void EffectFixture::UnitFake::makeDead()
{
	EXPECT_CALL(*this, alive()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(*this, isGhost()).Times(AtLeast(1)).WillRepeatedly(Return(false));
}

void EffectFixture::UnitFake::redirectBonusesToFake()
{
	ON_CALL(*this, getAllBonuses(_, _, _, _)).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getAllBonuses));
	ON_CALL(*this, getTreeVersion()).WillByDefault(Invoke(&bonusFake, &BonusBearerMock::getTreeVersion));
}

void EffectFixture::UnitFake::expectAnyBonusSystemCall()
{
	EXPECT_CALL(*this, getAllBonuses(_, _, _, _)).Times(AtLeast(0));
	EXPECT_CALL(*this, getTreeVersion()).Times(AtLeast(0));
}

EffectFixture::UnitFake & EffectFixture::UnitsFake::add(ui8 side)
{
	UnitFake * unit = new UnitFake();
	ON_CALL(*unit, unitSide()).WillByDefault(Return(side));
	unit->redirectBonusesToFake();

	allUnits.emplace_back(unit);
	return *allUnits.back().get();
}

battle::Units EffectFixture::UnitsFake::getUnitsIf(battle::UnitFilter predicate) const
{
	battle::Units ret;

	for(auto & unit : allUnits)
	{
		if(predicate(unit.get()))
			ret.push_back(unit.get());
	}
	return ret;
}

EffectFixture::BattleFake::BattleFake()
	: CBattleInfoCallback(),
	BattleStateMock()
{
}

void EffectFixture::BattleFake::setUp()
{
	CBattleInfoCallback::setBattle(this);
}

void EffectFixture::UnitsFake::setDefaultBonusExpectations()
{
	for(auto & unit : allUnits)
	{
		unit->expectAnyBonusSystemCall();
	}
}

EffectFixture::EffectFixture(std::string effectName_)
	:subject(),
	problemMock(),
	mechanicsMock(),
	battleFake(),
	effectName(effectName_)
{

}

EffectFixture::~EffectFixture() = default;

void EffectFixture::setupEffect(const JsonNode & effectConfig)
{
	JsonDeserializer deser(nullptr, effectConfig);
	subject->serializeJson(deser);
}

void EffectFixture::setUp()
{
	subject = Effect::create(effectName);
	ASSERT_TRUE(subject);
	battleFake = std::make_shared<BattleFake>();
	battleFake->setUp();

	mechanicsMock.cb = battleFake.get();

	battleProxy = std::make_shared<BattleStateProxy>(battleFake.get());

	ON_CALL(*battleFake, getUnitsIf(_)).WillByDefault(Invoke(&unitsFake, &UnitsFake::getUnitsIf));
}

static vstd::TRandI64 getInt64RangeDef(int64_t lower, int64_t upper)
{
	return [=]()->int64_t
	{
		return (lower + upper)/2;
	};
}

static vstd::TRand getDoubleRangeDef(double lower, double upper)
{
	return [=]()->double
	{
		return (lower + upper)/2;
	};
}

void EffectFixture::setupDefaultRNG()
{
	EXPECT_CALL(rngMock, getInt64Range(_,_)).WillRepeatedly(Invoke(&getInt64RangeDef));
	EXPECT_CALL(rngMock, getDoubleRange(_,_)).WillRepeatedly(Invoke(&getDoubleRangeDef));
}

void EffectFixture::setupEmptyBattlefield()
{
	EXPECT_CALL(*battleFake, getDefendedTown()).WillRepeatedly(Return(nullptr));
	EXPECT_CALL(*battleFake, getAllObstacles()).WillRepeatedly(Return(IBattleInfo::ObstacleCList()));
	EXPECT_CALL(*battleFake, getBattlefieldType()).WillRepeatedly(Return(BFieldType::NONE2));
}

}
