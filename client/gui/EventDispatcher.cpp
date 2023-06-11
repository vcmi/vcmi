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
	processList(AEventsReceiver::RCLICK, rclickable);
	processList(AEventsReceiver::HOVER, hoverable);
	processList(AEventsReceiver::MOVE, motioninterested);
	processList(AEventsReceiver::KEYBOARD, keyinterested);
	processList(AEventsReceiver::TIME, timeinterested);
	processList(AEventsReceiver::WHEEL, wheelInterested);
	processList(AEventsReceiver::DOUBLECLICK, doubleClickInterested);
	processList(AEventsReceiver::TEXTINPUT, textInterested);
	processList(AEventsReceiver::GESTURE_PANNING, panningInterested);
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
		if(!vstd::contains(timeinterested,elem)) continue;
		(elem)->tick(msPassed);
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
			i->clickDouble();
			doubleClicked = true;
		}
	}

	if(!doubleClicked)
		dispatchMouseButtonPressed(MouseButton::LEFT, position);
}

void EventDispatcher::dispatchMouseButtonPressed(const MouseButton & button, const Point & position)
{
	if (button == MouseButton::LEFT)
		handleLeftButtonClick(true);
	if (button == MouseButton::RIGHT)
		handleRightButtonClick(true);
}

void EventDispatcher::dispatchMouseButtonReleased(const MouseButton & button, const Point & position)
{
	if (button == MouseButton::LEFT)
		handleLeftButtonClick(false);
	if (button == MouseButton::RIGHT)
		handleRightButtonClick(false);
}

void EventDispatcher::handleRightButtonClick(bool isPressed)
{
	auto hlp = rclickable;
	for(auto & i : hlp)
	{
		if(!vstd::contains(rclickable, i))
			continue;

		if( isPressed && i->receiveEvent(GH.getCursorPosition(), AEventsReceiver::LCLICK))
			i->showPopupWindow();

		if(!isPressed)
			i->closePopupWindow();
	}
}

void EventDispatcher::handleLeftButtonClick(bool isPressed)
{
	auto hlp = lclickable;
	for(auto & i : hlp)
	{
		if(!vstd::contains(lclickable, i))
			continue;

		auto prev = i->isMouseButtonPressed(MouseButton::LEFT);

		if(!isPressed)
			i->currentMouseState[MouseButton::LEFT] = isPressed;

		if( i->receiveEvent(GH.getCursorPosition(), AEventsReceiver::LCLICK))
		{
			if(isPressed)
				i->currentMouseState[MouseButton::LEFT] = isPressed;
			i->clickLeft(isPressed, prev);
		}
		else if(!isPressed)
		{
			i->clickLeft(boost::logic::indeterminate, prev);
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
		if (it->receiveEvent(initialPosition, AEventsReceiver::GESTURE_PANNING))
		{
			it->panning(true, initialPosition, initialPosition);
			it->panningState = true;
		}
	}
}

void EventDispatcher::dispatchGesturePanningEnded(const Point & initialPosition, const Point & finalPosition)
{
	auto copied = panningInterested;

	for(auto it : copied)
	{
		if (it->isPanning())
		{
			it->panning(false, initialPosition, finalPosition);
			it->panningState = false;
		}
	}
}

void EventDispatcher::dispatchGesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	auto copied = panningInterested;

	for(auto it : copied)
	{
		if (it->isPanning())
			it->gesturePanning(initialPosition, currentPosition, lastUpdateDistance);
	}
}

void EventDispatcher::dispatchGesturePinch(const Point & initialPosition, double distance)
{
	for(auto it : panningInterested)
	{
		if (it->isPanning())
			it->gesturePinch(initialPosition, distance);
	}
}

void EventDispatcher::dispatchMouseMoved(const Point & position)
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
				(elem)->hover(false);
				(elem)->hoveredState = false;
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
		if(elem->receiveEvent(position, AEventsReceiver::HOVER))
		{
			(elem)->mouseMoved(position);
		}
	}
}
