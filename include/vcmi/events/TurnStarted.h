/*
 * TurnStarted.h, part of VCMI engine
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

class DLL_LINKAGE TurnStarted : public Event
{
public:
	using Sub = SubscriptionRegistry<TurnStarted>;
	using PreHandler = Sub::PreHandler;
	using PostHandler = Sub::PostHandler;
	using ExecHandler = Sub::ExecHandler;

	static Sub * getRegistry();
	static void defaultExecute(const EventBus * bus);

	friend class SubscriptionRegistry<TurnStarted>;
};

}
