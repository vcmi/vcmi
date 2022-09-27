/*
 * ObjectVisitEnded.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

class PlayerColor;
class ObjectInstanceID;

namespace events
{

class DLL_LINKAGE ObjectVisitEnded : public Event
{
public:
	using Sub = SubscriptionRegistry<ObjectVisitEnded>;
	using PreHandler = Sub::PreHandler;
	using PostHandler = Sub::PostHandler;
	using ExecHandler = Sub::ExecHandler;

	virtual PlayerColor getPlayer() const = 0;
	virtual ObjectInstanceID getHero() const = 0;

	static Sub * getRegistry();
	static void defaultExecute(const EventBus * bus, const ExecHandler & execHandler,
		const PlayerColor & player, const ObjectInstanceID & heroId);

	friend class SubscriptionRegistry<ObjectVisitEnded>;
};

}

VCMI_LIB_NAMESPACE_END
