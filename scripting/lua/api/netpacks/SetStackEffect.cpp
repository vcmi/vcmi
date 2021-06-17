/*
 * api/netpacks/SetStackEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SetStackEffect.h"

#include "../../LuaStack.h"

#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(SetStackEffectProxy, "netpacks.SetStackEffect");

const std::vector<SetStackEffectProxy::CustomRegType> SetStackEffectProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"toNetpackLight", &PackForClientProxy<SetStackEffectProxy>::toNetpackLight, false},
	{"addBonus", &SetStackEffectProxy::addBonus, false}
};

int SetStackEffectProxy::addBonus(lua_State * L)
{
	std::shared_ptr<SetStackEffect> pack;
	int bonusType;
	uint32_t unitID;
	std::shared_ptr<const Bonus> bonusToAdd;
	LuaStack S(L);

	if(!S.tryGet(1, pack))
		return S.retVoid();

	if(!S.tryGet(2, unitID))
		return luaL_error(L, "Invalid unit ID");

	if(!S.tryGet(3, bonusToAdd))
		return luaL_error(L, "Invalid bonus value");

	std::vector<Bonus> bonuses;
	
	bonuses.push_back(*bonusToAdd);

	pack->toAdd.push_back(std::make_pair(unitID, bonuses));

	return S.retVoid();
}

}
}
}
