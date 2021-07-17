/*
 * mock_scripting_Script.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

namespace scripting
{

class ScriptMock : public Script
{
public:
 	MOCK_CONST_METHOD0(getName, const std::string &());
 	MOCK_CONST_METHOD0(getSource, const std::string &());
	MOCK_CONST_METHOD1(createContext, std::shared_ptr<Context>(const Environment *));
};

}



