/*
 * LuaSpellEffectTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../spells/effects/EffectFixture.h"
#include "../JsonComparer.h"

#include "../mock/mock_scripting_Context.h"
#include "../mock/mock_scripting_Script.h"
#include "../mock/mock_scripting_Service.h"
#include "../mock/mock_spells_effects_Registry.h"
#include "../mock/mock_ServerCallback.h"

#include "../../../lib/GameLibrary.h"
#include "../../lib/json/JsonUtils.h"
#include "../../../lib/ScriptHandler.h"
#include "../../../lib/CScriptingModule.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::scripting;
using namespace ::testing;

class LuaSpellEffectTest : public Test, public EffectFixture
{
public:
	const std::string SCRIPT_NAME = "testScript";
	const int32_t EFFECT_LEVEL = 1456;
	const int32_t RANGE_LEVEL = 4561;
	const int32_t EFFECT_POWER = 4789;
	const int32_t EFFECT_DURATION = 42;

	const int32_t EFFECT_VALUE = 42000;//TODO: this should be 64 bit

	ServiceMock serviceMock;
	ScriptMock scriptMock;
	std::shared_ptr<ContextMock> contextMock;
	EffectFactoryMock factoryMock;

	RegistryMock registryMock;
	RegistryMock::FactoryPtr factory;
	StrictMock<ServerCallbackMock> serverMock;

	JsonNode request;

	LuaSpellEffectTest()
		: EffectFixture("testScript")
	{
		contextMock = std::make_shared<ContextMock>();
	}

	void expectSettingContextVariables()
	{
		EXPECT_CALL(mechanicsMock, getEffectLevel()).WillRepeatedly(Return(EFFECT_LEVEL));
		EXPECT_CALL(*contextMock, setGlobal(Eq("effectLevel"), Matcher<int>(Eq(EFFECT_LEVEL))));

		EXPECT_CALL(mechanicsMock, getRangeLevel()).WillRepeatedly(Return(RANGE_LEVEL));
		EXPECT_CALL(*contextMock, setGlobal(Eq("effectRangeLevel"), Matcher<int>(Eq(RANGE_LEVEL))));

		EXPECT_CALL(mechanicsMock, getEffectPower()).WillRepeatedly(Return(EFFECT_POWER));
		EXPECT_CALL(*contextMock, setGlobal(Eq("effectPower"), Matcher<int>(Eq(EFFECT_POWER))));

		EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(EFFECT_DURATION));
		EXPECT_CALL(*contextMock, setGlobal(Eq("effectDuration"), Matcher<int>(Eq(EFFECT_DURATION))));

		EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(EFFECT_VALUE));
		EXPECT_CALL(*contextMock, setGlobal(Eq("effectValue"), Matcher<int>(Eq(EFFECT_VALUE))));
	}

	void setDefaultExpectations()
	{
		EXPECT_CALL(mechanicsMock, scripts()).WillRepeatedly(Return(&serviceMock));

		EXPECT_CALL(*pool, getContext(Eq(&scriptMock))).WillOnce(Return(contextMock));

		expectSettingContextVariables();

//		JsonNode options(JsonNode::JsonType::DATA_STRUCT);
//		//TODO: test passing configuration
//		EffectFixture::setupEffect(options);
	}

	JsonNode saveRequest(const std::string &, const JsonNode & parameters)
	{
		JsonNode response(true);

		request = parameters;
		return response;
	}

	JsonNode saveRequest2(ServerCallback *, const std::string &, const JsonNode & parameters)
	{
		JsonNode response(true);

		request = parameters;
		return response;
	}

protected:
	void SetUp() override
	{
		EXPECT_CALL(registryMock, add(Eq(SCRIPT_NAME), _)).WillOnce(SaveArg<1>(&factory));
		EXPECT_CALL(scriptMock, getName()).WillRepeatedly(ReturnRef(SCRIPT_NAME));
		LIBRARY->scriptHandler->lua->registerSpellEffect(&registryMock, &scriptMock);

		GTEST_ASSERT_NE(factory, nullptr);
		subject.reset(factory->create());

		EffectFixture::setUp();
	}
};

TEST_F(LuaSpellEffectTest, ApplicableRedirected)
{
	setDefaultExpectations();

	JsonNode response(true);

	EXPECT_CALL(*contextMock, callGlobal(Eq("applicable"),_)).WillOnce(Return(response));//TODO: check call parameter

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock));
}

TEST_F(LuaSpellEffectTest, ApplicableTargetRedirected)
{
	setDefaultExpectations();

	EXPECT_CALL(*contextMock, callGlobal(Eq("applicableTarget"),_)).WillOnce(Invoke(this, &LuaSpellEffectTest::saveRequest));

	auto & unit1 = unitsFake.add(BattleSide::ATTACKER);

	EffectTarget target;

	BattleHex hex1(6,7);
	BattleHex hex2(7,8);

	int32_t id1 = 42;

	EXPECT_CALL(unit1, unitId()).WillOnce(Return(id1));

	target.emplace_back(&unit1, hex1);
	target.emplace_back(hex2);

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));


	JsonNode first;
	first.Vector().emplace_back(hex1.toInt());
	first.Vector().emplace_back(id1);

	JsonNode second;
	second.Vector().emplace_back(hex2.toInt());
	second.Vector().emplace_back(-1);

	JsonNode targets;
	targets.Vector().push_back(first);
	targets.Vector().push_back(second);

	JsonNode expected;
	expected.Vector().push_back(targets);

	JsonComparer c(false);
	c.compare("applicableTarget request", request, expected);
}

TEST_F(LuaSpellEffectTest, ApplyRedirected)
{
	setDefaultExpectations();

	EXPECT_CALL(*contextMock, callGlobal(Eq(&serverMock), Eq("apply"),_)).WillOnce(Invoke(this, &LuaSpellEffectTest::saveRequest2));

	auto & unit1 = unitsFake.add(BattleSide::ATTACKER);

	EffectTarget target;

	BattleHex hex1(6,7);

	int32_t id1 = 42;

	EXPECT_CALL(unit1, unitId()).WillOnce(Return(id1));

	target.emplace_back(&unit1, hex1);

	subject->apply(&serverMock, &mechanicsMock, target);

	JsonNode first;
	first.Vector().emplace_back(hex1.toInt());
	first.Vector().emplace_back(id1);

	JsonNode targets;
	targets.Vector().push_back(first);

	JsonNode expected;
	expected.Vector().push_back(targets);

	JsonComparer c(false);
	c.compare("apply request", request, expected);
}

}

