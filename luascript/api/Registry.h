/*
 * Registry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <typeindex>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class MethodRegistrar;
class FieldDocRegistrar;

/// Interface implemented by each proxy wrapper type; pushes the Lua metatable (and static table) for that type.
class Registar
{
public:
	virtual ~Registar() = default;

	virtual void pushMetatable(lua_State * L) const = 0;

	/// Drives the proxy's `registerMethods` against the given visitor. Used by the docs
	/// exporter and the coverage test to introspect bindings without touching a lua_State.
	virtual void collectDocs(MethodRegistrar & sink) const = 0;

	/// Drives `serializeScript` against the given field visitor for ApiSerializable types.
	/// Default is no-op — proxy wrappers (which expose methods, not fields) keep it empty.
	virtual void collectFields(FieldDocRegistrar &) const { }

	/// Returns the proxy's class-level description (the `luaDescription` constant).
	/// Used by the docs exporter and coverage test. Required to be non-empty for every
	/// type exposed to scripts — empty is allowed only as an explicit "no docs" marker.
	virtual std::string_view getDescription() const = 0;
};

/// Singleton that maps C++ type names to their Registar; used by LuaStack to attach metatables and by LuaContext to expose all types.
class Registry final : public boost::noncopyable
{
	using RegistryData = std::map<std::string, std::shared_ptr<Registar>>;

public:
	/// Returns global unique instance of type registry
	static const Registry * get();

	/// Returns type information for public type with provided registered name
	const Registar * find(const std::string & name) const;

	/// Returns all registered types including internal types
	const RegistryData & getAllTypes() const
	{
		return privateData;
	}

	/// Returns unique type name for a provided type
	/// TODO: consider offering demangled names instead, or those provided as part of registration
	template<typename T>
	const char * getTypeName() const
	{
		return typeid(T).name();
	}

	/// Returns the Lua-facing name for a C++ type. Throws std::runtime_error if the type was
	/// not registered — every non-primitive C++ type that appears in a binding signature must
	/// be registered either via a proxy (registerPrivate / registerSerializable) or via an
	/// explicit alias call in the constructor.
	std::string lookupLuaName(std::type_index t) const;
private:
	RegistryData privateData;
	RegistryData publicData;
	std::unordered_map<std::type_index, std::string> luaNameByType;

	Registry();

	void addPublic(const std::string & name, const std::shared_ptr<Registar> & item);
	void addPrivate(const std::string & name, const std::shared_ptr<Registar> & item);

	template<typename T>
	void registerLuaName(std::string_view name)
	{
		[[maybe_unused]] auto inserted = luaNameByType.try_emplace(std::type_index(typeid(T)), std::string(name));
		assert(inserted.second);
	}

	template<typename T>
	void registerPrivate()
	{
		auto r = std::make_shared<T>();
		addPrivate(std::string(T::luaName), r);
		registerLuaName<typename T::ObjectType>(T::luaName);
	}

	template<typename T>
	void registerSerializable();
};

}

VCMI_LIB_NAMESPACE_END
