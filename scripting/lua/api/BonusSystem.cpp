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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_SCRIPT_API(BonusProxy, "Bonus");

const std::vector<BonusProxy::CustomRegType> BonusProxy::REGISTER_CUSTOM =
{
	{"getType", &BonusProxy::getType, false},
	{"getSubtype", &BonusProxy::getSubtype, false},
	{"getDuration", &BonusProxy::getDuration, false},
	{"getTurns", &BonusProxy::getTurns, false},
	{"getValueType", &BonusProxy::getValueType, false},
	{"getVal", &BonusProxy::getVal, false},
	{"getSource", &BonusProxy::getSource, false},
	{"getSourceID", &BonusProxy::getSourceID, false},
	{"getEffectRange", &BonusProxy::getEffectRange, false},
	{"getStacking", &BonusProxy::getStacking, false},
	{"getDescription", &BonusProxy::getDescription, false},
	{"toJsonNode", &BonusProxy::toJsonNode, false}
};

int BonusProxy::getType(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->type);
}

int BonusProxy::getSubtype(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->subtype);
}

int BonusProxy::getDuration(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->duration);
}

int BonusProxy::getTurns(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->turnsRemain);
}

int BonusProxy::getValueType(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->valType);
}

int BonusProxy::getVal(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->val);
}

int BonusProxy::getSource(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->source);
}

int BonusProxy::getSourceID(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->sid);
}

int BonusProxy::getEffectRange(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetInt(L, object->effectRange);
}

int BonusProxy::getStacking(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetStr(L, object->stacking);
}

int BonusProxy::getDescription(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
	return LuaStack::quickRetStr(L, object->description);
}

int BonusProxy::toJsonNode(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<const Bonus> object;
	if(!S.tryGet(1, object))
		return S.retNil();
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
		auto id = static_cast<int32_t>(p.second);

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

const std::vector<BonusListProxy::CustomRegType> BonusListProxy::REGISTER_CUSTOM =
{

};

std::shared_ptr<const Bonus> BonusListProxy::index(std::shared_ptr<const BonusList> self, int key)
{
	//field = __index(self, key)

	std::shared_ptr<const Bonus> ret;

	if((key >= 1) && (key <= self->size()))
		ret = (*self)[key-1];
	return ret;
}

void BonusListProxy::adjustMetatable(lua_State * L) const
{
	lua_pushstring(L, "__index");
	lua_pushcclosure(L, LuaFunctionWrapper<decltype(&BonusListProxy::index), &BonusListProxy::index>::invoke, 0);
	lua_rawset(L, -3);
}

VCMI_REGISTER_SCRIPT_API(BonusBearerProxy, "BonusBearer");

const std::vector<BonusBearerProxy::CustomRegType> BonusBearerProxy::REGISTER_CUSTOM =
{
	{"getBonuses", &BonusBearerProxy::getBonuses, false},
};

int BonusBearerProxy::getBonuses(lua_State * L)
{
	LuaStack S(L);

	const IBonusBearer * object = nullptr;
	if(!S.tryGet(1, object))
		return S.retNil();

	TConstBonusListPtr ret;

	const bool hasSelector = S.isFunction(2);
	const bool hasRangeSelector = S.isFunction(3);

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


VCMI_LIB_NAMESPACE_END
