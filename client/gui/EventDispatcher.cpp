/*
 * EventDispatcher.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EventDispatcher.h"

#include "EventsReceiver.h"
#include "FramerateManager.h"
#include "CGuiHandler.h"
#include "MouseButton.h"
#include "WindowHandler.h"

#include "../../lib/Point.h"

template<typename Functor>
void EventDispatcher::processLists(ui16 activityFlag, const Functor & cb)
{
	auto processList = [&](ui16 mask, EventReceiversList & lst)
	{
		if(mask & activityFlag)
			cb(lst);
	};

	processList(AEventsReceiver::LCLICK, lclickable);
	processList(AEventsReceiver::SHOW_POPUP, rclickable);
	processList(AEventsReceiver::HOVER, hoverable);
	processList(AEventsReceiver::MOVE, motioninterested);
	processList(AEventsReceiver::DRAG, draginterested);
	processList(AEventsReceiver::KEYBOARD, keyinterested);
	processList(AEventsReceiver::TIME, timeinterested);
	processList(AEventsReceiver::WHEEL, wheelInterested);
	processList(AEventsReceiver::DOUBLECLICK, doubleClickInterested);
	processList(AEventsReceiver::TEXTINPUT, textInterested);
	processList(AEventsReceiver::GESTURE, panningInterested);
}

void EventDispatcher::activateElement(AEventsReceiver * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](EventReceiversList & lst){
		lst.push_front(elem);
	});
	elem->activeState |= activityFlag;
}

void EventDispatcher::deactivateElement(AEventsReceiver * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](EventReceiversList & lst){
		auto hlp = std::find(lst.begin(),lst.end(),elem);
		assert(hlp != lst.end());
		lst.erase(hlp);
	});
	elem->activeState &= ~activityFlag;
}

void EventDispatcher::dispatchTimer(uint32_t msPassed)
{
	EventReceiversList hlp = timeinterested;
	for (auto & elem : hlp)
	{
		if(!vstd::contains(timeinterested,elem))
			continue;

		elem->tick(msPassed);
	}
}

void EventDispatcher::dispatchShortcutPressed(const std::vector<EShortcut> & shortcutsVector)
{
	bool keysCaptured = false;

	for(auto & i : keyinterested)
		for(EShortcut shortcut : shortcutsVector)
			if(i->captureThisKey(shortcut))
				keysCaptured = true;

	EventReceiversList miCopy = keyinterested;

	for(auto & i : miCopy)
	{
		for(EShortcut shortcut : shortcutsVector)
			if(vstd::contains(keyinterested, i) && (!keysCaptured || i->captureThisKey(shortcut)))
			{
				i->keyPressed(shortcut);
				if (keysCaptured)
					return;
			}
	}
}

void EventDispatcher::dispatchShortcutReleased(const std::vector<EShortcut> & shortcutsVector)
{
	bool keysCaptured = false;

	for(auto & i : keyinterested)
		for(EShortcut shortcut : shortcutsVector)
			if(i->captureThisKey(shortcut))
				keysCaptured = true;

	EventReceiversList miCopy = keyinterested;

	for(auto & i : miCopy)
	{
		for(EShortcut shortcut : shortcutsVector)
			if(vstd::contains(keyinterested, i) && (!keysCaptured || i->captureThisKey(shortcut)))
			{
				i->keyReleased(shortcut);
				if (keysCaptured)
					return;
			}
	}
}

void EventDispatcher::dispatchMouseDoubleClick(const Point & position)
{
	bool doubleClicked = false;
	auto hlp = doubleClickInterested;

	for(auto & i : hlp)
	{
		if(!vstd::contains(doubleClickInterested, i))
			continue;

		if(i->receiveEvent(position, AEventsReceiver::DOUBLECLICK))
		{
			i->clickDouble(position);
			doubleClicked = true;
		}
	}

	if(!doubleClicked)
		handleLeftButtonClick(position, true);
}

void EventDispatcher::dispatchMouseLeftButtonPressed(const Point & position)
{
	handleLeftButtonClick(position, true);
}

void EventDispatcher::dispatchMouseLeftButtonReleased(const Point & position)
{
	handleLeftButtonClick(position, false);
}

void EventDispatcher::dispatchShowPopup(const Point & position)
{
	auto hlp = rclickable;
	for(auto & i : hlp)
	{
		if(!vstd::contains(rclickable, i))
			continue;

		if( !i->receiveEvent(position, AEventsReceiver::LCLICK))
			continue;

		i->showPopupWindow(position);
	}
}

void EventDispatcher::dispatchClosePopup(const Point & position)
{
	if (GH.windows().isTopWindowPopup())
		GH.windows().popWindows(1);

	assert(!GH.windows().isTopWindowPopup());
}

void EventDispatcher::handleLeftButtonClick(const Point & position, bool isPressed)
{
	// WARNING: this approach is NOT SAFE
	// 1) We allow (un)registering elements when list itself is being processed/iterated
	// 2) To avoid iterator invalidation we create a copy of this list for processing
	// HOWEVER it is completely possible (as in, actually happen, no just theory) to:
	// 1) element gets unregistered and deleted from lclickable
	// 2) element is completely deleted, as in - destructor called, memory freed
	// 3) new element is created *with exactly same address(!)
	// 4) new element is registered and code will incorrectly assume that this element is still registered
	// POSSIBLE SOLUTION: make EventReceivers inherit from create_shared_from this and store weak_ptr's in lists
	auto hlp = lclickable;
	for(auto & i : hlp)
	{
		if(!vstd::contains(lclickable, i))
			continue;

		if( i->receiveEvent(position, AEventsReceiver::LCLICK))
		{
			if(isPressed)
				i->clickPressed(position);

			if (i->mouseClickedState && !isPressed)
				i->clickReleased(position);

			i->mouseClickedState = isPressed;
		}
		else
		{
			if(i->mouseClickedState && !isPressed)
			{
				i->mouseClickedState = isPressed;
				i->clickCancel(position);
			}
		}
	}
}

void EventDispatcher::dispatchMouseScrolled(const Point & distance, const Point & position)
{
	EventReceiversList hlp = wheelInterested;
	for(auto & i : hlp)
	{
		if(!vstd::contains(wheelInterested,i))
			continue;

		if (i->receiveEvent(position, AEventsReceiver::WHEEL))
			i->wheelScrolled(distance.y);
	}
}

void EventDispatcher::dispatchTextInput(const std::string & text)
{
	for(auto it : textInterested)
	{
		it->textInputed(text);
	}
}

void EventDispatcher::dispatchTextEditing(const std::string & text)
{
	for(auto it : textInterested)
	{
		it->textEdited(text);
	}
}

void EventDispatcher::dispatchGesturePanningStarted(const Point & initialPosition)
{
	auto copied = panningInterested;

	for(auto it : copied)
	{
		if (it->receiveEvent(initialPosition, AEventsReceiver::GESTURE))
		{
			it->gesture(true, initialPosition, initialPosition);
			it->panningState = true;
		}
	}
}

void EventDispatcher::dispatchGesturePanningEnded(const Point & initialPosition, const Point & finalPosition)
{
	auto copied = panningInterested;

	for(auto it : copied)
	{
		if (it->isGesturing())
		{
			it->gesture(false, initialPosition, finalPosition);
			it->panningState = false;
		}
	}
}

void EventDispatcher::dispatchGesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	auto copied = panningInterested;

	for(auto it : copied)
	{
		if (!it->isGesturing() && it->receiveEvent(initialPosition, AEventsReceiver::GESTURE))
		{
			it->gesture(true, initialPosition, initialPosition);
			it->panningState = true;
		}

		if (it->isGesturing())
			it->gesturePanning(initialPosition, currentPosition, lastUpdateDistance);
	}
}

void EventDispatcher::dispatchGesturePinch(const Point & initialPosition, double distance)
{
	for(auto it : panningInterested)
	{
		if (it->isGesturing())
			it->gesturePinch(initialPosition, distance);
	}
}

void EventDispatcher::dispatchMouseMoved(const Point & distance, const Point & position)
{
	EventReceiversList newlyHovered;

	auto hoverableCopy = hoverable;
	for(auto & elem : hoverableCopy)
	{
		if(elem->receiveEvent(position, AEventsReceiver::HOVER))
		{
			if (!elem->isHovered())
			{
				newlyHovered.push_back((elem));
			}
		}
		else
		{
			if (elem->isHovered())
			{
				elem->hover(false);
				elem->hoveredState = false;
			}
		}
	}

	for(auto & elem : newlyHovered)
	{
		elem->hover(true);
		elem->hoveredState = true;
	}

	//sending active, MotionInterested objects mouseMoved() call
	EventReceiversList miCopy = motioninterested;
	for(auto & elem : miCopy)
	{
		if (!vstd::contains(motioninterested, elem))
			continue;

		if(elem->receiveEvent(position, AEventsReceiver::HOVER))
			elem->mouseMoved(position, distance);
	}
}

void EventDispatcher::dispatchMouseDragged(const Point & currentPosition, const Point & lastUpdateDistance)
{
	EventReceiversList diCopy = draginterested;
	for(auto & elem : diCopy)
	{
		if (elem->mouseClickedState)
			elem->mouseDragged(currentPosition, lastUpdateDistance);
	}
}
