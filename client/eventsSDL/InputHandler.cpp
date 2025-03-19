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
#include "InputSourceGameController.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/MouseButton.h"
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../lib/CConfigHandler.h"

#include <SDL_events.h>
#include <SDL_timer.h>
#include <SDL_clipboard.h>

InputHandler::InputHandler()
	: enableMouse(settings["input"]["enableMouse"].Bool())
	, enableTouch(settings["input"]["enableTouch"].Bool())
	, enableController(settings["input"]["enableController"].Bool())
	, currentInputMode(InputMode::KEYBOARD_AND_MOUSE)
	, mouseHandler(std::make_unique<InputSourceMouse>())
	, keyboardHandler(std::make_unique<InputSourceKeyboard>())
	, fingerHandler(std::make_unique<InputSourceTouch>())
	, textHandler(std::make_unique<InputSourceText>())
	, gameControllerHandler(std::make_unique<InputSourceGameController>())
{
}

InputHandler::~InputHandler() = default;

void InputHandler::handleCurrentEvent(const SDL_Event & current)
{
	switch (current.type)
	{
		case SDL_KEYDOWN:
			setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
			keyboardHandler->handleEventKeyDown(current.key);
			return;
		case SDL_KEYUP:
			keyboardHandler->handleEventKeyUp(current.key);
			return;
#ifndef VCMI_EMULATE_TOUCHSCREEN_WITH_MOUSE
		case SDL_MOUSEMOTION:
			if (enableMouse)
			{
				setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
				mouseHandler->handleEventMouseMotion(current.motion);
			}
			return;
		case SDL_MOUSEBUTTONDOWN:
			if (enableMouse)
			{
				setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
				mouseHandler->handleEventMouseButtonDown(current.button);
			}
			return;
		case SDL_MOUSEBUTTONUP:
			if (enableMouse)
				mouseHandler->handleEventMouseButtonUp(current.button);
			return;
		case SDL_MOUSEWHEEL:
			if (enableMouse)
				mouseHandler->handleEventMouseWheel(current.wheel);
			return;
#endif
		case SDL_TEXTINPUT:
			textHandler->handleEventTextInput(current.text);
			return;
		case SDL_TEXTEDITING:
			textHandler->handleEventTextEditing(current.edit);
			return;
		case SDL_FINGERMOTION:
			if (enableTouch)
			{
				setCurrentInputMode(InputMode::TOUCH);
				fingerHandler->handleEventFingerMotion(current.tfinger);
			}
			return;
		case SDL_FINGERDOWN:
			if (enableTouch)
			{
				setCurrentInputMode(InputMode::TOUCH);
				fingerHandler->handleEventFingerDown(current.tfinger);
			}
			return;
		case SDL_FINGERUP:
			if (enableTouch)
				fingerHandler->handleEventFingerUp(current.tfinger);
			return;
		case SDL_CONTROLLERAXISMOTION:
			if (enableController)
			{
				setCurrentInputMode(InputMode::CONTROLLER);
				gameControllerHandler->handleEventAxisMotion(current.caxis);
			}
			return;
		case SDL_CONTROLLERBUTTONDOWN:
			if (enableController)
			{
				setCurrentInputMode(InputMode::CONTROLLER);
				gameControllerHandler->handleEventButtonDown(current.cbutton);
			}
			return;
		case SDL_CONTROLLERBUTTONUP:
			if (enableController)
				gameControllerHandler->handleEventButtonUp(current.cbutton);
			return;
	}
}

void InputHandler::setCurrentInputMode(InputMode modi)
{
	if(currentInputMode != modi)
	{
		currentInputMode = modi;
		GH.events().dispatchInputModeChanged(modi);
	}
}

InputMode InputHandler::getCurrentInputMode()
{
	return currentInputMode;
}

void InputHandler::copyToClipBoard(const std::string & text)
{
	SDL_SetClipboardText(text.c_str());
}

std::vector<SDL_Event> InputHandler::acquireEvents()
{
	boost::unique_lock<boost::mutex> lock(eventsMutex);

	std::vector<SDL_Event> result;
	std::swap(result, eventsQueue);
	return result;
}

void InputHandler::processEvents()
{
	std::vector<SDL_Event> eventsToProcess = acquireEvents();

	for(const auto & currentEvent : eventsToProcess)
		handleCurrentEvent(currentEvent);

	gameControllerHandler->handleUpdate();
	fingerHandler->handleUpdate();
}

bool InputHandler::ignoreEventsUntilInput()
{
	bool inputFound = false;

	boost::unique_lock<boost::mutex> lock(eventsMutex);
	for(const auto & event : eventsQueue)
	{
		switch(event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
			case SDL_FINGERDOWN:
			case SDL_KEYDOWN:
			case SDL_CONTROLLERBUTTONDOWN:
				inputFound = true;
		}
	}
	eventsQueue.clear();

	return inputFound;
}

void InputHandler::preprocessEvent(const SDL_Event & ev)
{
	if(ev.type == SDL_QUIT)
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
#ifdef VCMI_ANDROID
		handleQuit(false);
#else
		handleQuit(true);
#endif
		return;
	}
	else if(ev.type == SDL_KEYDOWN)
	{
		if(ev.key.keysym.sym == SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT))
		{
			// FIXME: dead code? Looks like intercepted by OS/SDL and delivered as SDL_Quit instead?
			boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
			handleQuit(true);
			return;
		}

		if(ev.key.keysym.scancode == SDL_SCANCODE_AC_BACK && !settings["input"]["handleBackRightMouseButton"].Bool())
		{
			boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
			handleQuit(true);
			return;
		}
	}
	else if(ev.type == SDL_USEREVENT)
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
		handleUserEvent(ev.user);

		return;
	}
	else if(ev.type == SDL_WINDOWEVENT)
	{
		switch (ev.window.event) {
			case SDL_WINDOWEVENT_RESTORED:
#ifndef VCMI_IOS
			{
				boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
				GH.onScreenResize(false);
			}
#endif
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
#ifdef VCMI_MOBILE
			{
				boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
				GH.onScreenResize(true);
			}
#endif
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
			{
				boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
				if(settings["general"]["audioMuteFocus"].Bool()) {
					CCS->musich->setVolume(settings["general"]["music"].Integer());
					CCS->soundh->setVolume(settings["general"]["sound"].Integer());
				}
			}
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
			{
				boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
				if(settings["general"]["audioMuteFocus"].Bool()) {
					CCS->musich->setVolume(0);
					CCS->soundh->setVolume(0);
				}
			}
				break;
		}
		return;
	}
	else if(ev.type == SDL_SYSWMEVENT)
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
		if(!settings["session"]["headless"].Bool() && settings["general"]["notifications"].Bool())
		{
			NotificationHandler::handleSdlEvent(ev);
		}
	}
	else if(ev.type == SDL_CONTROLLERDEVICEADDED)
	{
		gameControllerHandler->handleEventDeviceAdded(ev.cdevice);
		return;
	}
	else if(ev.type == SDL_CONTROLLERDEVICEREMOVED)
	{
		gameControllerHandler->handleEventDeviceRemoved(ev.cdevice);
		return;
	}
	else if(ev.type == SDL_CONTROLLERDEVICEREMAPPED)
	{
		gameControllerHandler->handleEventDeviceRemapped(ev.cdevice);
		return;
	}

	//preprocessing
	if(ev.type == SDL_MOUSEMOTION)
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
		if (CCS && CCS->curh)
			CCS->curh->cursorMove(ev.motion.x, ev.motion.y);
	}

	{
		boost::unique_lock<boost::mutex> lock(eventsMutex);

		// In a sequence of motion events, skip all but the last one.
		// This prevents freezes when every motion event takes longer to handle than interval at which
		// the events arrive (like dragging on the minimap in world view, with redraw at every event)
		// so that the events would start piling up faster than they can be processed.
		if (!eventsQueue.empty())
		{
			const SDL_Event & prev = eventsQueue.back();

			if(ev.type == SDL_MOUSEMOTION && prev.type == SDL_MOUSEMOTION)
			{
				SDL_Event accumulated = ev;
				accumulated.motion.xrel += prev.motion.xrel;
				accumulated.motion.yrel += prev.motion.yrel;
				eventsQueue.back() = accumulated;
				return;
			}

			if(ev.type == SDL_FINGERMOTION && prev.type == SDL_FINGERMOTION && ev.tfinger.fingerId == prev.tfinger.fingerId)
			{
				SDL_Event accumulated = ev;
				accumulated.tfinger.dx += prev.tfinger.dx;
				accumulated.tfinger.dy += prev.tfinger.dy;
				eventsQueue.back() = accumulated;
				return;
			}
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

bool InputHandler::isKeyboardCmdDown() const
{
	return keyboardHandler->isKeyboardCmdDown();
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
	GH.events().dispatchMouseMoved(Point(0, 0), position);
}

void InputHandler::startTextInput(const Rect & where)
{
	textHandler->startTextInput(where);
}

void InputHandler::stopTextInput()
{
	textHandler->stopTextInput();
}

void InputHandler::hapticFeedback()
{
	if(currentInputMode == InputMode::TOUCH)
		fingerHandler->hapticFeedback();
}

uint32_t InputHandler::getTicks()
{
	return SDL_GetTicks();
}

bool InputHandler::hasTouchInputDevice() const
{
	return fingerHandler->hasTouchInputDevice();
}

int InputHandler::getNumTouchFingers() const
{
	if(currentInputMode != InputMode::TOUCH)
		return 0;
	return fingerHandler->getNumTouchFingers();
}

void InputHandler::dispatchMainThread(const std::function<void()> & functor)
{
	auto heapFunctor = new std::function<void()>(functor);

	SDL_Event event;
	event.user.type = SDL_USEREVENT;
	event.user.code = 0;
	event.user.data1 = static_cast <void*>(heapFunctor);
	event.user.data2 = nullptr;
	SDL_PushEvent(&event);
}

void InputHandler::handleUserEvent(const SDL_UserEvent & current)
{
	auto heapFunctor = static_cast<std::function<void()>*>(current.data1);

	(*heapFunctor)();

	delete heapFunctor;
}

const Point & InputHandler::getCursorPosition() const
{
	return cursorPosition;
}
