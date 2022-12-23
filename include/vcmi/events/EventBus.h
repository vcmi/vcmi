/*
 * EventBus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "SubscriptionRegistry.h"

VCMI_LIB_NAMESPACE_BEGIN

class Environment;

namespace events
{

class DLL_LINKAGE EventBus : public boost::noncopyable
{
public:
	template <typename E>
	std::unique_ptr<EventSubscription> subscribeBefore(typename E::PreHandler && cb)
	{
		auto registry = E::getRegistry();
		return registry->subscribeBefore(this, std::move(cb));
	}

	template <typename E>
	std::unique_ptr<EventSubscription> subscribeAfter(typename E::PostHandler && cb)
	{
		auto registry = E::getRegistry();
		return registry->subscribeAfter(this, std::move(cb));
	}

	template <typename E>
	void executeEvent(E & event, const typename E::ExecHandler & execHandler = typename E::ExecHandler()) const
	{
		auto registry = E::getRegistry();
		registry->executeEvent(this, event, execHandler);
	}
};
}

VCMI_LIB_NAMESPACE_END
