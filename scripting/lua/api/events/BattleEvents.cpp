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

namespace scripting
{
namespace api
{
namespace events
{
using ::events::ApplyDamage;

VCMI_REGISTER_SCRIPT_API(ApplyDamageProxy, "events.ApplyDamage");

const std::vector<ApplyDamageProxy::RegType> ApplyDamageProxy::REGISTER =
{
	{
		"getInitalDamage",
		LuaCallWrapper<ApplyDamage>::createFunctor(&ApplyDamage::getInitalDamage)
	},
	{
		"getDamage",
		LuaCallWrapper<ApplyDamage>::createFunctor(&ApplyDamage::getDamage)
	},
	{
		"setDamage",
		LuaCallWrapper<ApplyDamage>::createFunctor(&ApplyDamage::setDamage)
	},
	{
		"getTarget",
		LuaCallWrapper<ApplyDamage>::createFunctor(&ApplyDamage::getTarget)
	}
};

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
	}
};

}
}
}

