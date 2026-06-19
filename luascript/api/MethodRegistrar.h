/*
 * MethodRegistrar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../LuaCallWrapper.h"
#include "SignatureOf.h"

#include <span>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

/// Host-supplied per-parameter metadata for `method<>` / `function<>`. The type is auto-derived
/// from the C++ method pointer; only the name is required, the description is optional.
struct LuaParam
{
	std::string_view name;
	std::string_view description = "";
};

/// Host-supplied per-parameter metadata for `cfunction<>`. Type must be spelled out — the raw
/// `int(lua_State *)` signature has no traits to mine.
struct LuaCParam
{
	std::string_view name;
	std::string_view type;
	std::string_view description = "";
};

/// Host-supplied return metadata for `method<>` / `function<>`. Type is auto-derived; only the
/// optional description is carried. Pass `{}` when no description is needed (still required for
/// void returns).
struct LuaReturn
{
	std::string_view description = "";
};

/// Host-supplied return metadata for `cfunction<>`. Type must be spelled out, description is
/// optional. Pass `{}` (an empty `type`) when the function returns nothing.
struct LuaCReturn
{
	std::string_view type = "";
	std::string_view description = "";
};

/// One parameter as stored in DocRegistrar entries — fully resolved (name, type, description).
struct ParamDoc
{
	std::string name;
	std::string type;
	std::string description;
};

/// Return-value documentation for a single binding. `type` is empty for void returns.
struct ReturnDoc
{
	std::string type;
	std::string description;
};

/// One method/function/cfunction binding as collected by the doc registrar. The runtime
/// LuaRegistrar ignores all fields except the name and the Lua C function pointer.
struct DocEntry
{
	std::string name;
	std::string description;
	std::vector<ParamDoc> params;
	ReturnDoc ret;
};

/// Visitor that consumes per-proxy method bindings.
/// Each proxy implements `static void registerMethods(MethodRegistrar &)` and calls
/// the templated helpers on this interface. Concrete implementations either push the
/// bindings directly onto a Lua metatable (LuaRegistrar) or collect them as metadata
/// for documentation export (DocRegistrar).
class MethodRegistrar
{
public:
	virtual ~MethodRegistrar() = default;

	/// Lowest-level entry; the templated helpers below all funnel through this.
	/// Runtime registrars only consume `name` + `fn`; doc registrars store the whole entry.
	virtual void addEntry(DocEntry entry, lua_CFunction fn) = 0;

	/// Register a C++ member function with N>=1 parameters.
	/// Signature types are auto-derived; the host supplies the parameter names (and optional
	/// per-param descriptions) and an optional return description.
	/// `ExplicitObjectType` is required only when the method is defined on an untagged base class.
	template<auto Method, class ExplicitObjectType = void, std::size_t N>
	void method(std::string_view name,
	            const LuaParam (& params)[N],
	            LuaReturn ret,
	            std::string_view description)
	{
		using TR = LuaClassMemberTraits<decltype(Method)>;
		static_assert(N == std::tuple_size_v<typename TR::ArgsTuple>,
			"method<>: parameter name count does not match C++ argument count");
		using W = LuaMethodWrapper<Method, ExplicitObjectType>;
		addEntry(buildMethodEntry<Method>(name, params, ret, description), &W::invoke);
	}

	/// Zero-parameter overload of `method<>` — params array would otherwise have to be
	/// a zero-length C array, which the language does not allow.
	template<auto Method, class ExplicitObjectType = void>
	void method(std::string_view name,
	            LuaReturn ret,
	            std::string_view description)
	{
		using TR = LuaClassMemberTraits<decltype(Method)>;
		static_assert(std::tuple_size_v<typename TR::ArgsTuple> == 0,
			"method<>: zero-arg overload picked for a method that takes arguments");
		using W = LuaMethodWrapper<Method, ExplicitObjectType>;
		addEntry(buildMethodEntry<Method>(name, std::span<const LuaParam>{}, ret, description), &W::invoke);
	}

	/// Register a static proxy helper that takes the object as its first argument.
	/// The first C++ argument is treated as self and dropped from the documented parameters.
	template<auto Fn, std::size_t N>
	void function(std::string_view name,
	              const LuaParam (& params)[N],
	              LuaReturn ret,
	              std::string_view description)
	{
		using TR = LuaFunctionTraits<std::remove_pointer_t<std::decay_t<decltype(Fn)>>>;
		static_assert(std::tuple_size_v<typename TR::ArgsTuple> >= 1,
			"function<>: static helper must take the object as its first argument");
		static_assert(N == std::tuple_size_v<typename TR::ArgsTuple> - 1,
			"function<>: parameter name count does not match C++ argument count minus self");
		using W = LuaFunctionWrapper<Fn>;
		addEntry(buildFunctionEntry<Fn>(name, params, ret, description), &W::invoke);
	}

	/// Zero-extra-parameter overload of `function<>` — for static helpers whose only C++
	/// argument is the self/object reference.
	template<auto Fn>
	void function(std::string_view name,
	              LuaReturn ret,
	              std::string_view description)
	{
		using TR = LuaFunctionTraits<std::remove_pointer_t<std::decay_t<decltype(Fn)>>>;
		static_assert(std::tuple_size_v<typename TR::ArgsTuple> == 1,
			"function<>: zero-arg overload picked for a helper that takes script-side arguments");
		using W = LuaFunctionWrapper<Fn>;
		addEntry(buildFunctionEntry<Fn>(name, std::span<const LuaParam>{}, ret, description), &W::invoke);
	}

	/// Register a raw `int(lua_State *)` function with N>=1 script-visible parameters.
	/// Caller spells out both parameter types and the return type — nothing is auto-derived.
	template<auto Fn, std::size_t N>
	void cfunction(std::string_view name,
	               const LuaCParam (& params)[N],
				   const LuaCReturn & ret,
	               std::string_view description)
	{
		addEntry(buildCFunctionEntry(name, params, ret, description), &LuaCallWrapper<Fn>::invoke);
	}

	/// Zero-parameter overload of `cfunction<>`.
	template<auto Fn>
	void cfunction(std::string_view name,
				   const LuaCReturn & ret,
	               std::string_view description)
	{
		addEntry(buildCFunctionEntry(name, std::span<const LuaCParam>{}, ret, description), &LuaCallWrapper<Fn>::invoke);
	}

private:
	template<auto Method>
	static DocEntry buildMethodEntry(std::string_view name,
	                                 std::span<const LuaParam> params,
	                                 LuaReturn ret,
	                                 std::string_view description)
	{
		const auto types = paramTypesOfMethod<Method>();
		assert(types.size() == params.size());
		DocEntry entry{std::string(name), std::string(description), {}, {returnTypeOfMethod<Method>(), std::string(ret.description)}};
		entry.params.reserve(params.size());
		for(std::size_t i = 0; i < params.size(); ++i)
			entry.params.push_back({std::string(params[i].name), types[i], std::string(params[i].description)});
		return entry;
	}

	template<auto Fn>
	static DocEntry buildFunctionEntry(std::string_view name,
	                                   std::span<const LuaParam> params,
	                                   LuaReturn ret,
	                                   std::string_view description)
	{
		const auto types = paramTypesOfFunction<Fn>();
		assert(types.size() == params.size());
		DocEntry entry{std::string(name), std::string(description), {}, {returnTypeOfFunction<Fn>(), std::string(ret.description)}};
		entry.params.reserve(params.size());
		for(std::size_t i = 0; i < params.size(); ++i)
			entry.params.push_back({std::string(params[i].name), types[i], std::string(params[i].description)});
		return entry;
	}

	static DocEntry buildCFunctionEntry(std::string_view name,
	                                    std::span<const LuaCParam> params,
	                                    LuaCReturn ret,
	                                    std::string_view description)
	{
		DocEntry entry{std::string(name), std::string(description), {}, {std::string(ret.type), std::string(ret.description)}};
		entry.params.reserve(params.size());
		for(const auto & p : params)
			entry.params.push_back({std::string(p.name), std::string(p.type), std::string(p.description)});
		return entry;
	}
};

}

VCMI_LIB_NAMESPACE_END
