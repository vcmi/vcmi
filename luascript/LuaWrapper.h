/*
 * LuaWrapper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "LuaCallWrapper.h"
#include "api/LuaRegistrar.h"

/*
 * Original code is LunaWrapper by nornagon.
 * https://lua-users.org/wiki/LunaWrapper
 * Published under the BSD 2-clause license
 * https://opensource.org/licenses/BSD-2-Clause
 *
 */

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

namespace detail
{
	template <typename P, typename U>
	struct Dispatcher
	{
		using ProxyType = P;
		using UDataType = U;

		static void setIndexTable(lua_State * L)
		{
			lua_pushstring(L, "__index");

			lua_newtable(L);

			api::LuaRegistrar reg(L, lua_gettop(L));
			ProxyType::registerMethods(reg);

			lua_rawset(L, -3);
		}

		static void pushStaticTable(lua_State * L)
		{
			lua_newtable(L);

			lua_newtable(L);

			setIndexTable(L);

			lua_pushstring(L, "__newindex");
			lua_pushnil(L);
			lua_rawset(L, -3);

			lua_setmetatable(L, -2);
		}

		static int destructor(lua_State * L)
		{
			static const auto KEY = api::Registry::get()->getTypeName<UDataType>();

			void * objPtr = luaL_checkudata(L, 1, KEY);
			if(objPtr)
			{
				auto obj = static_cast<UDataType *>(objPtr);
				obj->~UDataType();
			}
			lua_settop(L, 0);
			return 0;
		}
	};
}

/// Base Registar implementation providing empty hooks for metatable and static table customization.
class RegistarBase : public api::Registar
{
public:
protected:

	virtual void adjustMetatable(lua_State * L) const
	{

	}

	virtual void adjustStaticTable(lua_State * L) const
	{

	}
};

/// Registers a raw-pointer API class in the Lua registry, creating metatables for both mutable and const pointer variants.
template<class T, class Proxy = T>
class RawPointerWrapper : public RegistarBase
{
public:
	using ObjectType = typename std::remove_cv_t<T>;
	using UDataType = ObjectType *;
	using CUDataType = const ObjectType *;

	static_assert(std::is_base_of_v<TagRawPointer, ObjectType>, "Class must inherit from ApiRawPointer to be used with this class!");
	static_assert(!std::is_base_of_v<TagSharedPointer, ObjectType>, "Class must not inherit from ApiSharedPointer to be used with this class!");
	static_assert(!std::is_base_of_v<TagCopyable, ObjectType>, "Class must not inherit from ApiCopyable to be used with this class!");

	void collectDocs(api::MethodRegistrar & sink) const final
	{
		Proxy::registerMethods(sink);
	}

	std::string_view getDescription() const final
	{
		return Proxy::luaDescription;
	}

	void pushMetatable(lua_State * L) const final
	{
		static const auto KEY = api::Registry::get()->getTypeName<UDataType>();
		static const auto S_KEY = api::Registry::get()->getTypeName<CUDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
			adjustMetatable(L);

		S.restoreInitialTop();

		if(luaL_newmetatable(L, S_KEY) != 0)
			adjustMetatable(L);

		S.restoreInitialTop();

		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);

		adjustStaticTable(L);
	}

protected:
	void adjustMetatable(lua_State * L) const final
	{
		detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);

		lua_pushstring(L, "__eq");
		lua_pushcfunction(L, &equalityImpl);
		lua_rawset(L, -3);
	}

private:
	static int equalityImpl(lua_State * L)
	{
		void * lhsRaw = lua_touserdata(L, 1);
		void * rhsRaw = lua_touserdata(L, 2);

		if(!lhsRaw || !rhsRaw)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		// Userdata block holds a T* (or const T*) by value; read both as void* to compare addresses.
		void * lhsPtr = *static_cast<void **>(lhsRaw);
		void * rhsPtr = *static_cast<void **>(rhsRaw);

		lua_pushboolean(L, lhsPtr == rhsPtr ? 1 : 0);
		return 1;
	}
};

/// Registers a shared_ptr API class in the Lua registry; manages lifetime via __gc and supports a default constructor.
template<class T, class Proxy = T>
class SharedPointerWrapper : public RegistarBase
{
public:
	using ObjectType = typename std::remove_cv_t<T>;
	using UDataType = std::shared_ptr<T>;
	static_assert(std::is_base_of_v<TagSharedPointer, ObjectType>, "Class must inherit from ApiSharedPointer to be used with this class!");
	static_assert(!std::is_base_of_v<TagRawPointer, ObjectType>, "Class must not inherit from ApiRawPointer to be used with this class!");
	static_assert(!std::is_base_of_v<TagCopyable, ObjectType>, "Class must not inherit from ApiCopyable to be used with this class!");

	void collectDocs(api::MethodRegistrar & sink) const final
	{
		Proxy::registerMethods(sink);
	}

	std::string_view getDescription() const final
	{
		return Proxy::luaDescription;
	}

	static int constructor(lua_State * L)
	{
		LuaStack S(L);
		S.clear();//we do not accept any parameters in constructor
		auto obj = std::make_shared<T>();
		S.push(obj);
		return 1;
	}

	void pushMetatable(lua_State * L) const final
	{
		static const auto KEY = api::Registry::get()->getTypeName<UDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
		{
			adjustMetatable(L);

			S.push("__gc");
			lua_pushcfunction(L, &(detail::Dispatcher<Proxy, UDataType>::destructor));
			lua_rawset(L, -3);
		}

		S.restoreInitialTop();

		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);

		adjustStaticTable(L);
	}
protected:
	void adjustMetatable(lua_State * L) const final
	{
		detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);

		lua_pushstring(L, "__eq");
		lua_pushcfunction(L, &equalityImpl);
		lua_rawset(L, -3);
	}

private:
	static int equalityImpl(lua_State * L)
	{
		void * lhsRaw = lua_touserdata(L, 1);
		void * rhsRaw = lua_touserdata(L, 2);

		if(!lhsRaw || !rhsRaw)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		auto * lhs = static_cast<UDataType *>(lhsRaw);
		auto * rhs = static_cast<UDataType *>(rhsRaw);

		lua_pushboolean(L, lhs->get() == rhs->get() ? 1 : 0);
		return 1;
	}
};

/// Registers a by-value (copyable) API class in the Lua registry; copies the object into Lua userdata with __gc.
template<class T, class Proxy = T>
class CopyableWrapper : public RegistarBase
{
public:
	using ObjectType = typename std::remove_cv_t<T>;
	using UDataType = T;
	static_assert(std::is_base_of_v<TagCopyable, ObjectType>, "Class must inherit from ApiCopyable to be used with this class!");
	static_assert(!std::is_base_of_v<TagRawPointer, ObjectType>, "Class must not inherit from ApiRawPointer to be used with this class!");
	static_assert(!std::is_base_of_v<TagSharedPointer, ObjectType>, "Class must not inherit from ApiSharedPointer to be used with this class!");

	void collectDocs(api::MethodRegistrar & sink) const final
	{
		Proxy::registerMethods(sink);
	}

	std::string_view getDescription() const final
	{
		return Proxy::luaDescription;
	}

	static int constructor(lua_State * L)
	{
		LuaStack S(L);
		S.clear();//we do not accept any parameters in constructor
		ObjectType obj;
		S.push(obj);
		return 1;
	}

	void pushMetatable(lua_State * L) const final
	{
		static const auto KEY = api::Registry::get()->getTypeName<UDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
		{
			adjustMetatable(L);

			S.push("__gc");
			lua_pushcfunction(L, &(detail::Dispatcher<Proxy, UDataType>::destructor));
			lua_rawset(L, -3);
		}

		S.restoreInitialTop();

		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);

		adjustStaticTable(L);
	}
protected:
	void adjustMetatable(lua_State * L) const final
	{
		detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);

		if constexpr(std::equality_comparable<UDataType>)
		{
			lua_pushstring(L, "__eq");
			lua_pushcfunction(L, &equalityImpl);
			lua_rawset(L, -3);
		}
	}

private:
	static int equalityImpl(lua_State * L) requires std::equality_comparable<UDataType>
	{
		void * lhsRaw = lua_touserdata(L, 1);
		void * rhsRaw = lua_touserdata(L, 2);

		if(!lhsRaw || !rhsRaw)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		auto * lhs = static_cast<UDataType *>(lhsRaw);
		auto * rhs = static_cast<UDataType *>(rhsRaw);

		lua_pushboolean(L, *lhs == *rhs ? 1 : 0);
		return 1;
	}
};


}

VCMI_LIB_NAMESPACE_END
