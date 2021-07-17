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
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv100:C?v1;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['100']");
}

TEST(ERM_VR_H, CheckEmptyString_ShouldGenerateCheckAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:H302;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "ERM.VR(z['101']):H('302')");
}
/* should it work?
TEST(ERM_VR_H, CheckEmptyStringWithFlagIndexInVariable_ShouldGenerateCheckAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:Hy1;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "ERM.VR(z['101']):H(y['1'])");
}
*/
TEST(ERM_VR_M1, AnyString_ShouldGenerateSubstringAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M1/z102/2/5;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['102'] = ERM.VR(z['101']):M1(2,5)");
}

TEST(ERM_VR_M1, WithVariables_ShouldGenerateSubstringAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M1/z102/y1/y2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['102'] = ERM.VR(z['101']):M1(y['1'],y['2'])");
}

TEST(ERM_VR_M2, AnyString_ShouldGenerateWordSplitAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M2/z102/2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['102'] = ERM.VR(z['101']):M2(2)");
}

TEST(ERM_VR_M3, AnyInteger_ShouldGenerateIntToStringConversionAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M3/102/16;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['101'] = ERM.VR(z['101']):M3(102,16)");
}
//V
TEST(ERM_VR_M3, IntegerVariable_ShouldGenerateIntToStringConversionAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M3/v1/10;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['101'] = ERM.VR(z['101']):M3(v['1'],10)");
}

TEST(ERM_VR_M4, AnyString_ShouldGenerateStringLengthAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M4/v2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['2'] = ERM.VR(z['101']):M4()");
}

TEST(ERM_VR_M5, AnyString_ShouldGenerateFindFirstNonSpaceCharPositionAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M5/v2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['2'] = ERM.VR(z['101']):M5()");
}

TEST(ERM_VR_M6, AnyString_ShouldGenerateFindLastNonSpaceCharPositionAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz101:M6/v2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['2'] = ERM.VR(z['101']):M6()");
}

TEST(ERM_VR_R, AnyVariable_ShouldGenerateRngAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:R23;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "--VR:R not implemented");
}

TEST(ERM_VR_S_R, AnyVariable_ShouldGenerateRngAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:R23;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "--VR:R not implemented");
}

TEST(ERM_VR_S, DynamicVariable_ShouldGenerateDynamicSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:Svy3;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v[tostring(y['3'])]");
}

TEST(ERM_VR_T, AnyVariable_ShouldGenerateTimeBasedRngAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:T23;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "--VR:T not implemented");
}

TEST(ERM_VR_U, StringVariable_ShouldGenerateSubstringFindAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz2:Uz3;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.lines[0];
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "ERM.VR(z['2']):U(z['3'])");
}

TEST(ERM_VR_U, StringConstant_ShouldGenerateSubstringFindAndSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz2:U^teest^;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "ERM.VR(z['2']):U([===[teest]===])");
}

TEST(ERM_VR_BIT, LogicalAndOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:&8;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = bit.band(v['1'], 8)");
}

TEST(ERM_VR_BITOR, LogicalOrOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:|8;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = bit.bor(v['1'], 8)");
}

TEST(ERM_VR_BITXOR, LogicalXorOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:Xv2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = bit.bxor(v['1'], v['2'])");
}

TEST(ERM_VR_PLUS, PlusOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:+8;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['1'] + 8");
}

TEST(ERM_VR_PLUS, ConcatenationOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRz1:+z3;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "z['1'] = z['1'] .. z['3']");
}

TEST(ERM_VR_MINUS, MinusOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:-v2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['1'] - v['2']");
}

TEST(ERM_VR_MULT, MultiplicationOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:*vy2;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['1'] * v[tostring(y['2'])]");
}

TEST(ERM_VR_DIV, DivisionOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1::8;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['1'] / 8");
}

TEST(ERM_VR_MOD, ModOperator_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:%7;"});

	ASSERT_EQ(lua.lines.size(), 9) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = v['1'] % 7");
}

TEST(ERM_VR_MINUS, Composition_ShouldGenerateSetStatement)
{
	LuaStrings lua = ErmRunner::convertErmToLua({"!#VRv1:S100 -v2 *v3;"});

	ASSERT_EQ(lua.lines.size(), 11) << lua.text;
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "v['1'] = 100");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 1], "v['1'] = v['1'] - v['2']");
	EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE + 2], "v['1'] = v['1'] * v['3']");
}

}
}


