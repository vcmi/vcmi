/*
 * PoolTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/ScriptHandler.h"

#include "../mock/mock_Environment.h"
#include "../mock/mock_ServerCallback.h"
#include "../mock/mock_scripting_Context.h"
#include "../mock/mock_scripting_Script.h"

#include "../JsonComparer.h"

namespace scripting
{
namespace test
{
using namespace ::testing;

class PoolTest : public Test
{
public:
	const std::string SCRIPT_NAME = "TEST SCRIPT";

	NiceMock<EnvironmentMock> env;

	StrictMock<ScriptMock> script;
	std::shared_ptr<PoolImpl> subject;
	StrictMock<ServerCallbackMock> server;

	void setDefaultExpectations()
	{
		EXPECT_CALL(script, getName()).WillRepeatedly(ReturnRef(SCRIPT_NAME));
	}

protected:
	void SetUp() override
	{
		subject = std::make_shared<PoolImpl>(&env);
	}
};

TEST_F(PoolTest, CreatesContext)
{
	setDefaultExpectations();
	auto context = std::make_shared<NiceMock<ContextMock>>();
	EXPECT_CALL(script, createContext(Eq(&env))).WillOnce(Return(context));

	EXPECT_EQ(context, subject->getContext(&script));
}

TEST_F(PoolTest, ReturnsCachedContext)
{
	setDefaultExpectations();
	auto context = std::make_shared<NiceMock<ContextMock>>();
	EXPECT_CALL(script, createContext(Eq(&env))).WillOnce(Return(context));

	auto first = subject->getContext(&script);
	EXPECT_EQ(first, subject->getContext(&script));
}

TEST_F(PoolTest, CreatesNewContextWithEmptyState)
{
	setDefaultExpectations();
	auto context = std::make_shared<StrictMock<ContextMock>>();
	EXPECT_CALL(script, createContext(Eq(&env))).WillOnce(Return(context));

	EXPECT_CALL(*context, run(Eq(JsonNode()))).Times(1);
	subject->getContext(&script);//return value ignored
}

TEST_F(PoolTest, SavesScriptState)
{
	setDefaultExpectations();
	auto context = std::make_shared<StrictMock<ContextMock>>();
	EXPECT_CALL(script, createContext(Eq(&env))).WillOnce(Return(context));

	EXPECT_CALL(*context, run(Eq(JsonNode()))).Times(1);
	subject->getContext(&script);

	JsonNode expectedState;
	expectedState[SCRIPT_NAME]["foo"].String() = "bar";

	EXPECT_CALL(*context, saveState()).WillOnce(Return(expectedState[SCRIPT_NAME]));

	JsonNode actualState;
	subject->serializeState(true, actualState);

	JsonComparer c(false);
	c.compare("state", actualState, expectedState);
}

TEST_F(PoolTest, LoadsScriptState)
{
	setDefaultExpectations();
	auto context = std::make_shared<StrictMock<ContextMock>>();
	EXPECT_CALL(script, createContext(Eq(&env))).WillOnce(Return(context));

	JsonNode expectedState;
	expectedState[SCRIPT_NAME]["foo"].String() = "bar";

	subject->serializeState(false, expectedState);

	EXPECT_CALL(*context, run(Eq(expectedState[SCRIPT_NAME]))).Times(1);
	subject->getContext(&script);
}

}
}
