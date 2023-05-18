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

void EventDispatcher::allowEventHandling(bool enable)
{
	eventHandlingAllowed = enable;
}

void EventDispatcher::processList(const ui16 mask, const ui16 flag, CIntObjectList *lst, std::function<void (CIntObjectList *)> cb)
{
	if (mask & flag)
		cb(lst);
}

void EventDispatcher::processLists(ui16 activityFlag, std::function<void (CIntObjectList *)> cb)
{
	processList(AEventsReceiver::LCLICK,activityFlag,&lclickable,cb);
	processList(AEventsReceiver::RCLICK,activityFlag,&rclickable,cb);
	processList(AEventsReceiver::MCLICK,activityFlag,&mclickable,cb);
	processList(AEventsReceiver::HOVER,activityFlag,&hoverable,cb);
	processList(AEventsReceiver::MOVE,activityFlag,&motioninterested,cb);
	processList(AEventsReceiver::KEYBOARD,activityFlag,&keyinterested,cb);
	processList(AEventsReceiver::TIME,activityFlag,&timeinterested,cb);
	processList(AEventsReceiver::WHEEL,activityFlag,&wheelInterested,cb);
	processList(AEventsReceiver::DOUBLECLICK,activityFlag,&doubleClickInterested,cb);
	processList(AEventsReceiver::TEXTINPUT,activityFlag,&textInterested,cb);
}

void EventDispatcher::handleElementActivate(AEventsReceiver * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](CIntObjectList * lst){
		lst->push_front(elem);
	});
	elem->activeState |= activityFlag;
}

void EventDispatcher::handleElementDeActivate(AEventsReceiver * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](CIntObjectList * lst){
		auto hlp = std::find(lst->begin(),lst->end(),elem);
		assert(hlp != lst->end());
		lst->erase(hlp);
	});
	elem->activeState &= ~activityFlag;
}

void EventDispatcher::dispatchTimer(uint32_t msPassed)
{
	CIntObjectList hlp = timeinterested;
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

	CIntObjectList miCopy = keyinterested;

	for(auto & i : miCopy)
	{
		if (!eventHandlingAllowed)
			break;

		for(EShortcut shortcut : shortcutsVector)
			if(vstd::contains(keyinterested, i) && (!keysCaptured || i->captureThisKey(shortcut)))
				i->keyPressed(shortcut);
	}
}

void EventDispatcher::dispatchShortcutReleased(const std::vector<EShortcut> & shortcutsVector)
{
	bool keysCaptured = false;

	for(auto & i : keyinterested)
		for(EShortcut shortcut : shortcutsVector)
			if(i->captureThisKey(shortcut))
				keysCaptured = true;

	CIntObjectList miCopy = keyinterested;

	for(auto & i : miCopy)
	{
		if (!eventHandlingAllowed)
			break;

		for(EShortcut shortcut : shortcutsVector)
			if(vstd::contains(keyinterested, i) && (!keysCaptured || i->captureThisKey(shortcut)))
				i->keyReleased(shortcut);
	}
}

EventDispatcher::CIntObjectList & EventDispatcher::getListForMouseButton(MouseButton button)
{
	switch (button)
	{
		case MouseButton::LEFT:
			return lclickable;
		case MouseButton::RIGHT:
			return rclickable;
		case MouseButton::MIDDLE:
			return mclickable;
	}
	throw std::runtime_error("Invalid mouse button in getListForMouseButton");
}

void EventDispatcher::dispatchMouseDoubleClick(const Point & position)
{
	bool doubleClicked = false;
	auto hlp = doubleClickInterested;

	for(auto & i : hlp)
	{
		if(!vstd::contains(doubleClickInterested, i))
			continue;

		if (!eventHandlingAllowed)
			break;

		if(i->isInside(position))
		{
			i->onDoubleClick();
			doubleClicked = true;
		}
	}

	if(!doubleClicked)
		dispatchMouseButtonPressed(MouseButton::LEFT, position);
}

void EventDispatcher::dispatchMouseButtonPressed(const MouseButton & button, const Point & position)
{
	handleMouseButtonClick(getListForMouseButton(button), button, true);
}

void EventDispatcher::dispatchMouseButtonReleased(const MouseButton & button, const Point & position)
{
	handleMouseButtonClick(getListForMouseButton(button), button, false);
}

void EventDispatcher::handleMouseButtonClick(CIntObjectList & interestedObjs, MouseButton btn, bool isPressed)
{
	auto hlp = interestedObjs;
	for(auto & i : hlp)
	{
		if(!vstd::contains(interestedObjs, i))
			continue;

		if (!eventHandlingAllowed)
			break;

		auto prev = i->isMouseButtonPressed(btn);
		if(!isPressed)
			i->currentMouseState[btn] = isPressed;
		if(i->isInside(GH.getCursorPosition()))
		{
			if(isPressed)
				i->currentMouseState[btn] = isPressed;
			i->click(btn, isPressed, prev);
		}
		else if(!isPressed)
			i->click(btn, boost::logic::indeterminate, prev);
	}
}

void EventDispatcher::dispatchMouseScrolled(const Point & distance, const Point & position)
{
	CIntObjectList hlp = wheelInterested;
	for(auto i = hlp.begin(); i != hlp.end() && eventHandlingAllowed; i++)
	{
		if(!vstd::contains(wheelInterested,*i))
			continue;
		(*i)->wheelScrolled(distance.y < 0, (*i)->isInside(position));
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

void EventDispatcher::dispatchMouseMoved(const Point & position)
{
	//sending active, hovered hoverable objects hover() call
	CIntObjectList hlp;

	auto hoverableCopy = hoverable;
	for(auto & elem : hoverableCopy)
	{
		if(elem->isInside(GH.getCursorPosition()))
		{
			if (!(elem)->isHovered())
				hlp.push_back((elem));
		}
		else if ((elem)->isHovered())
		{
			(elem)->hover(false);
			(elem)->hoveredState = false;
		}
	}

	for(auto & elem : hlp)
	{
		elem->hover(true);
		elem->hoveredState = true;
	}

	//sending active, MotionInterested objects mouseMoved() call
	CIntObjectList miCopy = motioninterested;
	for(auto & elem : miCopy)
	{
		if(elem->strongInterestState || elem->isInside(position)) //checking bounds including border fixes bug #2476
		{
			(elem)->mouseMoved(position);
		}
	}
}
