/*
 * ErmRunner.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../scripting/ScriptFixture.h"
#include "ErmRunner.h"

namespace test
{
	namespace scripting
	{
		const size_t ErmRunner::REGULAR_INSTRUCTION_FIRST_LINE = 5;

		LuaStrings::LuaStrings(std::string lua)
			:text(lua), lines()
		{
			boost::split(lines, lua, boost::is_any_of("\r\n"));
		}

		LuaStrings ErmRunner::convertErmToLua(std::initializer_list<std::string> list)
		{
			std::stringstream source;

			source << "ZVSE" << std::endl;
			for(auto ermLine : list)
				source << ermLine << std::endl;

			auto lua = VLC->scriptHandler->erm->compile("", source.str(), logGlobal);

			return LuaStrings(lua);
		}
	}
}