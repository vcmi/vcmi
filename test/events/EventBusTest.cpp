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
	using ExecHandler = SubscriptionRegistry<EventExample>::ExecHandler;

	MOCK_CONST_METHOD0(isEnabled, bool());
public:
	static SubscriptionRegistry<EventExample> * getRegistry()
	{
		static std::unique_ptr<SubscriptionRegistry<EventExample>> Instance = std::make_unique<SubscriptionRegistry<EventExample>>();
		return Instance.get();
	}

	friend class SubscriptionRegistry<EventExample>;
};

class ListenerMock
{
public:
	MOCK_METHOD1(beforeEvent, void(EventExample &));
	MOCK_METHOD1(onEvent, void(EventExample &));
	MOCK_METHOD1(afterEvent, void(const EventExample &));
};

class EventBusTest : public Test
{
public:
	EventExample event1;
	EventExample event2;
	EventBus subject1;
	EventBus subject2;

	StrictMock<ListenerMock> listener;
	StrictMock<ListenerMock> listener1;
	StrictMock<ListenerMock> listener2;
};

TEST_F(EventBusTest, ExecuteNoListeners)
{
	EXPECT_CALL(event1, isEnabled()).WillRepeatedly(Return(true));
	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, ExecuteNoListenersWithHandler)
{
	EXPECT_CALL(event1, isEnabled()).WillRepeatedly(Return(true));
	EXPECT_CALL(listener, onEvent(Ref(event1))).Times(1);

	subject1.executeEvent(event1, std::bind(&ListenerMock::onEvent, &listener, _1));
}

TEST_F(EventBusTest, ExecuteIgnoredSubscription)
{
	EXPECT_CALL(event1, isEnabled()).WillRepeatedly(Return(true));

	subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener, _1));
	subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener, _1));

	EXPECT_CALL(listener, beforeEvent(_)).Times(0);
	EXPECT_CALL(listener, afterEvent(_)).Times(0);

	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, ExecuteSequence)
{
	EXPECT_CALL(event1, isEnabled()).WillRepeatedly(Return(true));

	auto subscription1 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener1, _1));
	auto subscription2 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener1, _1));
	auto subscription3 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener2, _1));
	auto subscription4 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener2, _1));

	{
		InSequence sequence;
		EXPECT_CALL(listener1, beforeEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener2, beforeEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener, onEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener1, afterEvent(Ref(event1))).Times(1);
		EXPECT_CALL(listener2, afterEvent(Ref(event1))).Times(1);
	}

	subject1.executeEvent(event1, std::bind(&ListenerMock::onEvent, &listener, _1));
}

TEST_F(EventBusTest, BusesAreIndependent)
{
	EXPECT_CALL(event1, isEnabled()).WillRepeatedly(Return(true));

	auto subscription1 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener1, _1));
	auto subscription2 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener1, _1));
	auto subscription3 = subject2.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener2, _1));
	auto subscription4 = subject2.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener2, _1));

	EXPECT_CALL(listener1, beforeEvent(_)).Times(1);
	EXPECT_CALL(listener2, beforeEvent(_)).Times(0);
	EXPECT_CALL(listener1, afterEvent(_)).Times(1);
	EXPECT_CALL(listener2, afterEvent(_)).Times(0);

	subject1.executeEvent(event1);
}

TEST_F(EventBusTest, DisabledTestDontExecute)
{
	EXPECT_CALL(event1, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(listener, onEvent(Ref(event1))).Times(0);
	subject1.executeEvent(event1, std::bind(&ListenerMock::onEvent, &listener, _1));
}

TEST_F(EventBusTest, DisabledTestDontExecutePostHandler)
{
	EXPECT_CALL(event1, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(listener, onEvent(Ref(event1))).WillRepeatedly(Return());
	EXPECT_CALL(listener1, afterEvent(Ref(event1))).Times(0);

	auto subscription1 = subject1.subscribeAfter<EventExample>(std::bind(&ListenerMock::afterEvent, &listener1, _1));


	subject1.executeEvent(event1, std::bind(&ListenerMock::onEvent, &listener, _1));
}

TEST_F(EventBusTest, DisabledTestExecutePreHandler)
{
	EXPECT_CALL(event1, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(listener, onEvent(Ref(event1))).WillRepeatedly(Return());
	EXPECT_CALL(listener1, beforeEvent(Ref(event1))).Times(1);

	auto subscription1 = subject1.subscribeBefore<EventExample>(std::bind(&ListenerMock::beforeEvent, &listener1, _1));

	subject1.executeEvent(event1, std::bind(&ListenerMock::onEvent, &listener, _1));
}


//TODO: test sending event to destroyed event bus

}
