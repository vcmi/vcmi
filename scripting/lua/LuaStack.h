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

class JsonNode;

namespace scripting
{

class LuaStack
{
public:
	LuaStack(lua_State * L_);
	LuaStack(lua_State * L_, api::TypeRegistry * typeRegistry_);
	void balance();
	void clear();

	void pushByIndex(lua_Integer index);

	void pushNil();
	void pushInteger(lua_Integer value);
	void push(bool value);
	void push(const std::string & value);

	template<typename T>
	void push(const boost::optional<T> & value)
	{
		if(value.is_initialized())
			push(value.get());
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

	template<typename T, typename std::enable_if< std::is_class<T>::value, int>::type = 0>
	void push(const T * value)
	{
		pushObject<const T>(value);
	}

	template<typename T, typename std::enable_if< std::is_class<T>::value, int>::type = 0>
	void push(T * value)
	{
		pushObject<T>(value);
	}


	template<typename T, typename std::enable_if< std::is_class<T>::value, int>::type = 0>
	void push(std::unique_ptr<T> && value)
	{
		if(!value)
		{
			pushNil();
			return;
		}

		using UData = std::unique_ptr<T>;

		void * raw = lua_newuserdata(L, sizeof(UData));

		if(!raw)
		{
			pushNil();
			return;
		}

		new(raw) UData(std::move(value));

		luaL_getmetatable(L, typeRegistry->getKey<UData>());
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

	bool tryGet(int position, double & value);
	bool tryGet(int position, std::string & value);

	template<typename T, typename std::enable_if< std::is_class<T>::value, int>::type = 0>
	bool tryGet(int position, T * & value)
	{
		void * raw = luaL_checkudata(L, position, typeRegistry->getKey<T *>());

		if(!raw)
			return false;

		value = *(static_cast<T **>(raw));
		return true;
	}

	template<typename T>
	bool tryGet(int position, std::shared_ptr<T> & value)
	{
		void * raw = luaL_checkudata(L, position, typeRegistry->getKey<std::shared_ptr<T>>());

		if(!raw)
			return false;

		value = *(static_cast<std::shared_ptr<T> *>(raw));
		return true;
	}

	bool tryGet(int position, JsonNode & value);

	int retNil();
	int retVoid();

private:
	lua_State * L;
	api::TypeRegistry * typeRegistry;
	int initialTop;

	template<typename Object>
	void pushObject(Object * value)
	{
		if(value)
		{
			using UData = Object *;

			void * raw = lua_newuserdata(L, sizeof(UData));

			UData * ptr = static_cast<UData *>(raw);
			*ptr = value;

			luaL_getmetatable(L, typeRegistry->getKey<UData>());
//			if(!lua_istable(L, -1))
//			{
//				lua_pushstring(L, "internal error");
//				lua_error(L);
//			}
			lua_setmetatable(L, -2);
		}
		else
		{
			pushNil();
		}
	}
};

}
