/*
 * ERM_UN.cpp, part of VCMI engine
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

		TEST(ERM_UN_B, AnyInteger_ShouldGenerateIntToStringConversionAndSetStatement)
		{
			LuaStrings lua = ErmRunner::convertErmToLua({ "!#UN:B;" });
			ASSERT_EQ(lua.lines.size(), 9) << lua.text;
			EXPECT_EQ(lua.lines[ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE], "ERM.UN():B(x)");
		}
	}
}


