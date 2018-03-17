/*
 * SubscriptionRegistryProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SubscriptionRegistryProxy.h"

#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace events
{
//No methods here, just an empty metatable for type safety.
VCMI_REGISTER_CORE_SCRIPT_API(EventSubscriptionProxy, "EventSubscription");
const std::vector<EventSubscriptionProxy::RegType> EventSubscriptionProxy::REGISTER = {};
const std::vector<EventSubscriptionProxy::CustomRegType> EventSubscriptionProxy::REGISTER_CUSTOM = {};
}
}
}
