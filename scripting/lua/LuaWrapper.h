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
#include "LuaStack.h"

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
			static auto KEY = api::TypeRegistry::get()->getKey<UDataType>();

			int i = (int)lua_tonumber(L, lua_upvalueindex(1));

			void * raw = luaL_checkudata(L, 1, KEY);

			if(!raw)
			{
				lua_settop(L, 0);
				return 0;
			}

			lua_remove(L, 1);

			auto obj = *(static_cast<UDataType *>(raw));
			return (ProxyType::REGISTER[i].functor)(L, obj);
		}

		static void setIndexTable(lua_State * L)
		{
			lua_pushstring(L, "__index");

			lua_newtable(L);
			lua_Integer index = 0;
			for(auto & reg : ProxyType::REGISTER)
			{
				lua_pushstring(L, reg.name);
				lua_pushnumber(L, index);
				lua_pushcclosure(L, &invoke, 1);
				lua_rawset(L, -3);
				index++;
			}

			for(auto & reg : ProxyType::REGISTER_CUSTOM)
			{
				if(!reg.isStatic)
				{
					lua_pushstring(L, reg.name);
					lua_pushcclosure(L, reg.functor, 0);
					lua_rawset(L, -3);
				}
			}
			lua_rawset(L, -3);
		}

		static void pushStaticTable(lua_State * L)
		{
			lua_newtable(L);

			lua_newtable(L);

			lua_pushstring(L, "__index");
			{
				lua_newtable(L);

				for(auto & reg : ProxyType::REGISTER_CUSTOM)
				{
					if(reg.isStatic)
					{
						lua_pushstring(L, reg.name);
						lua_pushcclosure(L, reg.functor, 0);
						lua_rawset(L, -3);
					}
				}
			}

			lua_rawset(L, -3);

			lua_pushstring(L, "__newindex");
			lua_pushnil(L);
			lua_rawset(L, -3);

			lua_setmetatable(L, -2);
		}

		static int destructor(lua_State * L)
		{
			static auto KEY = api::TypeRegistry::get()->getKey<UDataType>();

			void * objPtr = luaL_checkudata(L, 1, KEY);
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

template<class T, class Proxy = T>
class OpaqueWrapper : public RegistarBase
{
public:
	using ObjectType = typename std::remove_cv<T>::type;
	using UDataType = T *;
	using RegType = detail::RegType<UDataType>;
	using CustomRegType = detail::CustomRegType;

	void pushMetatable(lua_State * L) const override final
	{
		static auto KEY = api::TypeRegistry::get()->getKey<UDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
			adjustMetatable(L);

		S.balance();

		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);

		adjustStaticTable(L);
	}

protected:
	void adjustMetatable(lua_State * L) const override
	{
		detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);
	}
};

template<class T, class Proxy = T>
class SharedWrapper : public RegistarBase
{
public:
	using ObjectType = typename std::remove_cv<T>::type;
	using UDataType = std::shared_ptr<T>;
	using RegType = detail::RegType<UDataType>;
	using CustomRegType = detail::CustomRegType;

	static int constructor(lua_State * L)
	{
		LuaStack S(L);
		S.clear();//we do not accept any parameters in constructor
		auto obj = std::make_shared<T>();
		S.push(obj);
		return 1;
	}

	void pushMetatable(lua_State * L) const override final
	{
		static auto KEY = api::TypeRegistry::get()->getKey<UDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
		{
			adjustMetatable(L);

			S.push("__gc");
			lua_pushcfunction(L, &(detail::Dispatcher<Proxy, UDataType>::destructor));
			lua_rawset(L, -3);
		}

		S.balance();

		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);

		adjustStaticTable(L);
	}
protected:
	void adjustMetatable(lua_State * L) const override
	{
		detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);
	}
};

template<class T, class Proxy = T>
class UniqueOpaqueWrapper : public api::Registar
{
public:
	using ObjectType = typename std::remove_cv<T>::type;
	using UDataType = std::unique_ptr<T>;
	using RegType = detail::RegType<UDataType>;
	using CustomRegType = detail::CustomRegType;

	void pushMetatable(lua_State * L) const override final
	{
		static auto KEY = api::TypeRegistry::get()->getKey<UDataType>();

		LuaStack S(L);

		if(luaL_newmetatable(L, KEY) != 0)
		{
//			detail::Dispatcher<Proxy, UDataType>::setIndexTable(L);

			S.push("__gc");
			lua_pushcfunction(L, &(detail::Dispatcher<Proxy, UDataType>::destructor));
			lua_rawset(L, -3);
		}

		S.balance();
		detail::Dispatcher<Proxy, UDataType>::pushStaticTable(L);
	}
};

}
