/*
 * UnitProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "UnitProxy.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace battle
{

VCMI_REGISTER_SCRIPT_API(UnitProxy, "battle.Unit")

const std::vector<UnitProxy::RegType> UnitProxy::REGISTER =
{
	{
		"getMinDamage",
		LuaCallWrapper<const IBonusBearer>::createFunctor(&IBonusBearer::getMinDamage)
	},
	{
		"getMaxDamage",
		LuaCallWrapper<const IBonusBearer>::createFunctor(&IBonusBearer::getMaxDamage)
	},
	{
		"getAttack",
		LuaCallWrapper<const IBonusBearer>::createFunctor(&IBonusBearer::getAttack)
	},
	{
		"getDefence",
		LuaCallWrapper<const IBonusBearer>::createFunctor(&IBonusBearer::getDefence)
	},
	{
		"isAlive",
		LuaCallWrapper<const Unit>::createFunctor(&Unit::alive)
	},
	{
		"unitId",
		LuaCallWrapper<const IUnitInfo>::createFunctor(&IUnitInfo::unitId)
	}
};


}
}
}
