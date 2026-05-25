/*
 * BonusSystem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class BonusList;
class IBonusBearer;

namespace scripting
{
namespace api
{

#if 0
class BonusProxy : public SharedPointerWrapper<const Bonus, BonusProxy>
{
public:
	using Wrapper = SharedPointerWrapper<const Bonus, BonusProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getType(lua_State * L);
	static int getSubtype(lua_State * L);
	static int getDuration(lua_State * L);
	static int getTurns(lua_State * L);
	static int getValueType(lua_State * L);
	static int getVal(lua_State * L);
	static int getSource(lua_State * L);
	static int getSourceID(lua_State * L);
	static int getDescription(lua_State * L);
	static int getEffectRange(lua_State * L);
	static int getStacking(lua_State * L);
	static int toJsonNode(lua_State * L);

protected:
	void adjustStaticTable(lua_State * L) const override;
};

class BonusListProxy : public SharedPointerWrapper<const BonusList, BonusListProxy>
{
public:
	using Wrapper = SharedPointerWrapper<const BonusList, BonusListProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

class BonusBearerProxy : public RawPointerWrapper<const IBonusBearer, BonusBearerProxy>
{
public:
	using Wrapper = RawPointerWrapper<const IBonusBearer, BonusBearerProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getBonuses(lua_State * L);
};

#endif

}
}

VCMI_LIB_NAMESPACE_END
