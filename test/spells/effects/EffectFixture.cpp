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

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/networkPacks/SetStackEffect.h"

#include "../../../lib/spells/effects/SpellEffectService.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"

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
		&& b1.parameters == b2.parameters
		&& b1.effectRange == b2.effectRange;
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
	mechanicsMock.casterSide = BattleSide::ATTACKER;
}

EffectFixture::~EffectFixture() = default;

void EffectFixture::setupEffect(const JsonNode & effectConfig)
{
	SpellEffectID effectID(*LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "spellEffect", effectName));

	subject = LIBRARY->spellEffects()->create(effectID);
	ASSERT_TRUE(subject);

	JsonNode effectConfigActual = effectConfig;
	effectConfigActual.setModScope("game");
	subject->init(std::move(effectConfigActual));
}


void EffectFixture::setUp()
{
	EXPECT_CALL(environmentMock, game()).WillRepeatedly(Return(&gameMock));
	EXPECT_CALL(environmentMock, logger()).WillRepeatedly(Return(&loggerMock));
	EXPECT_CALL(environmentMock, eventBus()).WillRepeatedly(Return(&eventBus));
	EXPECT_CALL(environmentMock, services()).WillRepeatedly(Return(&servicesMock));

	pool = LIBRARY->scripts()->createPoolInstance(&environmentMock);
	battleFake = std::make_shared<battle::BattleFake>();
	battleFake->setUp();

	EXPECT_CALL(mechanicsMock, game()).WillRepeatedly(Return(&gameMock));
	EXPECT_CALL(mechanicsMock, battle()).WillRepeatedly(Return(battleFake.get()));
	EXPECT_CALL(mechanicsMock, getBattleID()).WillRepeatedly(Return(BattleID()));
	EXPECT_CALL(mechanicsMock, getHeroCaster()).WillRepeatedly(Return(nullptr));

	EXPECT_CALL(*battleFake, getBattleID()).Times(AtLeast(0));

	EXPECT_CALL(*battleFake, getScriptContextPool()).WillRepeatedly(ReturnRef(*pool));

	EXPECT_CALL(servicesMock, creatures()).WillRepeatedly(Return(&creatureServiceMock));
	EXPECT_CALL(mechanicsMock, creatures()).WillRepeatedly(Return(&creatureServiceMock));

	ON_CALL(*battleFake, getUnitsIf(_)).WillByDefault(Invoke(&unitsFake, &battle::UnitsFake::getUnitsIf));
	ON_CALL(mechanicsMock, spells()).WillByDefault(Return(&spellServiceMock));
	EXPECT_CALL(servicesMock, spells()).WillRepeatedly(Return(&spellServiceMock));
	ON_CALL(spellServiceMock, getById(_)).WillByDefault(Return(&spellStub));
	EXPECT_CALL(spellServiceMock, getByName(_)).Times(AnyNumber()).WillRepeatedly(Invoke([](const std::string & name){
		return LIBRARY->spells()->getByName(name);
	}));
	EXPECT_CALL(servicesMock, spellSchools()).Times(AnyNumber()).WillRepeatedly(Return(LIBRARY->spellSchools()));

	ON_CALL(serverMock, getRNG()).WillByDefault(Return(&rngMock));

	ON_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleLogMessage>));
	ON_CALL(serverMock, apply(Matcher<BattleStackMoved &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleStackMoved>));
	ON_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleUnitsChanged>));
	ON_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<SetStackEffect>));
	ON_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<StacksInjured>));
	ON_CALL(serverMock, apply(Matcher<BattleObstaclesChanged &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<BattleObstaclesChanged>));
	ON_CALL(serverMock, apply(Matcher<CatapultAttack &>(_))).WillByDefault(Invoke(battleFake.get(),  &battle::BattleFake::accept<CatapultAttack>));
}

static int getIntRange(int lower, int upper)
{
	return (lower + upper)/2;
}

static int64_t getInt64Range(int64_t lower, int64_t upper)
{
	return (lower + upper)/2;
}

static double getDoubleRange(double lower, double upper)
{
	return (lower + upper)/2;
}

void EffectFixture::setupDefaultRNG()
{
	EXPECT_CALL(serverMock, getRNG()).Times(AtLeast(0));
	EXPECT_CALL(rngMock, nextInt(_,_)).WillRepeatedly(Invoke(&getIntRange));
	EXPECT_CALL(rngMock, nextInt64(_,_)).WillRepeatedly(Invoke(&getInt64Range));
	EXPECT_CALL(rngMock, nextDouble(_,_)).WillRepeatedly(Invoke(&getDoubleRange));
}

}
