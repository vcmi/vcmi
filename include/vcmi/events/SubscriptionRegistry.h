/*
 * SubscriptionRegistry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class Environment;

namespace events
{

class EventBus;

class DLL_LINKAGE EventSubscription : public boost::noncopyable
{
public:
	virtual ~EventSubscription() = default;
};

template <typename E>
class SubscriptionRegistry : public boost::noncopyable
{
public:
	using PreHandler = std::function<void(E &)>;
	using ExecHandler = std::function<void(E &)>;
	using PostHandler = std::function<void(const E &)>;
	using BusTag = const void *;

	std::unique_ptr<EventSubscription> subscribeBefore(BusTag tag, PreHandler && handler)
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);

		auto storage = std::make_shared<PreHandlerStorage>(std::move(handler));
		preHandlers[tag].push_back(storage);
		return std::make_unique<PreSubscription>(tag, storage);
	}

	std::unique_ptr<EventSubscription> subscribeAfter(BusTag tag, PostHandler && handler)
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);

		auto storage = std::make_shared<PostHandlerStorage>(std::move(handler));
		postHandlers[tag].push_back(storage);
		return std::make_unique<PostSubscription>(tag, storage);
	}

	void executeEvent(const EventBus * bus, E & event, const ExecHandler & execHandler)
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		{
			auto it = preHandlers.find(bus);

			if(it != std::end(preHandlers))
			{
				for(auto & h : it->second)
					(*h)(event);
			}
		}

		if(event.isEnabled())
		{
			if(execHandler)
				execHandler(event);

			auto it = postHandlers.find(bus);

			if(it != std::end(postHandlers))
			{
				for(auto & h : it->second)
					(*h)(event);
			}
		}
	}

private:

	template <typename T>
	class HandlerStorage
	{
	public:
		explicit HandlerStorage(T && handler_)
			: handler(handler_)
		{
		}

		STRONG_INLINE
		void operator()(E & event)
		{
			handler(event);
		}
	private:
		T handler;
	};

	using PreHandlerStorage = HandlerStorage<PreHandler>;
	using PostHandlerStorage = HandlerStorage<PostHandler>;

	class PreSubscription : public EventSubscription
	{
	public:
		PreSubscription(BusTag tag_, std::shared_ptr<PreHandlerStorage> handler_)
			: handler(handler_),
			tag(tag_)
		{
		}

		virtual ~PreSubscription()
		{
			auto registry = E::getRegistry();
			registry->unsubscribe(tag, handler, registry->preHandlers);
		}
	private:
		BusTag tag;
		std::shared_ptr<PreHandlerStorage> handler;
	};

	class PostSubscription : public EventSubscription
	{
	public:
		PostSubscription(BusTag tag_, std::shared_ptr<PostHandlerStorage> handler_)
			: handler(handler_),
			tag(tag_)
		{
		}

		virtual ~PostSubscription()
		{
			auto registry = E::getRegistry();
			registry->unsubscribe(tag, handler, registry->postHandlers);
		}
	private:
		BusTag tag;
		std::shared_ptr<PostHandlerStorage> handler;
	};

	boost::shared_mutex mutex;

	std::map<BusTag, std::vector<std::shared_ptr<PreHandlerStorage>>> preHandlers;
	std::map<BusTag, std::vector<std::shared_ptr<PostHandlerStorage>>> postHandlers;

	template <typename T>
	void unsubscribe(BusTag tag, T what, std::map<BusTag, std::vector<T>> & from)
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);

		auto it = from.find(tag);

		if(it != std::end(from))
		{
			it->second -= what;

			if(it->second.empty())
			{
				from.erase(tag);
			}
		}

	}
};
}

VCMI_LIB_NAMESPACE_END
