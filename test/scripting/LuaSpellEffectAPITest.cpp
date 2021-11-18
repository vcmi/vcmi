/*
 * LuaSpellEffectAPITest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptFixture.h"

#include "../../lib/NetPacks.h"

#include "../mock/mock_ServerCallback.h"

namespace test
{

using namespace ::testing;
using namespace ::scripting;

class LuaSpellEffectAPITest : public Test, public ScriptFixture
{
public:
	StrictMock<ServerCallbackMock> serverMock;

protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(LuaSpellEffectAPITest, ApplicableOnExpert)
{
	loadScriptFromFile("test/lua/SpellEffectAPITest.lua");

	context->setGlobal("effectLevel", 3);

	runClientServer();

	JsonNode params;

	JsonNode ret = context->callGlobal("applicable", params);

	JsonNode expected = JsonUtils::boolNode(true);

	JsonComparer cmp(false);
	cmp.compare("applicable result", ret, expected);

}

TEST_F(LuaSpellEffectAPITest, NotApplicableOnAdvanced)
{
	loadScriptFromFile("test/lua/SpellEffectAPITest.lua");

	context->setGlobal("effectLevel", 2);

	runClientServer();

	JsonNode params;

	JsonNode ret = context->callGlobal("applicable", params);

	JsonNode expected = JsonUtils::boolNode(false);

	JsonComparer cmp(false);
	cmp.compare("applicable result", ret, expected);
}

TEST_F(LuaSpellEffectAPITest, ApplicableOnLeftSideOfField)
{
	loadScriptFromFile("test/lua/SpellEffectAPITest.lua");

	context->setGlobal("effectLevel", 1);

	runClientServer();

	JsonNode params;

	BattleHex hex(2,2);

	JsonNode first;
	first.Vector().push_back(JsonUtils::intNode(hex.hex));
	first.Vector().push_back(JsonNode());

	JsonNode targets;
	targets.Vector().push_back(first);

	params.Vector().push_back(targets);

	JsonNode ret = context->callGlobal("applicableTarget", params);

	JsonNode expected = JsonUtils::boolNode(true);

	JsonComparer cmp(false);
	cmp.compare("applicable result", ret, expected);
}

TEST_F(LuaSpellEffectAPITest, NotApplicableOnRightSideOfField)
{
	loadScriptFromFile("test/lua/SpellEffectAPITest.lua");

	runClientServer();

	context->setGlobal("effectLevel", 1);

	JsonNode params;

	BattleHex hex(11,2);

	JsonNode first;
	first.Vector().push_back(JsonUtils::intNode(hex.hex));
	first.Vector().push_back(JsonUtils::intNode(-1));

	JsonNode targets;
	targets.Vector().push_back(first);

	params.Vector().push_back(targets);

	JsonNode ret = context->callGlobal("applicableTarget", params);

	JsonNode expected = JsonUtils::boolNode(false);

	JsonComparer cmp(false);
	cmp.compare("applicable result", ret, expected);
}

TEST_F(LuaSpellEffectAPITest, ApplyMoveUnit)
{
	loadScriptFromFile("test/lua/SpellEffectAPIMoveUnit.lua");

	runClientServer();

	BattleHex hex1(11,2);

	JsonNode unit;
	unit.Vector().push_back(JsonUtils::intNode(hex1.hex));
	unit.Vector().push_back(JsonUtils::intNode(42));

	BattleHex hex2(5,4);

	JsonNode destination;
	destination.Vector().push_back(JsonUtils::intNode(hex2.hex));
	destination.Vector().push_back(JsonUtils::intNode(-1));

	JsonNode targets;
	targets.Vector().push_back(unit);
	targets.Vector().push_back(destination);

	JsonNode params;
	params.Vector().push_back(targets);

	BattleStackMoved expected;
	BattleStackMoved actual;

	auto checkMove = [&](BattleStackMoved * pack)
	{
		EXPECT_EQ(pack->stack, 42);
		EXPECT_EQ(pack->teleporting, true);
		EXPECT_EQ(pack->distance, 0);

		std::vector<BattleHex> toMove(1, hex2);

		EXPECT_EQ(pack->tilesToMove, toMove);
	};

	EXPECT_CALL(serverMock, apply(Matcher<BattleStackMoved *>(_))).WillOnce(Invoke(checkMove));

	context->callGlobal(&serverMock, "apply", params);
}

}
