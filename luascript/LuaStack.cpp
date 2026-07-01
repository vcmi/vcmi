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

#include "../lib/json/JsonNode.h"
#include "../lib/int3.h"
#include "../lib/battle/BattleHex.h"
#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/GameLibrary.h"

#include <vcmi/Creature.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

LuaStack::LuaStack(lua_State * L_):
	L(L_),
	initialTop(lua_gettop(L))
{
}

void LuaStack::restoreInitialTop()
{
	lua_settop(L, initialTop);
}

void LuaStack::clear()
{
	lua_settop(L, 0);
}

void LuaStack::debugPrintStack()
{
	int top = lua_gettop(L);
	logScript->error("Lua stack (top=%d):\n", top);
	for (int i = 1; i <= top; ++i) {
		int t = lua_type(L, i);
		const char *tn = lua_typename(L, t);
		switch (t) {
			case LUA_TSTRING:
				logScript->error("  %d: %s = '%s'", i, tn, lua_tostring(L, i));
				break;
			case LUA_TNUMBER:
				logScript->error("  %d: %s = %g", i, tn, lua_tonumber(L, i));
				break;
			case LUA_TBOOLEAN:
				logScript->error("  %d: %s = %s", i, tn, lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNIL:
				logScript->error("  %d: %s = (nil)", i, tn);
				break;
			default:
				logScript->error("  %d: %s (%p)", i, tn, lua_topointer(L, i));
				break;
		}
	}
}

int LuaStack::absindex(int idx)
{
	if (idx > 0)
		return idx;

	return lua_gettop(L) + 1 + idx;
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
			for(const auto & keyValue : value.Struct())
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

void LuaStack::get(int position, bool & value)
{
	if(!lua_isboolean(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected boolean at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = (lua_toboolean(L, position) != 0);
}

void LuaStack::get(int position, double & value)
{
	if(!lua_isnumber(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected number at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = lua_tonumber(L, position);
}

void LuaStack::getInteger(int position, lua_Integer & value)
{
	if(!lua_isnumber(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected integer at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = lua_tointeger(L, position);
}

void LuaStack::get(int position, std::string & value)
{
	if(!lua_isstring(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected string at position ") + std::to_string(position) + ", but found " + actualType);
	}

	size_t len = 0;
	const auto *raw = lua_tolstring(L, position, &len);
	value = std::string(raw, len);
}

void LuaStack::get(int position, int3 & value)
{
	get(position,   value.x);
	get(position+1, value.y);
	get(position+2, value.z);
}

void LuaStack::get(int position, BattleHex & value)
{
	if(lua_isnumber(L, position))
	{
		lua_Integer temp;
		getInteger(position, temp);
		value = BattleHex(static_cast<si16>(temp));
	}
	else
	{
		getUData<BattleHex>(position, value);
	}
}

void LuaStack::push(const CreatureID & value)
{
	push(value.toEntity(LIBRARY));
}

void LuaStack::get(int position, CreatureID & value)
{
	const Creature * creature = nullptr;
	get(position, creature);
	value = creature ? creature->getId() : CreatureID(CreatureID::NONE);
}

void LuaStack::get(int position, JsonNode & value)
{
	auto type = lua_type(L, position);
	value.setModScope("game");

	switch(type)
	{
	case LUA_TNIL:
		value.clear();
		return;
	case LUA_TNUMBER:
		get(position, value.Float());
		return;
	case LUA_TBOOLEAN:
		value.Bool() = (lua_toboolean(L, position) != 0);
		return;
	case LUA_TSTRING:
		get(position, value.String());
		return;
	case LUA_TTABLE:
		{
			JsonNode asVector;
			JsonNode asStruct;

			lua_pushnil(L);  /* first key */

			while(lua_next(L, position) != 0)
			{
				/* 'key' (at index -2) and 'value' (at index -1) */

				JsonNode fieldValue;
				try
				{
					get(lua_gettop(L), fieldValue);
				}
				catch(...)
				{
					lua_pop(L, 2);
					value.clear();
					throw;
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
					get(-1, key);
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
		return;
	default:
		{
			const char * actualType = lua_typename(L, type);
			throw LuaApiException(std::string("Unsupported Lua type for JsonNode: ") + actualType);
		}
	}
}

int LuaStack::retVoid()
{
	clear();
	return 0;
}


}

VCMI_LIB_NAMESPACE_END
