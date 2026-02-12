/*
 * api/netpacks/BattleLogMessage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleLogMessage.h"

#include "../../LuaStack.h"

#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleLogMessageProxy, "netpacks.BattleLogMessage");

const std::vector<BattleLogMessageProxy::CustomRegType> BattleLogMessageProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"addText", &BattleLogMessageProxy::addText, false},
	{"toNetpackLight",&PackForClientProxy<BattleLogMessageProxy>::toNetpackLight, false}
};

int BattleLogMessageProxy::addText(lua_State * L)
{
	LuaStack S(L);

	std::shared_ptr<BattleLogMessage> object;

	if(S.tryGet(1, object))
	{
		std::string text;

		if(S.tryGet(2, text))
		{
			if(object->lines.empty())
				object->lines.emplace_back();
			object->lines.back().appendRawString(text);
		}
	}

	return S.retVoid();
}


}
}
}

VCMI_LIB_NAMESPACE_END
