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
#include "../lib/constants/IdentifierBase.h"
#include "vcmi/scripting/ApiTags.h"
#include <boost/core/demangle.hpp>

VCMI_LIB_NAMESPACE_BEGIN
namespace scripting::api { template<typename E> struct EnumGroup; }
VCMI_LIB_NAMESPACE_END


VCMI_LIB_NAMESPACE_BEGIN

class BattleHex;
class JsonNode;
class int3;
class CreatureID;
class Creature;

namespace scripting
{

/// Exception thrown by LuaStack when a type mismatch or missing value is detected while reading the Lua stack.
class LuaApiException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class LuaStack;

/// Typed helper for Lua stack operations: push/get for all VCMI types and API objects.
/// Records the initial stack top on construction; call restoreInitialTop() to reset to it.
/// All get() overloads throw LuaApiException on type mismatch.
class LuaStack
{
public:
	LuaStack(lua_State * L_);
	void restoreInitialTop();
	void clear();
	void debugPrintStack();

	template<typename T>
	LuaStack & operator<<(const T & parameter)
	{
		push(parameter);
		return *this;
	}

	/// Converts stack position relative to top (e.g. -1) into absolute position
	/// Behavior is identical to `lua_absindex` function, available from Lua 5.2
	int absindex(int idx);

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

	template<typename T, typename std::enable_if_t< std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
	void push(const T value)
	{
		pushInteger(static_cast<lua_Integer>(value));
	}

	template<typename T, typename std::enable_if_t< std::is_enum_v<T>, int> = 0>
	void push(const T value)
	{
		pushInteger(static_cast<lua_Integer>(value));
	}

	void push(const int3 & value);
	void push(const CreatureID & value);

	template<typename T, typename std::enable_if_t< std::is_base_of_v<IdentifierBase, T>, int> = 0>
	void push(const T & value)
	{
		pushInteger(static_cast<lua_Integer>(value.getNum()));
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagRawPointer, T>, int> = 0>
	void push(T * value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using PtrType = std::conditional_t<std::is_const_v<T>, const BaseType *, BaseType *>;
		static const auto KEY = api::Registry::get()->getTypeName<PtrType>();

		if(value == nullptr)
		{
			pushNil();
			return;
		}

		createLuaUserdata<PtrType>(value);

		luaL_getmetatable(L, KEY);
		if(lua_isnil(L, -1))
			throw LuaApiException(std::string("Unregistered type pushed on Lua stack: ") + KEY);

		lua_setmetatable(L, -2);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSharedPointer, T>, int> = 0>
	void push(std::shared_ptr<T> value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using PtrType = std::conditional_t<std::is_const_v<T>, std::shared_ptr<const BaseType>, std::shared_ptr<BaseType>>;
		static const auto KEY = api::Registry::get()->getTypeName<PtrType>();

		if(value == nullptr)
		{
			pushNil();
			return;
		}

		createLuaUserdata<PtrType>(value);

		luaL_getmetatable(L, KEY);
		if(lua_isnil(L, -1))
			throw LuaApiException(std::string("Unregistered type pushed on Lua stack: ") + KEY);

		lua_setmetatable(L, -2);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagCopyable, T>, int> = 0>
	void push(const T & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		static_assert(std::is_same_v<std::remove_const_t<DataType>, BaseType>, "Can not push derived class as copyable!");

		static const auto KEY = api::Registry::get()->getTypeName<BaseType>();

		createLuaUserdata<BaseType>(value);

		luaL_getmetatable(L, KEY);
		if(lua_isnil(L, -1))
			throw LuaApiException(std::string("Unregistered type pushed on Lua stack: ") + KEY);

		lua_setmetatable(L, -2);
	}

	template<typename T, std::size_t N>
	void push(const boost::container::small_vector<T,N> & value)
	{
		pushArray(value);
	}

	template<typename T>
	void push(const std::vector<T> & value)
	{
		pushArray(value);
	}

	template<typename T>
	void push(const std::map<std::string, T> & value)
	{
		lua_newtable(L);
		int tableIndex = lua_gettop(L);

		for (const auto &entry : value)
		{
			push(entry.second);
			lua_setfield(L, tableIndex, entry.first.c_str());
		}
	}

	template<typename E>
	void push(const api::EnumGroup<E> & group)
	{
		lua_newtable(L);
		int tableIndex = lua_gettop(L);

		for(const auto & item : group.items)
		{
			push(item.value);
			lua_setfield(L, tableIndex, item.key.c_str());
		}
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSerializable, T>, int> = 0>
	void push(const T & value)
	{
		lua_newtable(L);
		int tableIndex = lua_gettop(L);

		// get non-const value - ugly, but required since same template method is used for deserialization
		T & nonConstValue = const_cast<T&>(value);

		const auto & luaSerializer = [this, tableIndex]<typename Field>(const std::string &keyName, const Field & data, std::string_view /*description*/)
		{
			push(data);
			lua_setfield(L, tableIndex, keyName.c_str());
		};

		nonConstValue.serializeScript(luaSerializer);
	}

	void get(int position, bool & value);

	template<typename T, typename std::enable_if_t< std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
	void get(int position, T & value)
	{
		lua_Integer temp;
		getInteger(position, temp);
		value = static_cast<T>(temp);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<IdentifierBase, T>, int> = 0>
	void get(int position, T & value)
	{
		if constexpr (requires { T::decode(std::string{}); })
		{
			if(lua_isstring(L, position) && !lua_isnumber(L, position))
			{
				std::string temp;
				get(position, temp);
				value = T(T::decode(temp));
				return;
			}
		}
		lua_Integer temp;
		getInteger(position, temp);
		value = T(temp);
	}

	template<typename T, typename std::enable_if_t< std::is_enum_v<T>, int> = 0>
	void get(int position, T & value)
	{
		lua_Integer temp;
		getInteger(position, temp);
		value = static_cast<T>(temp);
	}

	void get(int position, int3 & value);
	void get(int position, BattleHex & value);
	void get(int position, CreatureID & value);

	void get(int position, double & value);
	void get(int position, std::string & value);

	template<typename T>
	void get(int idx, std::vector<T> & out)
	{
		if (!lua_istable(L, idx))
			throw LuaApiException("value at index is not a table");

#if LUA_VERSION_NUM < 503
		size_t len = lua_objlen(L, idx);
#else
		size_t len = lua_rawlen(L, idx);
#endif
		out.resize(len);

		for (size_t i = 0; i < len; ++i) {
			lua_rawgeti(L, idx, i+1);
			get(-1, out[i]);
			lua_pop(L, 1);
		}
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSerializable, T>, int> = 0>
	inline void get(int position, T & value)
	{
		const auto & deserializer = [this, position]<typename Data>(const std::string &keyName, Data & data, std::string_view /*description*/)
		{
			if (!lua_istable(L, position))
				throw LuaApiException("value at index is not a table");

			lua_getfield(L, position, keyName.c_str());

			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				return;
			}

			try {
				get(absindex(-1), data);
			} catch (...) {
				lua_pop(L, 1);
				throw;
			}

			lua_pop(L, 1);
		};

		value.serializeScript(deserializer);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagRawPointer, T> && std::is_const_v<T>, int> = 0>
	inline void get(int position, T * & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = BaseType *;
		using ConstPtrType = const BaseType *;

		ConstPtrType basePtr = nullptr;
		getCUData<BasePtrType, ConstPtrType>(position, basePtr);
		value = dynamic_cast<T*>(basePtr);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagRawPointer, T> && !std::is_const_v<T>, int> = 0>
	inline void get(int position, T * & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = BaseType*;

		BasePtrType basePtr = nullptr;
		getUData<BasePtrType>(position, basePtr);
		value = dynamic_cast<T*>(basePtr);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSharedPointer, T> && std::is_const_v<T>, int> = 0>
	inline void get(int position, std::shared_ptr<T> & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = std::shared_ptr<BaseType>;
		using ConstPtrType = std::shared_ptr<const BaseType>;

		ConstPtrType basePtr;
		getCUData<BasePtrType, ConstPtrType>(position, basePtr);
		value = std::dynamic_pointer_cast<T>(basePtr);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSharedPointer, T> && !std::is_const_v<T>, int> = 0>
	inline void get(int position, std::shared_ptr<T> & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = std::shared_ptr<BaseType>;

		BasePtrType basePtr;
		getUData<BasePtrType>(position, basePtr);
		value = std::dynamic_pointer_cast<T>(basePtr);
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagCopyable, T>, int> = 0>
	inline void get(int position, T & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		static_assert(std::is_same_v<DataType, BaseType>, "Can not push derived class as copyable!");

		getUData<BaseType>(position, value);
	}

	template<typename T>
	void getNonNull(int position, T & value)
	{
		get(position, value);
		if (value == nullptr)
		{
			std::string expectedType = boost::core::demangle(typeid(T).name());
			std::string message = std::string("Invalid Lua value! Expected ") + expectedType + " at position " + std::to_string(position) + ", but found nil value!";
			throw LuaApiException( message );
		}
	}

	void get(int position, JsonNode & value);

	int retVoid();

	/// Returns the current stack size.
	inline int stackSize()
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

private:
	void getInteger(int position, lua_Integer & value);

	template<typename T>
	void pushArray(T & value)
	{
		lua_newtable(L);
		int tableIndex = lua_gettop(L);

		for (size_t i = 0; i < value.size(); ++i)
		{
			push(value[i]);
			lua_rawseti(L, tableIndex, i + 1);
		}
	}

	template<typename BaseType>
	void getUData(int position, BaseType & value)
	{
		if constexpr (std::is_assignable_v<std::remove_cvref_t<BaseType>&, std::nullptr_t>)
		{
			if (lua_isnil(L, position))
			{
				value = nullptr;
				return;
			}
		}

		static const auto KEY = api::Registry::get()->getTypeName<BaseType>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
		{
			std::string expectedType = boost::core::demangle(typeid(BaseType).name());
			const char * actualType = lua_typename(L, lua_type(L, position));
			std::string message = std::string("Invalid Lua value! Expected ") + expectedType + " at position " + std::to_string(position) + ", but found " + actualType;
			throw LuaApiException( message );
		}

		if(lua_getmetatable(L, position) == 0)
			throw LuaApiException( "Invalid Lua value! Found userdata without assigned metatable!");

		lua_getfield(L, LUA_REGISTRYINDEX, KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<BaseType *>(raw));
			lua_pop(L, 2);
			return;
		}

		lua_pop(L, 2);
		throw LuaApiException( "Failed to retrieve class " + boost::core::demangle(typeid(BaseType).name()) + " from stack!");
	}

	template<typename BaseType, typename BaseConstType>
	void getCUData(int position, BaseConstType & value)
	{
		if (lua_isnil(L, position))
		{
			value = nullptr;
			return;
		}

		static const auto KEY = api::Registry::get()->getTypeName<BaseType>();
		static const auto C_KEY = api::Registry::get()->getTypeName<BaseConstType>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
		{
			std::string expectedType = boost::core::demangle(typeid(BaseType).name());
			const char * actualType = lua_typename(L, lua_type(L, position));
			std::string message = std::string("Invalid Lua value! Expected ") + expectedType + " at position " + std::to_string(position) + ", but found " + actualType;
			throw LuaApiException( message );
		}

		if(lua_getmetatable(L, position) == 0)
			throw LuaApiException( "Invalid Lua value! Found userdata without assigned metatable!");

		//top is metatable

		lua_getfield(L, LUA_REGISTRYINDEX, KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<BaseType *>(raw));
			lua_pop(L, 2);
			return;
		}

		lua_pop(L, 1);

		//top is metatable

		lua_getfield(L, LUA_REGISTRYINDEX, C_KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<BaseConstType *>(raw));
			lua_pop(L, 2);
			return;
		}

		lua_pop(L, 2);
		throw LuaApiException( "Failed to retrieve class " + boost::core::demangle(typeid(BaseType).name()) + " from stack!");
	}

	/// Pushes copy of FinalType on top of Lua stack as userdata
	template<typename DataType, typename FinalType>
	void createLuaUserdata(const FinalType & copySource)
	{
		void * raw = lua_newuserdata(L, sizeof(DataType));
		if(!raw)
			throw LuaApiException("Failed to allocate new user data!");

		new(raw) DataType(copySource);
	}

	lua_State * L;
	int initialTop;
};

}

VCMI_LIB_NAMESPACE_END
