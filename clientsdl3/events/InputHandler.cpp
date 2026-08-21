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

#include "GameEngine.h"
#include "GameEngineUser.h"
#include "gui/CursorHandler.h"
#include "gui/EventDispatcher.h"
#include "gui/MouseButton.h"
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
#include "CMT.h"
#include "CPlayerInterface.h"

#include "lib/AsyncRunner.h"
#include "lib/CConfigHandler.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_power.h>

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
	, cachedPowerStateMode(static_cast<int>(PowerStateMode::UNKNOWN))
	, cachedPowerStateSeconds(-1)
	, cachedPowerStatePercent(-1)
	, powerStateFrameCounter(0)
{
}

InputHandler::~InputHandler() = default;

void InputHandler::handleCurrentEvent(const SDL_Event & current)
{
	switch (current.type)
	{
		case SDL_EVENT_KEY_DOWN:
			setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
			keyboardHandler->handleEventKeyDown(current.key);
			return;
		case SDL_EVENT_KEY_UP:
			keyboardHandler->handleEventKeyUp(current.key);
			return;
#ifndef VCMI_EMULATE_TOUCHSCREEN_WITH_MOUSE
		case SDL_EVENT_MOUSE_MOTION:
			// a hovering pen is only reported here, a dragging one by the finger events
			if (current.motion.which == SDL_PEN_MOUSEID)
			{
				if (enableMouse && !penIsTouching)
				{
					setCurrentInputMode(InputMode::PEN);
					mouseHandler->handleEventMouseMotion(current.motion);
				}
				return;
			}
			if (enableMouse)
			{
				setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
				mouseHandler->handleEventMouseMotion(current.motion);
			}
			return;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			// the tip repeats the tap the finger events deliver, the barrel buttons do not
			if (current.button.which == SDL_PEN_MOUSEID)
			{
				if (enableMouse && current.button.button != SDL_BUTTON_LEFT)
				{
					setCurrentInputMode(InputMode::PEN);
					mouseHandler->handleEventMouseButtonDown(current.button);
				}
				return;
			}
			if (enableMouse)
			{
				setCurrentInputMode(InputMode::KEYBOARD_AND_MOUSE);
				mouseHandler->handleEventMouseButtonDown(current.button);
			}
			return;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (current.button.which == SDL_PEN_MOUSEID)
			{
				if (enableMouse && current.button.button != SDL_BUTTON_LEFT)
					mouseHandler->handleEventMouseButtonUp(current.button);
				return;
			}
			if (enableMouse)
				mouseHandler->handleEventMouseButtonUp(current.button);
			return;
		case SDL_EVENT_MOUSE_WHEEL:
			if (enableMouse)
				mouseHandler->handleEventMouseWheel(current.wheel);
			return;
#endif
		case SDL_EVENT_TEXT_INPUT:
			textHandler->handleEventTextInput(current.text);
			return;
		case SDL_EVENT_TEXT_EDITING:
			textHandler->handleEventTextEditing(current.edit);
			return;
		case SDL_EVENT_FINGER_MOTION:
			if (enableTouch)
			{
				setCurrentInputMode(inputModeForTouch(current.tfinger));
				fingerHandler->handleEventFingerMotion(current.tfinger);
			}
			return;
		case SDL_EVENT_FINGER_DOWN:
			if (current.tfinger.touchID == SDL_PEN_TOUCHID)
				penIsTouching = true;
			if (enableTouch)
			{
				setCurrentInputMode(inputModeForTouch(current.tfinger));
				fingerHandler->handleEventFingerDown(current.tfinger);
			}
			return;
		case SDL_EVENT_FINGER_UP:
			if (current.tfinger.touchID == SDL_PEN_TOUCHID)
				penIsTouching = false;
			if (enableTouch)
				fingerHandler->handleEventFingerUp(current.tfinger);
			return;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			if (enableController)
			{
				if(gameControllerHandler->isAxisMotionActive(current.gaxis))
				{
					gameControllerHandler->setActiveController(current.gaxis.which);
					setCurrentInputMode(InputMode::CONTROLLER);
				}
				gameControllerHandler->handleEventAxisMotion(current.gaxis);
			}
			return;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			if (enableController)
			{
				gameControllerHandler->setActiveController(current.gbutton.which);
				setCurrentInputMode(InputMode::CONTROLLER);
				gameControllerHandler->handleEventButtonDown(current.gbutton);
			}
			return;
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			if (enableController)
				gameControllerHandler->handleEventButtonUp(current.gbutton);
			return;
	}
}

InputMode InputHandler::inputModeForTouch(const SDL_TouchFingerEvent & tfinger)
{
	return tfinger.touchID == SDL_PEN_TOUCHID ? InputMode::PEN : InputMode::TOUCH;
}

void InputHandler::setCurrentInputMode(InputMode modi)
{
	if(currentInputMode != modi)
	{
		currentInputMode = modi;
		ENGINE->events().dispatchInputModeChanged(modi);
	}
}

InputMode InputHandler::getCurrentInputMode()
{
	return currentInputMode;
}

ControllerPrompt::Family InputHandler::getActiveControllerPromptFamily() const
{
	return gameControllerHandler->getActiveControllerPromptFamily();
}

bool InputHandler::inputModeSupportsHover() const
{
	return currentInputMode != InputMode::TOUCH;
}

bool InputHandler::inputModeUsesGestures() const
{
	return currentInputMode == InputMode::TOUCH || currentInputMode == InputMode::PEN;
}

void InputHandler::copyToClipBoard(const std::string & text)
{
	SDL_SetClipboardText(text.c_str());
}

void InputHandler::updatePowerState()
{
	int seconds;
	int percent;
	auto sdlPowerState = SDL_GetPowerInfo(&seconds, &percent);

	PowerStateMode powerState = PowerStateMode::UNKNOWN;
	if(sdlPowerState == SDL_POWERSTATE_ON_BATTERY)
		powerState = PowerStateMode::ON_BATTERY;
	else if(sdlPowerState == SDL_POWERSTATE_CHARGING || sdlPowerState == SDL_POWERSTATE_CHARGED)
		powerState = PowerStateMode::CHARGING;

	cachedPowerStateMode.store(static_cast<int>(powerState), std::memory_order_relaxed);
	cachedPowerStateSeconds.store(seconds, std::memory_order_relaxed);
	cachedPowerStatePercent.store(percent, std::memory_order_relaxed);
}

PowerState InputHandler::getPowerState()
{
	return PowerState{
		static_cast<PowerStateMode>(cachedPowerStateMode.load(std::memory_order_relaxed)),
		cachedPowerStateSeconds.load(std::memory_order_relaxed),
		cachedPowerStatePercent.load(std::memory_order_relaxed)
	};
}

std::vector<SDL_Event> InputHandler::acquireEvents()
{
	std::unique_lock<std::mutex> lock(eventsMutex);

	std::vector<SDL_Event> result;
	std::swap(result, eventsQueue);
	return result;
}

void InputHandler::processEvents()
{
	
	// Update power state every ~300 frames (approx 5 seconds at 60 FPS)
	if (++powerStateFrameCounter >= 300)
	{
		const auto updateTask = [this]()
		{
			updatePowerState();
		};
		ENGINE->async().run(updateTask);
		powerStateFrameCounter = 0;
	}
	
	std::vector<SDL_Event> eventsToProcess = acquireEvents();

	for(const auto & currentEvent : eventsToProcess)
		handleCurrentEvent(currentEvent);

	gameControllerHandler->handleUpdate();
	fingerHandler->handleUpdate();
}

bool InputHandler::ignoreEventsUntilInput()
{
	bool inputFound = false;

	std::unique_lock<std::mutex> lock(eventsMutex);
	for(const auto & event : eventsQueue)
	{
		switch(event.type)
		{
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_FINGER_DOWN:
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				inputFound = true;
		}
	}
	eventsQueue.clear();

	return inputFound;
}

void InputHandler::preprocessEvent(const SDL_Event & ev)
{
	if(ev.type == SDL_EVENT_QUIT)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
#ifdef VCMI_ANDROID
		ENGINE->user().onShutdownRequested(false);
#else
		ENGINE->user().onShutdownRequested(true);
#endif
		return;
	}
	else if(ev.type == SDL_EVENT_WILL_ENTER_BACKGROUND)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		ENGINE->user().onAppPaused();
		return;
	}
	else if(ev.type == SDL_EVENT_KEY_DOWN)
	{
		if(ev.key.key == SDLK_F4 && (ev.key.mod & SDL_KMOD_ALT))
		{
			// FIXME: dead code? Looks like intercepted by OS/SDL and delivered as SDL_Quit instead?
			std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
			ENGINE->user().onShutdownRequested(true);
			return;
		}

		if(ev.key.scancode == SDL_SCANCODE_AC_BACK && !settings["input"]["handleBackRightMouseButton"].Bool())
		{
			std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
			ENGINE->user().onShutdownRequested(true);
			return;
		}
	}
	else if(ev.type == SDL_EVENT_USER)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		handleUserEvent(ev.user);

		return;
	}
	// SDL3 delivers each window event as its own event type instead of a single SDL_WINDOWEVENT
	else if(ev.type == SDL_EVENT_WINDOW_RESTORED)
	{
#ifndef VCMI_IOS
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		ENGINE->onScreenResize(false, false);
#endif
		return;
	}
	// SDL3 split SDL2's SIZE_CHANGED in two. Android posts only RESIZED - it enters immersive
	// mode after the window exists - and SDL drops the derived pixel size event as a duplicate.
	else if(ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || ev.type == SDL_EVENT_WINDOW_RESIZED)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
#ifdef VCMI_MOBILE
		ENGINE->onScreenResize(true, false);
#else
		ENGINE->onScreenResize(true, true);
#endif
		return;
	}
	else if(ev.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		if(settings["general"]["audioMuteFocus"].Bool()) {
			ENGINE->music().setVolume(settings["general"]["music"].Integer());
			ENGINE->sound().setVolume(settings["general"]["sound"].Integer());
		}
		return;
	}
	else if(ev.type == SDL_EVENT_WINDOW_FOCUS_LOST)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		if(settings["general"]["audioMuteFocus"].Bool()) {
			ENGINE->music().setVolume(0);
			ENGINE->sound().setVolume(0);
		}
		return;
	}
	else if(ev.type == SDL_EVENT_GAMEPAD_ADDED)
	{
		gameControllerHandler->handleEventDeviceAdded(ev.gdevice);
		return;
	}
	else if(ev.type == SDL_EVENT_GAMEPAD_REMOVED)
	{
		gameControllerHandler->handleEventDeviceRemoved(ev.gdevice);
		return;
	}
	else if(ev.type == SDL_EVENT_GAMEPAD_REMAPPED)
	{
		gameControllerHandler->handleEventDeviceRemapped(ev.gdevice);
		return;
	}

#ifndef VCMI_EMULATE_TOUCHSCREEN_WITH_MOUSE
	//preprocessing
	if(ev.type == SDL_EVENT_MOUSE_MOTION)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		ENGINE->cursor().cursorMove(ev.motion.x, ev.motion.y);
	}
#endif

	{
		std::unique_lock<std::mutex> lock(eventsMutex);

		// In a sequence of motion events, skip all but the last one.
		// This prevents freezes when every motion event takes longer to handle than interval at which
		// the events arrive (like dragging on the minimap in world view, with redraw at every event)
		// so that the events would start piling up faster than they can be processed.
		if (!eventsQueue.empty())
		{
			const SDL_Event & prev = eventsQueue.back();

			if(ev.type == SDL_EVENT_MOUSE_MOTION && prev.type == SDL_EVENT_MOUSE_MOTION)
			{
				SDL_Event accumulated = ev;
				accumulated.motion.xrel += prev.motion.xrel;
				accumulated.motion.yrel += prev.motion.yrel;
				eventsQueue.back() = accumulated;
				return;
			}

			if(ev.type == SDL_EVENT_FINGER_MOTION && prev.type == SDL_EVENT_FINGER_MOTION && ev.tfinger.fingerID == prev.tfinger.fingerID)
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

	while(SDL_PollEvent(&ev))
	{
		// touch events stay normalized and are left alone
		InputSourceMouse::convertToRenderCoordinates(ev);

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
	ENGINE->events().dispatchMouseMoved(Point(0, 0), position);
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
	if(inputModeUsesGestures())
		fingerHandler->hapticFeedback();
}

uint32_t InputHandler::getTicks()
{
	return static_cast<uint32_t>(SDL_GetTicks());
}

bool InputHandler::hasTouchInputDevice() const
{
	return fingerHandler->hasTouchInputDevice();
}

int InputHandler::getNumTouchFingers() const
{
	if(!inputModeUsesGestures())
		return 0;
	return fingerHandler->getNumTouchFingers();
}

void InputHandler::dispatchMainThread(const std::function<void()> & functor)
{
	auto heapFunctor = std::make_unique<std::function<void()>>(functor);

	SDL_Event event;
	event.user.type = SDL_EVENT_USER;
	event.user.code = 0;
	event.user.data1 = nullptr;
	event.user.data2 = nullptr;
	SDL_PushEvent(&event);

	// NOTE: approach with dispatchedTasks container is a bit excessive
	// used mostly to prevent false-positives leaks in analyzers
	dispatchedTasks.push(std::move(heapFunctor));
}

void InputHandler::handleUserEvent(const SDL_UserEvent & current)
{
	std::unique_ptr<std::function<void()>> task;

	if (!dispatchedTasks.try_pop(task))
	{
		logGlobal->error("InputHandler::handleUserEvent received without active task!");
		return;
	}

	(*task)();
}

const Point & InputHandler::getCursorPosition() const
{
	return cursorPosition;
}
