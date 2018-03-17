/*
 * BonusSystem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BonusSystem.h"

#include "../../../lib/HeroBonus.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_SCRIPT_API(BonusProxy, "Bonus");

const std::vector<BonusProxy::RegType> BonusProxy::REGISTER =
{
	{"getType", &BonusProxy::getType},
	{"getSubtype", &BonusProxy::getSubtype},
	{"getDuration", &BonusProxy::getDuration},
	{"getTurns", &BonusProxy::getTurns},
	{"getValueType", &BonusProxy::getValueType},
	{"getVal", &BonusProxy::getVal},
	{"getSource", &BonusProxy::getSource},
	{"getSourceID", &BonusProxy::getSourceID},
	{"getEffectRange", &BonusProxy::getEffectRange},
	{"getStacking", &BonusProxy::getStacking},
	{"getDescription", &BonusProxy::getDescription},
	{"toJsonNode", &BonusProxy::toJsonNode}
};

const std::vector<BonusProxy::CustomRegType> BonusProxy::REGISTER_CUSTOM =
{

};

int BonusProxy::getType(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->type);
}

int BonusProxy::getSubtype(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->subtype);
}

int BonusProxy::getDuration(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->duration);
}

int BonusProxy::getTurns(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->turnsRemain);
}

int BonusProxy::getValueType(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->valType);
}

int BonusProxy::getVal(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->val);
}

int BonusProxy::getSource(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->source);
}

int BonusProxy::getSourceID(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->sid);
}

int BonusProxy::getEffectRange(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetInt(L, object->effectRange);
}

int BonusProxy::getStacking(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetStr(L, object->stacking);
}

int BonusProxy::getDescription(lua_State * L, std::shared_ptr<const Bonus> object)
{
	return LuaStack::quickRetStr(L, object->description);
}

int BonusProxy::toJsonNode(lua_State * L, std::shared_ptr<const Bonus> object)
{
	LuaStack S(L);
	S.clear();
	S.push(object->toJsonNode());
	return 1;
}

template <typename T>
static void publishMap(lua_State * L, const T & map)
{
	for(auto & p : map)
	{
		const std::string & name = p.first;
		int32_t id = static_cast<int32_t>(p.second);

		lua_pushstring(L, name.c_str());
		lua_pushinteger(L, id);
		lua_rawset(L, -3);
	}
}

void BonusProxy::adjustStaticTable(lua_State * L) const
{
	publishMap(L, bonusNameMap);
	publishMap(L, bonusValueMap);
	publishMap(L, bonusSourceMap);
	publishMap(L, bonusDurationMap);
}

VCMI_REGISTER_SCRIPT_API(BonusListProxy, "BonusList");

const std::vector<BonusListProxy::RegType> BonusListProxy::REGISTER =
{

};

const std::vector<BonusListProxy::CustomRegType> BonusListProxy::REGISTER_CUSTOM =
{

};

int BonusListProxy::index(lua_State * L)
{
	//field = __index(self, key)
	LuaStack S(L);

	std::shared_ptr<const BonusList> self;
	lua_Integer key = -1;

	if(S.tryGet(1, self) && S.tryGetInteger(2, key))
	{
		if((key >= 1) && (key <= self->size()))
		{
			std::shared_ptr<const Bonus> ret = (*self)[key-1];

			S.clear();
			S.push(ret);
			return 1;
		}
	}

	return S.retNil();
}

void BonusListProxy::adjustMetatable(lua_State * L) const
{
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, &BonusListProxy::index);
	lua_rawset(L, -3);
}


VCMI_REGISTER_SCRIPT_API(BonusBearerProxy, "BonusBearer");

const std::vector<BonusBearerProxy::RegType> BonusBearerProxy::REGISTER =
{
	{"getBonuses", &BonusBearerProxy::getBonuses},
};

const std::vector<BonusBearerProxy::CustomRegType> BonusBearerProxy::REGISTER_CUSTOM =
{

};

int BonusBearerProxy::getBonuses(lua_State * L, const IBonusBearer * object)
{
	LuaStack S(L);

	TConstBonusListPtr ret;

	const bool hasSelector = S.isFunction(1);
	const bool hasRangeSelector = S.isFunction(2);

	if(hasSelector)
	{
		auto selector = [](const Bonus * b)
		{
			return false; //TODO: BonusBearerProxy::getBonuses selector
		};

		if(hasRangeSelector)
		{
			auto rangeSelector = [](const Bonus * b)
			{
				return false;//TODO: BonusBearerProxy::getBonuses rangeSelector
			};

			ret = object->getBonuses(selector, rangeSelector);
		}
		else
		{
			ret = object->getBonuses(selector, Selector::all);
		}
	}
	else
	{
		ret = object->getBonuses(Selector::all, Selector::all);
	}

	S.clear();
	S.push(ret);
	return S.retPushed();
}


}
}

