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

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleLogMessageProxy, "netpacks.BattleLogMessage");

const std::vector<BattleLogMessageProxy::RegType> BattleLogMessageProxy::REGISTER =
{
	{
		"addText",
		&BattleLogMessageProxy::addText
	},
	{
		"toNetpackLight",
		&PackForClientProxy<BattleLogMessageProxy>::toNetpackLight
	},
};

const std::vector<BattleLogMessageProxy::CustomRegType> BattleLogMessageProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true}
};

int BattleLogMessageProxy::addText(lua_State * L, std::shared_ptr<BattleLogMessage> object)
{
	LuaStack S(L);

	std::string text;

	if(S.tryGet(1, text))
	{
		if(object->lines.empty())
			object->lines.emplace_back();
		object->lines.back() << text;
	}

	return S.retVoid();
}


}
}
}
