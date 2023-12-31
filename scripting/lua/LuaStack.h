/*
 * LuaStack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "api/Registry.h"
#include "../../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class int3;

namespace scripting
{

namespace detail
{
	template<typename T>
	struct IsRegularClass
	{
		static constexpr auto value = std::is_class<T>::value && !std::is_base_of<IdentifierBase, T>::value;
	};

	template<typename T>
	struct IsIdClass
	{
		static constexpr auto value = std::is_class<T>::value && std::is_base_of<IdentifierBase, T>::value;
	};
}

class LuaStack
{
public:
	LuaStack(lua_State * L_);
	void balance();
	void clear();

	void pushByIndex(lua_Integer index);

	void pushNil();
	void pushInteger(lua_Integer value);
	void push(bool value);
	void push(const char * value);
	void push(const std::string & value);
	void push(const JsonNode & value);

	template<typename T>
	void push(const std::optional<T> & value)
	{
		if(value.has_value())
			push(value.value());
		else
			pushNil();
	}

	template<typename T, typename std::enable_if< std::is_integral<T>::value && !std::is_same<T, bool>::value, int>::type = 0>
	void push(const T value)
	{
		pushInteger(static_cast<lua_Integer>(value));
	}

	template<typename T, typename std::enable_if< std::is_enum<T>::value, int>::type = 0>
	void push(const T value)
	{
		pushInteger(static_cast<lua_Integer>(value));
	}

	void push(const int3 & value);

	template<typename T, typename std::enable_if< detail::IsIdClass<T>::value, int>::type = 0>
	void push(const T & value)
	{
		pushInteger(static_cast<lua_Integer>(value.getNum()));
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value, int>::type = 0>
	void push(T * value)
	{
		using UData = T *;
		static auto KEY = api::TypeRegistry::get()->getKey<UData>();

		if(!value)
		{
			pushNil();
			return;
		}

		void * raw = lua_newuserdata(L, sizeof(UData));
		if(!raw)
		{
			pushNil();
			return;
		}

		auto * ptr = static_cast<UData *>(raw);
		*ptr = value;

		luaL_getmetatable(L, KEY);
		lua_setmetatable(L, -2);
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value, int>::type = 0>
	void push(std::shared_ptr<T> value)
	{
		using UData = std::shared_ptr<T>;
		static auto KEY = api::TypeRegistry::get()->getKey<UData>();

		if(!value)
		{
			pushNil();
			return;
		}

		void * raw = lua_newuserdata(L, sizeof(UData));

		if(!raw)
		{
			pushNil();
			return;
		}

		new(raw) UData(value);

		luaL_getmetatable(L, KEY);
		lua_setmetatable(L, -2);
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value, int>::type = 0>
	void push(std::unique_ptr<T> && value)
	{
		using UData = std::unique_ptr<T>;
		static auto KEY = api::TypeRegistry::get()->getKey<UData>();

		if(!value)
		{
			pushNil();
			return;
		}

		void * raw = lua_newuserdata(L, sizeof(UData));

		if(!raw)
		{
			pushNil();
			return;
		}

		new(raw) UData(std::move(value));

		luaL_getmetatable(L, KEY);
		lua_setmetatable(L, -2);
	}

	bool tryGetInteger(int position, lua_Integer & value);

	bool tryGet(int position, bool & value);

	template<typename T, typename std::enable_if< std::is_integral<T>::value && !std::is_same<T, bool>::value, int>::type = 0>
	bool tryGet(int position, T & value)
	{
		lua_Integer temp;
		if(tryGetInteger(position, temp))
		{
			value = static_cast<T>(temp);
			return true;
		}
		else
		{
			return false;
		}
	}

	template<typename T, typename std::enable_if<detail::IsIdClass<T>::value, int>::type = 0>
	bool tryGet(int position, T & value)
	{
		lua_Integer temp;
		if(tryGetInteger(position, temp))
		{
			value = T(temp);
			return true;
		}
		else
		{
			return false;
		}
	}

	template<typename T, typename std::enable_if< std::is_enum<T>::value, int>::type = 0>
	bool tryGet(int position, T & value)
	{
		lua_Integer temp;
		if(tryGetInteger(position, temp))
		{
			value = static_cast<T>(temp);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool tryGet(int position, int3 & value);

	bool tryGet(int position, double & value);
	bool tryGet(int position, std::string & value);

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value && std::is_const<T>::value, int>::type = 0>
	STRONG_INLINE bool tryGet(int position, T * & value)
	{
		using NCValue = typename std::remove_const<T>::type;

		using UData = NCValue *;
		using CUData = T *;

		return tryGetCUData<T *, UData, CUData>(position, value);
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value && !std::is_const<T>::value, int>::type = 0>
	STRONG_INLINE bool tryGet(int position, T * & value)
	{
		return tryGetUData<T *>(position, value);
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value && std::is_const<T>::value, int>::type = 0>
	STRONG_INLINE bool tryGet(int position, std::shared_ptr<T> & value)
	{
		using NCValue = typename std::remove_const<T>::type;

		using UData = std::shared_ptr<NCValue>;
		using CUData = std::shared_ptr<T>;

		return tryGetCUData<std::shared_ptr<T>, UData, CUData>(position, value);
	}

	template<typename T, typename std::enable_if<detail::IsRegularClass<T>::value && !std::is_const<T>::value, int>::type = 0>
	STRONG_INLINE bool tryGet(int position, std::shared_ptr<T> & value)
	{
		return tryGetUData<std::shared_ptr<T>>(position, value);
	}

	template<typename U>
	bool tryGetUData(int position, U & value)
	{
		static auto KEY = api::TypeRegistry::get()->getKey<U>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
			return false;

		if(lua_getmetatable(L, position) == 0)
			return false;

		lua_getfield(L, LUA_REGISTRYINDEX, KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<U *>(raw));
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
		return false;
	}

	template<typename T, typename U, typename CU>
	bool tryGetCUData(int position, T & value)
	{
		static auto KEY = api::TypeRegistry::get()->getKey<U>();
		static auto C_KEY = api::TypeRegistry::get()->getKey<CU>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
			return false;

		if(lua_getmetatable(L, position) == 0)
			return false;

		//top is metatable

		lua_getfield(L, LUA_REGISTRYINDEX, KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<U *>(raw));
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 1);

		//top is metatable

		lua_getfield(L, LUA_REGISTRYINDEX, C_KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<CU *>(raw));
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
		return false;
	}

	bool tryGet(int position, JsonNode & value);

	int retNil();
	int retVoid();

	STRONG_INLINE
	int retPushed()
	{
		return lua_gettop(L);
	}

	inline bool isFunction(int position)
	{
		return lua_isfunction(L, position);
	}

	inline bool isNumber(int position)
	{
		return lua_isnumber(L, position);
	}

	static int quickRetBool(lua_State * L, bool value)
	{
		lua_settop(L, 0);
		lua_pushboolean(L, value);
		return 1;
	}

	template<typename T>
	static int quickRetInt(lua_State * L, const T & value)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, static_cast<int32_t>(value));
		return 1;
	}

	template<std::size_t T>
	static int quickRetInt(lua_State * L, const std::bitset<T> & value)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, static_cast<int32_t>(value.to_ulong()));
		return 1;
	}

	static int quickRetStr(lua_State * L, const std::string & value)
	{
		lua_settop(L, 0);
		lua_pushlstring(L, value.c_str(), value.size());
		return 1;
	}

private:
	lua_State * L;
	int initialTop;
};

}

VCMI_LIB_NAMESPACE_END
