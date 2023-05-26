/*
* InputHandler.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputHandler.h"

#include "NotificationHandler.h"
#include "InputSourceMouse.h"
#include "InputSourceKeyboard.h"
#include "InputSourceTouch.h"
#include "InputSourceText.h"
#include "UserEventHandler.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/MouseButton.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../lib/CConfigHandler.h"

#include <SDL_events.h>
#include <SDL_hints.h>

InputHandler::InputHandler()
	: mouseHandler(std::make_unique<InputSourceMouse>())
	, keyboardHandler(std::make_unique<InputSourceKeyboard>())
	, fingerHandler(std::make_unique<InputSourceTouch>())
	, textHandler(std::make_unique<InputSourceText>())
	, userHandler(std::make_unique<UserEventHandler>())
{
}

InputHandler::~InputHandler() = default;

void InputHandler::handleCurrentEvent(const SDL_Event & current)
{
	switch (current.type)
	{
		case SDL_KEYDOWN:
			return keyboardHandler->handleEventKeyDown(current.key);
		case SDL_KEYUP:
			return keyboardHandler->handleEventKeyUp(current.key);
		case SDL_MOUSEMOTION:
			return mouseHandler->handleEventMouseMotion(current.motion);
		case SDL_MOUSEBUTTONDOWN:
			return mouseHandler->handleEventMouseButtonDown(current.button);
		case SDL_MOUSEWHEEL:
			return mouseHandler->handleEventMouseWheel(current.wheel);
		case SDL_TEXTINPUT:
			return textHandler->handleEventTextInput(current.text);
		case SDL_TEXTEDITING:
			return textHandler->handleEventTextEditing(current.edit);
		case SDL_MOUSEBUTTONUP:
			return mouseHandler->handleEventMouseButtonUp(current.button);
		case SDL_FINGERMOTION:
			return fingerHandler->handleEventFingerMotion(current.tfinger);
		case SDL_FINGERDOWN:
			return fingerHandler->handleEventFingerDown(current.tfinger);
		case SDL_FINGERUP:
			return fingerHandler->handleEventFingerUp(current.tfinger);
	}
}

void InputHandler::processEvents()
{
	boost::unique_lock<boost::mutex> lock(eventsMutex);
	for (auto const & currentEvent : eventsQueue)
		handleCurrentEvent(currentEvent);

	eventsQueue.clear();
}

bool InputHandler::ignoreEventsUntilInput()
{
	bool inputFound = false;

	boost::unique_lock<boost::mutex> lock(eventsMutex);
	for (auto const & event : eventsQueue)
	{
		switch(event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
			case SDL_FINGERDOWN:
			case SDL_KEYDOWN:
				inputFound = true;
		}
	}
	eventsQueue.clear();

	return inputFound;
}

void InputHandler::preprocessEvent(const SDL_Event & ev)
{
	if((ev.type==SDL_QUIT) ||(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT)))
	{
#ifdef VCMI_ANDROID
		handleQuit(false);
#else
		handleQuit();
#endif
		return;
	}
#ifdef VCMI_ANDROID
	else if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
	{
		handleQuit(true);
	}
#endif
	else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
	{
		Settings full = settings.write["video"]["fullscreen"];
		full->Bool() = !full->Bool();
		return;
	}
	else if(ev.type == SDL_USEREVENT)
	{
		userHandler->handleUserEvent(ev.user);

		return;
	}
	else if(ev.type == SDL_WINDOWEVENT)
	{
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_RESTORED:
#ifndef VCMI_IOS
			{
				boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);
				GH.onScreenResize();
			}
#endif
			break;
		}
		return;
	}
	else if(ev.type == SDL_SYSWMEVENT)
	{
		if(!settings["session"]["headless"].Bool() && settings["general"]["notifications"].Bool())
		{
			NotificationHandler::handleSdlEvent(ev);
		}
	}

	//preprocessing
	if(ev.type == SDL_MOUSEMOTION)
	{
		if (CCS && CCS->curh)
			CCS->curh->cursorMove(ev.motion.x, ev.motion.y);
	}

	{
		boost::unique_lock<boost::mutex> lock(eventsMutex);

		if(ev.type == SDL_MOUSEMOTION && !eventsQueue.empty() && eventsQueue.back().type == SDL_MOUSEMOTION)
		{
			// In a sequence of mouse motion events, skip all but the last one.
			// This prevents freezes when every motion event takes longer to handle than interval at which
			// the events arrive (like dragging on the minimap in world view, with redraw at every event)
			// so that the events would start piling up faster than they can be processed.
			eventsQueue.back() = ev;
			return;
		}
		eventsQueue.push_back(ev);
	}
}

void InputHandler::fetchEvents()
{
	SDL_Event ev;

	while(1 == SDL_PollEvent(&ev))
	{
		preprocessEvent(ev);
	}
}

bool InputHandler::isKeyboardCtrlDown() const
{
	return keyboardHandler->isKeyboardCtrlDown();
}

bool InputHandler::isKeyboardAltDown() const
{
	return keyboardHandler->isKeyboardAltDown();
}

bool InputHandler::isKeyboardShiftDown() const
{
	return keyboardHandler->isKeyboardShiftDown();
}

void InputHandler::moveCursorPosition(const Point & distance)
{
	setCursorPosition(getCursorPosition() + distance);
}

void InputHandler::setCursorPosition(const Point & position)
{
	cursorPosition = position;
	GH.events().dispatchMouseMoved(position);
}

void InputHandler::startTextInput(const Rect & where)
{
	textHandler->startTextInput(where);
}

void InputHandler::stopTextInput()
{
	textHandler->stopTextInput();
}

bool InputHandler::isMouseButtonPressed(MouseButton button) const
{
	return mouseHandler->isMouseButtonPressed(button) || fingerHandler->isMouseButtonPressed(button);
}

void InputHandler::pushUserEvent(EUserEvent usercode, void * userdata)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = static_cast<int32_t>(usercode);
	event.user.data1 = userdata;
	SDL_PushEvent(&event);
}

const Point & InputHandler::getCursorPosition() const
{
	return cursorPosition;
}
