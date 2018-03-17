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

#include "api/Registry.h"
#include "LuaFunctor.h"

/*
 * Original code is LunaWrapper by nornagon.
 * https://lua-users.org/wiki/LunaWrapper
 * Published under the BSD 2-clause license
 * https://opensource.org/licenses/BSD-2-Clause
 *
 */

namespace scripting
{

namespace detail
{
	template<typename T>
	struct RegType
	{
		const char * name;
		std::function<int(lua_State *, T)> functor;
	};

	struct CustomRegType
	{
		const char * name;
		lua_CFunction functor;
		bool isStatic;
	};

	template <typename P, typename U>
	struct Dispatcher
	{
		using ProxyType = P;
		using UDataType = U;

		static int invoke(lua_State * L)
		{
			int i = (int)lua_tonumber(L, lua_upvalueindex(1));

			void * raw = luaL_checkudata(L, 1, api::TypeRegistry::get()->getKey<UDataType>());

			if(!raw)
			{
				lua_settop(L, 0);
				return 0;
			}

			lua_remove(L, 1);

			auto obj = *(static_cast<UDataType *>(raw));
			return (ProxyType::REGISTER[i].functor)(L, obj);
		}

		static void pushIndexTable(lua_State * L)
		{
			lua_newtable(L);
			lua_Integer index = 0;
			for(auto & reg : ProxyType::REGISTER)
			{
				lua_pushstring(L, reg.name);
				lua_pushnumber(L, index);
				lua_pushcclosure(L, &invoke, 1);
				lua_settable(L, -3);
				index++;
			}
		}
	};
}

template<class T, class Proxy = T> class OpaqueWrapper
{
public:
	using ObjectType = typename std::remove_cv<T>::type;
	using UDataType = T *;
    using SelfType = OpaqueWrapper<T, Proxy>;
	using RegType = detail::RegType<UDataType>;

	static int registrator(lua_State * L, api::TypeRegistry * typeRegistry)
	{
		if(luaL_newmetatable(L, typeRegistry->getKey<UDataType>()) != 0)
		{
			lua_pushstring(L, "__index");
			detail::Dispatcher<Proxy, UDataType>::pushIndexTable(L);
			lua_settable(L, -3);
		}

		lua_settop(L, 0);
		lua_newtable(L);
		return 1;
	}
};

template<class T, class Proxy = T> class OpaqueWrapperEx
{
public:
	using ObjectType = typename std::remove_cv<T>::type;
	using UDataType = T *;
    using SelfType = OpaqueWrapper<T, Proxy>;
	using RegType = detail::RegType<UDataType>;
	using CustomRegType = detail::CustomRegType;

	static int registrator(lua_State * L, api::TypeRegistry * typeRegistry)
	{
		if(luaL_newmetatable(L, typeRegistry->getKey<UDataType>()) != 0)
		{
			lua_pushstring(L, "__index");
			detail::Dispatcher<Proxy, UDataType>::pushIndexTable(L);

			for(auto & reg : Proxy::REGISTER_CUSTOM)
			{
				if(!reg.isStatic)
				{
					lua_pushstring(L, reg.name);
					lua_pushcclosure(L, reg.functor, 0);
					lua_settable(L, -3);
				}
			}

			lua_settable(L, -3);
		}

		lua_settop(L, 0);

		lua_newtable(L);

		for(auto & reg : Proxy::REGISTER_CUSTOM)
		{
			if(reg.isStatic)
			{
				lua_pushstring(L, reg.name);
				lua_pushcclosure(L, reg.functor, 0);
				lua_settable(L, -3);
			}
		}

		return 1;
	}
};

template<class T, class Proxy = T> class SharedWrapper
{
public:
	using UDataType = std::shared_ptr<T>;
	using SelfType = SharedWrapper<T, Proxy>;
	using RegType = detail::RegType<UDataType>;

	static int registrator(lua_State * L, api::TypeRegistry * typeRegistry)
	{
		if(luaL_newmetatable(L, typeRegistry->getKey<UDataType>()) != 0)
		{
			lua_pushstring(L, "__index");
			detail::Dispatcher<Proxy, UDataType>::pushIndexTable(L);
			lua_settable(L, -3);

			lua_pushstring(L, "__gc");
			lua_pushcfunction(L, &(SelfType::destructor));
			lua_settable(L, -3);
		}

		lua_settop(L, 0);

		lua_newtable(L);
		lua_pushstring(L, "new");
		lua_pushcfunction(L, &(SelfType::constructor));
		lua_settable(L, -3);

		return 1;
	}

	static int constructor(lua_State * L)
	{
		lua_settop(L, 0);//we do not accept any parameters in constructor

		auto obj = std::make_shared<T>();

		lua_newtable(L);

		void * raw = lua_newuserdata(L, sizeof(UDataType));

		if(!raw)
		{
			lua_settop(L, 0);
			return 0;
		}

		new(raw) UDataType(obj);

		luaL_getmetatable(L, api::TypeRegistry::get()->getKey<UDataType>());

		if(!lua_istable(L, -1))
		{
			lua_settop(L, 0);
			return 0;
		}

		lua_setmetatable(L, -2);
		return 1;
	}

	static int destructor(lua_State * L)
	{
		void * objPtr = luaL_checkudata(L, 1, api::TypeRegistry::get()->getKey<UDataType>());
		if(objPtr)
		{
			auto obj = static_cast<UDataType *>(objPtr);
			obj->reset();
		}
		lua_settop(L, 0);
		return 0;
	}
};

template<class T, class Proxy = T> class UniqueOpaqueWrapper
{
public:
	using UDataType = std::unique_ptr<T>;
	using SelfType = UniqueOpaqueWrapper<T, Proxy>;
	using RegType = detail::RegType<UDataType>;

	static int registrator(lua_State * L, api::TypeRegistry * typeRegistry)
	{
		if(luaL_newmetatable(L, typeRegistry->getKey<UDataType>()) != 0)
		{
//			lua_pushstring(L, "__index");
//			detail::Dispatcher<Proxy, UDataType>::pushIndexTable(L);
//			lua_settable(L, -3);

			lua_pushstring(L, "__gc");
			lua_pushcfunction(L, &(SelfType::destructor));
			lua_settable(L, -3);
		}

		lua_settop(L, 0);
		lua_newtable(L);
		return 1;
	}

	static int destructor(lua_State * L)
	{
		void * objPtr = luaL_checkudata(L, 1, api::TypeRegistry::get()->getKey<UDataType>());
		if(objPtr)
		{
			auto obj = static_cast<UDataType *>(objPtr);
			obj->reset();
		}
		lua_settop(L, 0);
		return 0;
	}
};

}
