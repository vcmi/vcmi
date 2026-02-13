/*
 * api/netpacks/EntitiesChanged.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EntitiesChanged.h"

#include "../../LuaStack.h"

#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(EntitiesChangedProxy, "netpacks.EntitiesChanged");

const std::vector<EntitiesChangedProxy::CustomRegType> EntitiesChangedProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"update", &EntitiesChangedProxy::update, false},
	{"toNetpackLight", &PackForClientProxy<EntitiesChangedProxy>::toNetpackLight, false}
};

int EntitiesChangedProxy::update(lua_State * L)
{
	LuaStack S(L);

	std::shared_ptr<EntitiesChanged> object;
	if(!S.tryGet(1, object))
		return S.retVoid();

	EntityChanges changes;

	int32_t metaIndex = 0;

	if(!S.tryGet(2, metaIndex))
		return S.retVoid();
	changes.metatype = static_cast<Metatype>(metaIndex);

	if(!S.tryGet(3, changes.entityIndex))
		return S.retVoid();

	if(!S.tryGet(4, changes.data))
		return S.retVoid();

	object->changes.push_back(changes);

	return S.retVoid();
}

}
}
}

VCMI_LIB_NAMESPACE_END
