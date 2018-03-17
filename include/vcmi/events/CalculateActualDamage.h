/*
 * CalculateActualDamage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Event.h"
#include "SubscriptionRegistry.h"

namespace events
{

class DLL_LINKAGE CalculateActualDamage : public Event
{
public:
	using PreHandler = SubscriptionRegistry<CalculateActualDamage>::PreHandler;
	using PostHandler = SubscriptionRegistry<CalculateActualDamage>::PostHandler;
	using BusTag = SubscriptionRegistry<CalculateActualDamage>::BusTag;

	static std::unique_ptr<CalculateActualDamage> create();

	friend class SubscriptionRegistry<CalculateActualDamage>;
};

extern template class SubscriptionRegistry<CalculateActualDamage>;

}



