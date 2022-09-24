/*
 * SubscriptionRegistryProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/Event.h>
#include <vcmi/events/EventBus.h>
#include <vcmi/events/SubscriptionRegistry.h>

#include "../../LuaWrapper.h"
#include "../../LuaStack.h"
#include "../../LuaReference.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace events
{


class EventSubscriptionProxy : public UniqueOpaqueWrapper<::events::EventSubscription, EventSubscriptionProxy>
{
public:
	using Wrapper = UniqueOpaqueWrapper<::events::EventSubscription, EventSubscriptionProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

template <typename EventProxy>
class SubscriptionRegistryProxy
{
public:
	using EventType = typename EventProxy::ObjectType;
	using RegistryType = ::events::SubscriptionRegistry<EventType>;

	static_assert(std::is_base_of<::events::Event, EventType>::value, "Invalid template parameter");

	static int subscribeBefore(lua_State * L)
	{
		LuaStack S(L);
		// subscription = subscribeBefore(eventBus, callback)

		//TODO: use capture by move from c++14
		auto callbackRef = std::make_shared<LuaReference>(L);

		::events::EventBus * eventBus = nullptr;

		if(!S.tryGet(1, eventBus))
		{
			S.push("No event bus");
			return 1;
		}

		S.clear();

		RegistryType * registry = EventType::getRegistry();

		typename EventType::PreHandler callback = [=](EventType & event)
		{
			LuaStack S(L);
			callbackRef->push();
			S.push(&event);

			if(lua_pcall(L, 1, 0, 0) != 0)
			{
				std::string msg;
				S.tryGet(1, msg);
				logMod->error("Script callback error: %s", msg);
			}

			S.clear();
		};

		std::unique_ptr<::events::EventSubscription> subscription = registry->subscribeBefore(eventBus, std::move(callback));
		S.push(std::move(subscription));
		return 1;
	}

	static int subscribeAfter(lua_State * L)
	{
		LuaStack S(L);

		//TODO: use capture by move from c++14
		auto callbackRef = std::make_shared<LuaReference>(L);

		::events::EventBus * eventBus = nullptr;

		if(!S.tryGet(1, eventBus))
		{
			S.push("No event bus");
			return 1;
		}

		S.clear();

		RegistryType * registry = EventType::getRegistry();

		typename EventType::PostHandler callback = [=](const EventType & event)
		{
			LuaStack S(L);
			callbackRef->push();
			S.push(const_cast<EventType *>(&event)); //FIXME:

			if(lua_pcall(L, 1, 0, 0) != 0)
			{
				std::string msg;
				S.tryGet(1, msg);
				logMod->error("Script callback error: %s", msg);
			}

			S.clear();
		};

		std::unique_ptr<::events::EventSubscription> subscription = registry->subscribeAfter(eventBus, std::move(callback));
		S.push(std::move(subscription));
		return 1;
	}
};


}
}
}

VCMI_LIB_NAMESPACE_END
