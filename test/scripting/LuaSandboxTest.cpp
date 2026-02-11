/*
 * LuaSandboxTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ScriptFixture.h"

namespace test
{

using ::testing::Test;

class LuaSandboxTest : public Test, public ScriptFixture
{
public:


protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};


TEST_F(LuaSandboxTest, Example)
{
	loadScriptFromFile("test/lua/SandboxTest.lua");
	runClientServer();
}

}
