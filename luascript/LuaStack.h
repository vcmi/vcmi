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


VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class int3;

namespace scripting
{

class LuaApiException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class LuaStack;

class LuaStack
{
public:
	LuaStack(lua_State * L_);
	void balance();
	void clear();
	void debugPrintStack();

	void pushByIndex(lua_Integer index);

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

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSerializable, T>, int> = 0>
	void push(const T & value)
	{
		lua_newtable(L);
		int tableIndex = lua_gettop(L);

		// get non-const value - ugly, but required since same template method is used for deserialization
		T & nonConstValue = const_cast<T&>(value);

		const auto & luaSerializer = [this, tableIndex]<typename Field>(const std::string &keyName, const Field & data)
		{
			push(data);
			lua_setfield(L, tableIndex, keyName.c_str());
		};

		nonConstValue.serializeScript(luaSerializer);
	}

	bool tryGet(int position, bool & value);

	template<typename T, typename std::enable_if_t< std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
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

	template<typename T, typename std::enable_if_t<std::is_base_of_v<IdentifierBase, T>, int> = 0>
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

	template<typename T, typename std::enable_if_t< std::is_enum_v<T>, int> = 0>
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

	template<typename T>
	bool tryGet(int idx, std::vector<T> & out)
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
			tryGet(-1, out[i]);
			lua_pop(L, 1);
		}

		return true;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSerializable, T>, int> = 0>
	inline bool tryGet(int position, T & value)
	{
		const auto & deserializer = [this, position]<typename Data>(const std::string &keyName, Data & data)
		{
			if (!lua_istable(L, position))
				throw LuaApiException("value at index is not a table");

			// pushes table[keyName] on stack
			// NOTE: if value is not present or set to nil, top of stack will contain nil
			// do not handle it here, but in tryGet - attempt to tryGet values that don't support nil values
			// will throw exceptions, while allowing null values (where supported) to load
			lua_getfield(L, position, keyName.c_str());

			try {
				tryGet(absindex(-1), data);
			} catch (...) {
				// restore stack
				lua_pop(L, 1);
				throw;
			}

			lua_pop(L, 1); // pop pushed value
		};

		value.serializeScript(deserializer);
		return true;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagRawPointer, T> && std::is_const_v<T>, int> = 0>
	inline bool tryGet(int position, T * & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = BaseType *;
		using ConstPtrType = const BaseType *;

		ConstPtrType basePtr;
		bool result = tryGetCUData<BasePtrType, ConstPtrType>(position, basePtr);
		value = dynamic_cast<T*>(basePtr);
		return result;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagRawPointer, T> && !std::is_const_v<T>, int> = 0>
	inline bool tryGet(int position, T * & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = BaseType*;

		BasePtrType basePtr;
		bool result = tryGetUData<BasePtrType>(position, basePtr);
		value = dynamic_cast<T*>(basePtr);
		return result;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSharedPointer, T> && std::is_const_v<T>, int> = 0>
	inline bool tryGet(int position, std::shared_ptr<T> & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = std::shared_ptr<BaseType>;
		using ConstPtrType = std::shared_ptr<const BaseType>;

		ConstPtrType basePtr;
		bool result = tryGetCUData<BasePtrType, ConstPtrType>(position, basePtr);
		value = std::dynamic_pointer_cast<T>(basePtr);
		return result;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagSharedPointer, T> && !std::is_const_v<T>, int> = 0>
	inline bool tryGet(int position, std::shared_ptr<T> & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		using BasePtrType = std::shared_ptr<BaseType>;

		BasePtrType basePtr;
		bool result = tryGetUData<BasePtrType>(position, basePtr);
		value = std::dynamic_pointer_cast<T>(basePtr);
		return result;
	}

	template<typename T, typename std::enable_if_t<std::is_base_of_v<scripting::TagCopyable, T>, int> = 0>
	inline bool tryGet(int position, T & value)
	{
		using DataType = T;
		using BaseType = typename DataType::ScriptingApiName;
		static_assert(std::is_same_v<DataType, BaseType>, "Can not push derived class as copyable!");

		return tryGetUData<BaseType>(position, value);
	}

	template<typename... Args>
	bool tryGetAll(int position, Args &...args) {
		bool failed = false;
		int i = 0;

		((tryGet(position + i, args) ? ++i : (failed = true, ++i, false)), ...);

		return !failed;
	}

	template<typename T>
	void getOrThrow(int position, T & value)
	{
		bool result = tryGet(position, value);
		if (!result)
		{
			const char * expectedType = typeid(T).name();
			const char * actualType = lua_typename(L, position);
			std::string message	= std::string("Invalid Lua value! Expected ") + expectedType + " at position" + std::to_string(position) + ", but found " + actualType;
			throw LuaApiException( message );
		}
	}

	template<typename T>
	void getNonNullOrThrow(int position, T & value)
	{
		getOrThrow(position, value);
		if (value == nullptr)
		{
			const char * expectedType = typeid(T).name();
			std::string message	= std::string("Invalid Lua value! Expected ") + expectedType + " at position" + std::to_string(position) + ", but found nil value!";
			throw LuaApiException( message );
		}
	}

	bool tryGet(int position, JsonNode & value);

	int retVoid();

	inline int retPushed()
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
	bool tryGetInteger(int position, lua_Integer & value);

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
	bool tryGetUData(int position, BaseType & value)
	{
		if constexpr (std::is_assignable_v<std::remove_cvref_t<BaseType>&, std::nullptr_t>)
		{
			if (lua_isnil(L, position))
			{
				value = nullptr;
				return true;
			}
		}

		static const auto KEY = api::Registry::get()->getTypeName<BaseType>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
		{
			const char * expectedType = typeid(BaseType).name();
			const char * actualType = lua_typename(L, position);
			std::string message	= std::string("Invalid Lua value! Expected ") + expectedType + "at position" + std::to_string(position) + ", but found " + actualType;
			throw LuaApiException( message );

		}

		if(lua_getmetatable(L, position) == 0)
			throw LuaApiException( "Invalid Lua value! Found userdata without assigned metatable!");

		lua_getfield(L, LUA_REGISTRYINDEX, KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<BaseType *>(raw));
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
		throw LuaApiException( "Failed to retrieve class " + std::string(typeid(BaseType).name()) + " from stack!");
	}

	template<typename BaseType, typename BaseConstType>
	bool tryGetCUData(int position, BaseConstType & value)
	{
		if (lua_isnil(L, position))
		{
			value = nullptr;
			return true;
		}

		static const auto KEY = api::Registry::get()->getTypeName<BaseType>();
		static const auto C_KEY = api::Registry::get()->getTypeName<BaseConstType>();

		void * raw = lua_touserdata(L, position);

		if(!raw)
		{
			const char * expectedType = typeid(BaseType).name();
			const char * actualType = lua_typename(L, position);
			std::string message	= std::string("Invalid Lua value! Expected ") + expectedType + "at position" + std::to_string(position) + ", but found " + actualType;
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
			return true;
		}

		lua_pop(L, 1);

		//top is metatable

		lua_getfield(L, LUA_REGISTRYINDEX, C_KEY);

		if(lua_rawequal(L, -1, -2) == 1)
		{
			value = *(static_cast<BaseConstType *>(raw));
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
		throw LuaApiException( "Failed to retrieve class " + std::string(typeid(BaseType).name()) + " from stack!");
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
