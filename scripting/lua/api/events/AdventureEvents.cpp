/*
 * AdventureEvents.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AdventureEvents.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"
#include "SubscriptionRegistryProxy.h"

namespace scripting
{
namespace api
{
namespace events
{

VCMI_REGISTER_SCRIPT_API(ObjectVisitStartedProxy, "events.ObjectVisitStarted");

const std::vector<ObjectVisitStartedProxy::RegType> ObjectVisitStartedProxy::REGISTER = {};

const std::vector<ObjectVisitStartedProxy::CustomRegType> ObjectVisitStartedProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&SubscriptionRegistryProxy<ObjectVisitStartedProxy>::subscribeBefore,
		true
	},
	{
		"subscribeAfter",
		&SubscriptionRegistryProxy<ObjectVisitStartedProxy>::subscribeAfter,
		true
	},
};

}
}
}

