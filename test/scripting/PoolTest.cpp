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

#include "../mock/mock_Environment.h"
#include "../mock/mock_ServerCallback.h"
#include "../mock/mock_scripting_Context.h"
#include "../mock/mock_scripting_Script.h"

#include "../JsonComparer.h"

#include "../../luascript/LuaScriptPool.h"

namespace scripting
{
namespace test
{
using namespace ::testing;

#if 0

class PoolTest : public Test
{
public:
	const std::string SCRIPT_NAME = "TEST SCRIPT";

	NiceMock<EnvironmentMock> env;

	StrictMock<ScriptMock> script;
	std::shared_ptr<LuaScriptPool> subject;
	StrictMock<ServerCallbackMock> server;

	void setDefaultExpectations()
	{
		EXPECT_CALL(script, getJsonKey()).WillRepeatedly(Return(SCRIPT_NAME));
	}

protected:
	void SetUp() override
	{
		subject = std::make_shared<LuaScriptPool>(&env);
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

#endif

}
}
