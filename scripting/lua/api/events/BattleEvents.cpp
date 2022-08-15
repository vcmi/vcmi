/*
 * BattleEvents.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleEvents.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"
#include "../../../../lib/battle/Unit.h"
#include "SubscriptionRegistryProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace events
{
using ::events::ApplyDamage;

VCMI_REGISTER_SCRIPT_API(ApplyDamageProxy, "events.ApplyDamage");

const std::vector<ApplyDamageProxy::CustomRegType> ApplyDamageProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&SubscriptionRegistryProxy<ApplyDamageProxy>::subscribeBefore,
		true
	},
	{
		"subscribeAfter",
		&SubscriptionRegistryProxy<ApplyDamageProxy>::subscribeAfter,
		true
	},
	{
		"getInitalDamage",
		LuaMethodWrapper<ApplyDamage, decltype(&ApplyDamage::getInitalDamage), &ApplyDamage::getInitalDamage>::invoke,
		false
	},
	{
		"getDamage",
		LuaMethodWrapper<ApplyDamage, decltype(&ApplyDamage::getDamage), &ApplyDamage::getDamage>::invoke,
		false
	},
	{
		"setDamage",
		LuaMethodWrapper<ApplyDamage, decltype(&ApplyDamage::setDamage), &ApplyDamage::setDamage>::invoke,
		false
	},
	{
		"getTarget",
		LuaMethodWrapper<ApplyDamage, decltype(&ApplyDamage::getTarget), &ApplyDamage::getTarget>::invoke,
		false
	},

};

}
}
}


VCMI_LIB_NAMESPACE_END
