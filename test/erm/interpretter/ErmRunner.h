/*
 * ErmRunner.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../scripting/erm/ERMScriptModule.h"

namespace test
{
	namespace scripting
	{
		struct LuaStrings
		{
			std::string text;
			std::vector<std::string> lines;

			LuaStrings(std::string lua);
		};

		class ErmRunner
		{
		public:
			static const size_t REGULAR_INSTRUCTION_FIRST_LINE;
			static LuaStrings convertErmToLua(std::initializer_list<std::string> list);
		};
	}
}
