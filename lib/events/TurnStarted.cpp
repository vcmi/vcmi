/*
 * TurnStarted.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TurnStarted.h"

#include <vcmi/events/EventBus.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

SubscriptionRegistry<TurnStarted> * TurnStarted::getRegistry()
{
	static std::unique_ptr<SubscriptionRegistry<TurnStarted>> Instance = make_unique<SubscriptionRegistry<TurnStarted>>();
	return Instance.get();
}

void TurnStarted::defaultExecute(const EventBus * bus)
{
	CTurnStarted event;
	bus->executeEvent(event);
}

CTurnStarted::CTurnStarted() = default;

bool CTurnStarted::isEnabled() const
{
	return true;
}


}

VCMI_LIB_NAMESPACE_END
