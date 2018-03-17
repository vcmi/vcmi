/*
 * EventBusTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/events/Event.h>
#include <vcmi/events/EventBus.h>

namespace test
{
using namespace ::testing;
using namespace ::events;

class EventExample : public Event
{
public:
	using PreHandler = SubscriptionRegistry<EventExample>::PreHandler;
	using PostHandler = SubscriptionRegistry<EventExample>::PostHandler;
	using BusTag = SubscriptionRegistry<EventExample>::BusTag;

public:
	MOCK_METHOD0(executeStub, void());

	virtual void execute() override
	{
		executeStub();
	}

	static SubscriptionRegistry<EventExample> * getRegistry()
	{
		static std::unique_ptr<SubscriptionRegistry<EventExample>> Instance = make_unique<SubscriptionRegistry<EventExample>>();
		return Instance.get();
	}

	friend class SubscriptionRegistry<EventExample>;
};

class ListenerMock
{
public:
	MOCK_METHOD1(beforeEvent, void(EventExample &));
	MOCK_METHOD1(afterEvent, void(const EventExample &));
};

class EventBusTest : public Test
{
public:
	EventExample event1;
	EventExample event2;
	EventBus subject1;
	EventBus subject2;
};

TEST_F(EventBusTest, ExecuteNoListeners)
{
	EXPECT_CALL(event1, executeStub()).Times(1);
	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, ExecuteIgnoredSubscription)
{
	StrictMock<ListenerMock> listener;

	subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener, _1));
	subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener, _1));

	EXPECT_CALL(listener, beforeEvent(_)).Times(0);
	EXPECT_CALL(event1, executeStub()).Times(1);
	EXPECT_CALL(listener, afterEvent(_)).Times(0);

	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, ExecuteSequence)
{
	StrictMock<ListenerMock> listener1;
	StrictMock<ListenerMock> listener2;

	auto subscription1 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener1, _1));
	auto subscription2 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener1, _1));
	auto subscription3 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener2, _1));
	auto subscription4 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener2, _1));

	{
		InSequence sequence;
		EXPECT_CALL(listener1, beforeEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener2, beforeEvent(Ref(event1))).Times(1);
		EXPECT_CALL(event1, executeStub()).Times(1);
		EXPECT_CALL(listener1, afterEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener2, afterEvent(Ref(event1))).Times(1);
	}

	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, BusesAreIndependent)
{
	StrictMock<ListenerMock> listener1;
	StrictMock<ListenerMock> listener2;

	auto subscription1 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener1, _1));
	auto subscription2 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener1, _1));
	auto subscription3 = subject2.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener2, _1));
	auto subscription4 = subject2.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener2, _1));

	EXPECT_CALL(listener1, beforeEvent(_)).Times(1);
	EXPECT_CALL(listener2, beforeEvent(_)).Times(0);
	EXPECT_CALL(event1, executeStub()).Times(1);
	EXPECT_CALL(listener1, afterEvent(_)).Times(1);
	EXPECT_CALL(listener2, afterEvent(_)).Times(0);

	subject1.executeEvent(event1);
}

//TODO: test sending event to destroyed event bus

}
