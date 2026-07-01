/*
 * SerializableRegistar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Registry.h"
#include "FieldDocRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

/// Registar implementation for ApiSerializable types (POD descriptors round-tripped as Lua
/// tables via `serializeScript`). They have no methods, no metatable, and scripts construct
/// them inline as table literals — but they still appear as parameter types in proxy method
/// signatures, so the docs exporter needs to enumerate their fields.
template<class T>
class SerializableRegistar final : public Registar
{
public:
	/// Placeholder so LuaContext::registerPublicTypes can complete its uniform loop —
	/// serializables have no metatable, but the loop expects one value on the stack.
	void pushMetatable(lua_State * L) const override
	{
		lua_newtable(L);
	}

	void collectDocs(MethodRegistrar &) const override { }

	void collectFields(FieldDocRegistrar & sink) const override
	{
		T instance{};
		instance.serializeScript(sink);
	}

	std::string_view getDescription() const override
	{
		return T::luaDescription;
	}
};

template<typename T>
void Registry::registerSerializable()
{
	auto r = std::make_shared<SerializableRegistar<T>>();
	addPrivate(std::string(T::luaName), r);
	registerLuaName<T>(T::luaName);
}

}

VCMI_LIB_NAMESPACE_END
