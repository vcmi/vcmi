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

#include "../../../lib/NetPacks.h"

#include "../../../lib/serializer/JsonDeserializer.h"

bool battle::operator==(const Destination& left, const Destination& right)
{
	return left.unitValue == right.unitValue && left.hexValue == right.hexValue;
}

bool operator==(const Bonus & b1, const Bonus & b2)
{
	return b1.duration == b2.duration
		&& b1.type == b2.type
		&& b1.subtype == b2.subtype
		&& b1.source == b2.source
		&& b1.val == b2.val
		&& b1.sid == b2.sid
		&& b1.valType == b2.valType
		&& b1.additionalInfo == b2.additionalInfo
		&& b1.effectRange == b2.effectRange
		&& b1.description == b2.description;
}

namespace test
{

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
	subject = Effect::create(GlobalRegistry::get(), effectName);
	ASSERT_TRUE(subject);

	JsonDeserializer deser(nullptr, effectConfig);
	subject->serializeJson(deser);
}

void EffectFixture::setupEffect(Registry * registry, const JsonNode & effectConfig)
{
	subject = Effect::create(registry, effectName);
	ASSERT_TRUE(subject);

	JsonDeserializer deser(nullptr, effectConfig);
	subject->serializeJson(deser);
}


void EffectFixture::setUp()
{
#if SCRIPTING_ENABLED
	pool = std::make_shared<PoolMock>();
	battleFake = std::make_shared<battle::BattleFake>(pool);
#else
	battleFake = std::make_shared<battle::BattleFake>();
#endif
	battleFake->setUp();

	EXPECT_CALL(mechanicsMock, game()).WillRepeatedly(Return(&gameMock));
	EXPECT_CALL(mechanicsMock, battle()).WillRepeatedly(Return(battleFake.get()));

	ON_CALL(*battleFake, getUnitsIf(_)).WillByDefault(Invoke(&unitsFake, &battle::UnitsFake::getUnitsIf));
	ON_CALL(mechanicsMock, spells()).WillByDefault(Return(&spellServiceMock));
	ON_CALL(spellServiceMock, getById(_)).WillByDefault(Return(&spellStub));

	ON_CALL(mechanicsMock, creatures()).WillByDefault(Return(&creatureServiceMock));
	ON_CALL(creatureServiceMock, getById(_)).WillByDefault(Return(&creatureStub));

	ON_CALL(serverMock, getRNG()).WillByDefault(Return(&rngMock));

	ON_CALL(serverMock, apply(Matcher<BattleLogMessage *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleLogMessage>));
	ON_CALL(serverMock, apply(Matcher<BattleStackMoved *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleStackMoved>));
	ON_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleUnitsChanged>));
	ON_CALL(serverMock, apply(Matcher<SetStackEffect *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<SetStackEffect>));
	ON_CALL(serverMock, apply(Matcher<StacksInjured *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<StacksInjured>));
	ON_CALL(serverMock, apply(Matcher<BattleObstaclesChanged *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleObstaclesChanged>));
	ON_CALL(serverMock, apply(Matcher<CatapultAttack *>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<CatapultAttack>));
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
	EXPECT_CALL(serverMock, getRNG()).Times(AtLeast(0));
	EXPECT_CALL(rngMock, getInt64Range(_,_)).WillRepeatedly(Invoke(&getInt64RangeDef));
	EXPECT_CALL(rngMock, getDoubleRange(_,_)).WillRepeatedly(Invoke(&getDoubleRangeDef));
}

}
