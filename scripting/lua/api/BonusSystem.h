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

class Bonus;
class BonusList;
class IBonusBearer;

namespace scripting
{
namespace api
{

class BonusProxy : public SharedWrapper<const Bonus, BonusProxy>
{
public:
	using Wrapper = SharedWrapper<const Bonus, BonusProxy>;

	static const std::vector<typename Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getType(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getSubtype(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getDuration(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getTurns(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getValueType(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getVal(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getSource(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getSourceID(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getDescription(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getEffectRange(lua_State * L, std::shared_ptr<const Bonus> object);
	static int getStacking(lua_State * L, std::shared_ptr<const Bonus> object);
	static int toJsonNode(lua_State * L, std::shared_ptr<const Bonus> object);

protected:
	virtual void adjustStaticTable(lua_State * L) const override;
};

class BonusListProxy : public SharedWrapper<const BonusList, BonusListProxy>
{
public:
	using Wrapper = SharedWrapper<const BonusList, BonusListProxy>;

	static const std::vector<typename Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int index(lua_State * L);
protected:
	virtual void adjustMetatable(lua_State * L) const override;
};

class BonusBearerProxy : public OpaqueWrapper<const IBonusBearer, BonusBearerProxy>
{
public:
	using Wrapper = OpaqueWrapper<const IBonusBearer, BonusBearerProxy>;

	static int getBonuses(lua_State * L, const IBonusBearer * object);

	static const std::vector<typename Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};


}
}
