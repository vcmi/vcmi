/*
 * SignatureOf.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../LuaCallWrapper.h"
#include "../../lib/constants/IdentifierBase.h"
#include "../../include/vcmi/scripting/ApiTags.h"

#include "Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

namespace detail
{
template<class T> struct IsVector                       : std::false_type {};
template<class T> struct IsVector<std::vector<T>>       : std::true_type  { using value_type = T; };
template<class T> struct IsOptional                     : std::false_type {};
template<class T> struct IsOptional<std::optional<T>>   : std::true_type  { using value_type = T; };
template<class T> struct IsSharedPtr                    : std::false_type {};
template<class T> struct IsSharedPtr<std::shared_ptr<T>>: std::true_type  { using element_type = T; };
}

/// Returns the Lua-facing type name for a C++ type. Primitives, identifiers and the wrapper
/// templates (vector, optional, shared_ptr, raw pointer) are resolved at compile time;
/// everything else goes through `Registry::lookupLuaName`, which is populated by proxy
/// registrations and explicit aliases in `Registry::Registry()`. Unregistered types throw
/// `std::runtime_error` — caught in `BindingsCoverageTest` so misses surface in CI.
template<class T>
inline std::string luaTypeName()
{
	using U = std::remove_cvref_t<T>;
	if constexpr (std::is_same_v<U, bool>)
		return "boolean";
	else if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, const char *>)
		return "string";
	else if constexpr (std::is_floating_point_v<U>)
		return "number";
	else if constexpr (std::is_integral_v<U>)
		return "integer";
	else if constexpr (std::is_base_of_v<IdentifierBase, U>)
		return "integer";
	else if constexpr (detail::IsVector<U>::value)
		return luaTypeName<typename detail::IsVector<U>::value_type>() + "[]";
	else if constexpr (detail::IsOptional<U>::value)
		return luaTypeName<typename detail::IsOptional<U>::value_type>() + "?";
	else if constexpr (detail::IsSharedPtr<U>::value)
		return luaTypeName<typename detail::IsSharedPtr<U>::element_type>();
	else if constexpr (std::is_pointer_v<U>)
		return luaTypeName<std::remove_pointer_t<U>>();
	else
		return Registry::get()->lookupLuaName(std::type_index(typeid(U)));
}

namespace detail
{
template<class Tuple, std::size_t Offset, std::size_t... I>
inline std::vector<std::string> paramTypesImpl(std::index_sequence<I...>)
{
	std::vector<std::string> out;
	(out.push_back(luaTypeName<std::tuple_element_t<I + Offset, Tuple>>()), ...);
	return out;
}

template<class Tuple, std::size_t Offset = 0>
inline std::vector<std::string> paramTypesFromTuple()
{
	static_assert(std::tuple_size_v<Tuple> >= Offset, "Offset exceeds tuple size");
	constexpr std::size_t N = std::tuple_size_v<Tuple> - Offset;
	return paramTypesImpl<Tuple, Offset>(std::make_index_sequence<N>{});
}

template<class Ret>
inline std::string returnTypeOrEmpty()
{
	if constexpr (std::is_void_v<Ret>)
		return {};
	else
		return luaTypeName<Ret>();
}
}

/// Returns the per-argument Lua type names for a C++ member function pointer, one entry per arg.
/// Pairs with `LuaParam` (host-supplied names) to build a full param documentation list.
template<auto Method>
inline std::vector<std::string> paramTypesOfMethod()
{
	using TR = LuaClassMemberTraits<decltype(Method)>;
	return detail::paramTypesFromTuple<typename TR::ArgsTuple>();
}

/// Returns the Lua return type for a C++ member function pointer, empty string for void.
template<auto Method>
inline std::string returnTypeOfMethod()
{
	using TR = LuaClassMemberTraits<decltype(Method)>;
	return detail::returnTypeOrEmpty<typename TR::ReturnType>();
}

/// Returns the per-argument Lua type names for a static proxy helper, dropping the first
/// argument (treated as self).
template<auto Fn>
inline std::vector<std::string> paramTypesOfFunction()
{
	using TR = LuaFunctionTraits<std::remove_pointer_t<std::decay_t<decltype(Fn)>>>;
	using Args = typename TR::ArgsTuple;
	static_assert(std::tuple_size_v<Args> >= 1, "Proxy static helpers must take the object as their first argument");
	return detail::paramTypesFromTuple<Args, /*Offset=*/1>();
}

/// Returns the Lua return type for a static proxy helper, empty string for void.
template<auto Fn>
inline std::string returnTypeOfFunction()
{
	using TR = LuaFunctionTraits<std::remove_pointer_t<std::decay_t<decltype(Fn)>>>;
	return detail::returnTypeOrEmpty<typename TR::ReturnType>();
}

}

VCMI_LIB_NAMESPACE_END
