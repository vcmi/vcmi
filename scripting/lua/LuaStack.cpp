/*
 * LuaStack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaStack.h"

#include "../../lib/JsonNode.h"
#include "../../lib/int3.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

LuaStack::LuaStack(lua_State * L_)
	: L(L_)
{
	initialTop = lua_gettop(L);
}

void LuaStack::balance()
{
	lua_settop(L, initialTop);
}

void LuaStack::clear()
{
	lua_settop(L, 0);
}

void LuaStack::pushByIndex(lua_Integer index)
{
	lua_pushvalue(L, index);
}

void LuaStack::pushNil()
{
	lua_pushnil(L);
}

void LuaStack::pushInteger(lua_Integer value)
{
	lua_pushinteger(L, value);
}

void LuaStack::push(bool value)
{
	lua_pushboolean(L, value);
}

void LuaStack::push(const char * value)
{
	lua_pushstring(L, value);
}

void LuaStack::push(const std::string & value)
{
	lua_pushlstring(L, value.c_str(), value.size());
}

void LuaStack::push(const int3 & value)
{
	push(value.x);
	push(value.y);
	push(value.z);
}

void LuaStack::push(const JsonNode & value)
{
	switch(value.getType())
	{
	case JsonNode::JsonType::DATA_BOOL:
		{
			push(value.Bool());
		}
		break;
	case JsonNode::JsonType::DATA_FLOAT:
		{
			lua_pushnumber(L, value.Float());
		}
		break;
	case JsonNode::JsonType::DATA_INTEGER:
		{
			pushInteger(value.Integer());
		}
		break;
	case JsonNode::JsonType::DATA_STRUCT:
		{
			lua_newtable(L);
			for(auto & keyValue : value.Struct())
			{
				push(keyValue.first);
				push(keyValue.second);
				lua_rawset(L, -3);
			}
		}
		break;
	case JsonNode::JsonType::DATA_STRING:
		push(value.String());
		break;
	case JsonNode::JsonType::DATA_VECTOR:
		{
			lua_newtable(L);
			for(int idx = 0; idx < value.Vector().size(); idx++)
			{
				pushInteger(idx + 1);
				push(value.Vector()[idx]);
				lua_rawset(L, -3);
			}
		}
		break;

	default:
		pushNil();
		break;
	}
}

bool LuaStack::tryGet(int position, bool & value)
{
	if(!lua_isboolean(L, position))
		return false;
	value = (lua_toboolean(L, position) != 0);
	return true;
}

bool LuaStack::tryGet(int position, double & value)
{
	if(!lua_isnumber(L, position))
		return false;
	value = lua_tonumber(L, position);
	return true;
}

bool LuaStack::tryGetInteger(int position, lua_Integer & value)
{
	if(!lua_isnumber(L, position))
		return false;

	value = lua_tointeger(L, position);
	return true;
}

bool LuaStack::tryGet(int position, std::string & value)
{
	if(!lua_isstring(L, position))
		return false;

	size_t len = 0;
	auto raw = lua_tolstring(L, position, &len);
	value = std::string(raw, len);

	return true;
}

bool LuaStack::tryGet(int position, int3 & value)
{
	return tryGet(position, value.x) && tryGet(position+1, value.y) && tryGet(position+2, value.z);
}

bool LuaStack::tryGet(int position, JsonNode & value)
{
	auto type = lua_type(L, position);

	switch(type)
	{
	case LUA_TNIL:
		value.clear();
		return true;
	case LUA_TNUMBER:
		return tryGet(position, value.Float());
	case LUA_TBOOLEAN:
		value.Bool() = (lua_toboolean(L, position) != 0);
		return true;
	case LUA_TSTRING:
		return tryGet(position, value.String());
	case LUA_TTABLE:
		{
			JsonNode asVector(JsonNode::JsonType::DATA_VECTOR);
			JsonNode asStruct(JsonNode::JsonType::DATA_STRUCT);

			lua_pushnil(L);  /* first key */

			while(lua_next(L, position) != 0)
			{
				/* 'key' (at index -2) and 'value' (at index -1) */

				JsonNode fieldValue;
				if(!tryGet(lua_gettop(L), fieldValue))
				{
					lua_pop(L, 2);
					value.clear();
					return false;
				}

				lua_pop(L, 1); //pop value

				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					auto key = lua_tointeger(L, -1);

					if(key > 0)
					{
						if(asVector.Vector().size() < key)
							asVector.Vector().resize(key);
						--key;
						asVector.Vector().at(key) = fieldValue;
					}
				}
				else if(lua_isstring(L, -1))
				{
					std::string key;
					tryGet(-1, key);
					asStruct[key] = fieldValue;
				}
			}

			if(!asVector.Vector().empty())
			{
				std::swap(value, asVector);
			}
			else
			{
				std::swap(value, asStruct);
			}
		}
		return true;
	default:
		value.clear();
		return false;
	}
}

int LuaStack::retNil()
{
	clear();
	pushNil();
	return 1;
}

int LuaStack::retVoid()
{
	clear();
	return 0;
}


}

VCMI_LIB_NAMESPACE_END
