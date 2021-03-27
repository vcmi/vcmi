/*
 * ERM_VR.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ErmRunner.h"


namespace test
{
namespace scripting
{

using namespace ::testing;

TEST(ERM_VR_S, SetInteger_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv2:S110;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['2'] = 110");
}

TEST(ERM_VR_S, SetString_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz100:S^Some str^;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['100'] = [===[Some str]===]");
}

TEST(ERM_VR_S, SetStringMultiline_ShouldGenerateMultilineSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({
		"!#VRz100:S^Some",
		"multiline",
		"string^;"
		});

	ASSERT_EQ(lua.lines.size(), 11) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['100'] = [===[Some");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 1], "multiline");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 2], "string]===]");
}

TEST(ERM_VR_C, SetIntegers_ShouldGenerateSetStatements)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv100:C10/20/40;"});

	ASSERT_EQ(lua.lines.size(), 11) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['100'] = 10");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 1], "v['101'] = 20");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 2], "v['102'] = 40");
}

TEST(ERM_VR_C, GetInteger_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({
		"!#VRv100:C10/20/40;",
		"!#VRv100:C?v1;"});

	ASSERT_EQ(lua.lines.size(), 12) << lua.text;
	EXPECT_EQ(lua.lines[8], "v['1'] = v['100']");
}

}
}


