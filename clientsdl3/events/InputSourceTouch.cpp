/*
* InputSourceTouch.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputSourceTouch.h"

#include "InputHandler.h"

#include "lib/CConfigHandler.h"
#include "gui/CursorHandler.h"
#include "GameEngine.h"
#include "GameEngineUser.h"
#include "gui/EventDispatcher.h"
#include "gui/MouseButton.h"
#include "gui/WindowHandler.h"
#include "../render/IScreenHandler.h"

#if defined(VCMI_ANDROID)
#include "lib/CAndroidVMHelper.h"
#elif defined(VCMI_IOS)
#include "ios/utils.h"
#endif

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_touch.h>

/// SDL3 replaced the indexed finger accessors with a single array query
static int countTouchFingers(SDL_TouchID touchID)
{
	int count = 0;
	SDL_free(SDL_GetTouchFingers(touchID, &count));
	return count;
}

/// SDL3 timestamps events in nanoseconds, while the gesture timings are milliseconds
static uint32_t touchEventTimeMs(const SDL_TouchFingerEvent & tfinger)
{
	return static_cast<uint32_t>(SDL_NS_TO_MS(tfinger.timestamp));
}

InputSourceTouch::InputSourceTouch()
	: lastTapTimeTicks(0), lastLeftClickTimeTicks(0), numTouchFingers(0)
{
	params.useRelativeMode = settings["general"]["userRelativePointer"].Bool();
	params.relativeModeSpeedFactor = settings["general"]["relativePointerSpeedMultiplier"].Float();
	params.longTouchTimeMilliseconds = settings["general"]["longTouchTimeMilliseconds"].Float();
	params.hapticFeedbackEnabled = settings["general"]["hapticFeedback"].Bool();
	params.touchToleranceDistance = settings["input"]["touchToleranceDistance"].Float();

	if (params.useRelativeMode)
		state = TouchState::RELATIVE_MODE;
	else
		state = TouchState::IDLE;

#ifdef VCMI_EMULATE_TOUCHSCREEN_WITH_MOUSE
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#else
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
}

void InputSourceTouch::handleEventFingerMotion(const SDL_TouchFingerEvent & tfinger)
{
	Point screenSize = ENGINE->screenDimensions();

	motionAccumulatedX[tfinger.fingerID] += tfinger.dx;
	motionAccumulatedY[tfinger.fingerID] += tfinger.dy;

	float motionThreshold = 1.0 / std::min(screenSize.x, screenSize.y);
	if(std::abs(motionAccumulatedX[tfinger.fingerID]) < motionThreshold && std::abs(motionAccumulatedY[tfinger.fingerID]) < motionThreshold)
		return;

	int scalingFactor = ENGINE->screenHandler().getScalingFactor();

	if (settings["video"]["cursor"].String() == "software" && state != TouchState::RELATIVE_MODE)
	{
		Point cursorPosition = Point(tfinger.x * screenSize.x, tfinger.y * screenSize.y) * scalingFactor;
		ENGINE->cursor().cursorMove(cursorPosition.x, cursorPosition.y);
	}

	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			Point moveDistance {
				static_cast<int>(screenSize.x * params.relativeModeSpeedFactor * motionAccumulatedX[tfinger.fingerID]),
				static_cast<int>(screenSize.y * params.relativeModeSpeedFactor * motionAccumulatedY[tfinger.fingerID])
			};

			ENGINE->input().moveCursorPosition(moveDistance);
			ENGINE->cursor().cursorMove(ENGINE->getCursorPosition().x * scalingFactor, ENGINE->getCursorPosition().y * scalingFactor);

			break;
		}
		case TouchState::IDLE:
		{
			// no-op, might happen in some edge cases, e.g. when fingerdown event was ignored
			break;
		}
		case TouchState::TAP_DOWN_SHORT:
		case TouchState::TAP_DOWN_LONG_AWAIT:
		{
			Point distance = convertTouchToMouse(tfinger) - lastTapPosition;
			if ( std::abs(distance.x) > params.panningSensitivityThreshold || std::abs(distance.y) > params.panningSensitivityThreshold)
			{
				ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
				state = state == TouchState::TAP_DOWN_SHORT ? TouchState::TAP_DOWN_PANNING : TouchState::TAP_DOWN_PANNING_POPUP;
				ENGINE->events().dispatchGesturePanningStarted(lastTapPosition);
			}
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
		case TouchState::TAP_DOWN_PANNING_POPUP:
		{
			emitPanningEvent(tfinger);
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		{
			emitPinchEvent(tfinger);
			break;
		}
		case TouchState::TAP_DOWN_LONG:
		{
			// no-op
			break;
		}
	}

	if(std::abs(motionAccumulatedX[tfinger.fingerID]) >= motionThreshold)
		motionAccumulatedX[tfinger.fingerID] = 0;
	if(std::abs(motionAccumulatedY[tfinger.fingerID]) >= motionThreshold)
		motionAccumulatedY[tfinger.fingerID] = 0;
}

void InputSourceTouch::handleEventFingerDown(const SDL_TouchFingerEvent & tfinger)
{
	numTouchFingers = countTouchFingers(tfinger.touchID);

	// FIXME: better place to update potentially changed settings?
	params.longTouchTimeMilliseconds = settings["general"]["longTouchTimeMilliseconds"].Float();
	params.hapticFeedbackEnabled = settings["general"]["hapticFeedback"].Bool();

	lastTapTimeTicks = touchEventTimeMs(tfinger);

	if (settings["video"]["cursor"].String() == "software" && state != TouchState::RELATIVE_MODE)
	{
		int scalingFactor = ENGINE->screenHandler().getScalingFactor();
		Point screenSize = ENGINE->screenDimensions();
		Point cursorPosition = Point(tfinger.x * screenSize.x, tfinger.y * screenSize.y) * scalingFactor;
		ENGINE->cursor().cursorMove(cursorPosition.x, cursorPosition.y);
	}

	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			if(tfinger.x > 0.5)
			{
				if (tfinger.y < 0.5)
					ENGINE->events().dispatchShowPopup(ENGINE->getCursorPosition(), params.touchToleranceDistance);
				else
					ENGINE->events().dispatchMouseLeftButtonPressed(ENGINE->getCursorPosition(), params.touchToleranceDistance);
			}
			break;
		}
		case TouchState::IDLE:
		{
			lastTapPosition = convertTouchToMouse(tfinger);
			ENGINE->input().setCursorPosition(lastTapPosition);
			ENGINE->events().dispatchTouchPress(lastTapPosition, true, params.touchToleranceDistance);
			state = TouchState::TAP_DOWN_SHORT;
			break;
		}
		case TouchState::TAP_DOWN_SHORT:
		{
			ENGINE->input().setCursorPosition(convertTouchToMouse(tfinger));
			ENGINE->events().dispatchGesturePanningStarted(lastTapPosition);
			state = TouchState::TAP_DOWN_DOUBLE;
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
		{
			ENGINE->input().setCursorPosition(convertTouchToMouse(tfinger));
			state = TouchState::TAP_DOWN_DOUBLE;
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		{
			ENGINE->user().onGlobalLobbyInterfaceActivated();
			break;
		}
		case TouchState::TAP_DOWN_LONG_AWAIT:
			lastTapPosition = convertTouchToMouse(tfinger);
			break;
		case TouchState::TAP_DOWN_LONG:
		case TouchState::TAP_DOWN_PANNING_POPUP:
		{
			// no-op
			break;
		}
	}
}

void InputSourceTouch::handleEventFingerUp(const SDL_TouchFingerEvent & tfinger)
{
	numTouchFingers = countTouchFingers(tfinger.touchID);

	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			if(tfinger.x > 0.5)
			{
				if (tfinger.y < 0.5)
					ENGINE->events().dispatchClosePopup(ENGINE->getCursorPosition());
				else
					ENGINE->events().dispatchMouseLeftButtonReleased(ENGINE->getCursorPosition(), params.touchToleranceDistance);
			}
			break;
		}
		case TouchState::IDLE:
		{
			// no-op, might happen in some edge cases, e.g. when fingerdown event was ignored
			break;
		}
		case TouchState::TAP_DOWN_SHORT:
		{
			ENGINE->input().setCursorPosition(convertTouchToMouse(tfinger));
			if(touchEventTimeMs(tfinger) - lastLeftClickTimeTicks < params.doubleTouchTimeMilliseconds && (convertTouchToMouse(tfinger) - lastLeftClickPosition).length() < params.doubleTouchToleranceDistance)
			{
				ENGINE->events().dispatchMouseDoubleClick(convertTouchToMouse(tfinger), params.touchToleranceDistance);
				ENGINE->events().dispatchMouseLeftButtonReleased(convertTouchToMouse(tfinger), params.touchToleranceDistance);
			}
			else
			{
				ENGINE->events().dispatchMouseLeftButtonPressed(convertTouchToMouse(tfinger), params.touchToleranceDistance);
				ENGINE->events().dispatchMouseLeftButtonReleased(convertTouchToMouse(tfinger), params.touchToleranceDistance);
				lastLeftClickTimeTicks = touchEventTimeMs(tfinger);
				lastLeftClickPosition = convertTouchToMouse(tfinger);
			}
			ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
			state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
		case TouchState::TAP_DOWN_PANNING_POPUP:
		{
			ENGINE->events().dispatchGesturePanningEnded(lastTapPosition, convertTouchToMouse(tfinger));
			ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
			state = state == TouchState::TAP_DOWN_PANNING ? TouchState::IDLE : TouchState::TAP_DOWN_LONG_AWAIT;
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		{
			if (countTouchFingers(tfinger.touchID) == 1)
			{
				ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
				state = TouchState::TAP_DOWN_PANNING;
			}
			if (countTouchFingers(tfinger.touchID) == 0)
			{
				ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
				ENGINE->events().dispatchGesturePanningEnded(lastTapPosition, convertTouchToMouse(tfinger));
				state = TouchState::IDLE;
			}
			break;
		}
		case TouchState::TAP_DOWN_LONG:
		{
			if (countTouchFingers(tfinger.touchID) == 0)
			{
				state = TouchState::TAP_DOWN_LONG_AWAIT;
			}
			break;
		}
		case TouchState::TAP_DOWN_LONG_AWAIT:
		{
			if (countTouchFingers(tfinger.touchID) == 0)
			{
				ENGINE->input().setCursorPosition(convertTouchToMouse(tfinger));
				ENGINE->events().dispatchClosePopup(convertTouchToMouse(tfinger));
				state = TouchState::IDLE;
			}
			break;
		}
	}
}

void InputSourceTouch::handleUpdate()
{
	if ( state == TouchState::TAP_DOWN_SHORT)
	{
		uint32_t currentTime = static_cast<uint32_t>(SDL_GetTicks());
		if (currentTime > lastTapTimeTicks + params.longTouchTimeMilliseconds)
		{
			ENGINE->events().dispatchTouchPress(lastTapPosition, false, params.touchToleranceDistance);
			ENGINE->events().dispatchShowPopup(ENGINE->getCursorPosition(), params.touchToleranceDistance);

			if (ENGINE->windows().isTopWindowPopup())
			{
				hapticFeedback();
				state = TouchState::TAP_DOWN_LONG;
			}
		}
	}
}

Point InputSourceTouch::convertTouchToMouse(const SDL_TouchFingerEvent & tfinger)
{
	return convertTouchToMouse(tfinger.x, tfinger.y);
}

Point InputSourceTouch::convertTouchToMouse(float x, float y)
{
	return Point(x * ENGINE->screenDimensions().x, y * ENGINE->screenDimensions().y);
}

bool InputSourceTouch::hasTouchInputDevice() const
{
	int count = 0;
	SDL_free(SDL_GetTouchDevices(&count));
	return count > 0;
}

int InputSourceTouch::getNumTouchFingers() const
{
	return numTouchFingers;
}

void InputSourceTouch::emitPanningEvent(const SDL_TouchFingerEvent & tfinger)
{
	Point distance = convertTouchToMouse(-motionAccumulatedX[tfinger.fingerID], -motionAccumulatedY[tfinger.fingerID]);

	ENGINE->events().dispatchGesturePanning(lastTapPosition, convertTouchToMouse(tfinger), distance);
}

void InputSourceTouch::emitPinchEvent(const SDL_TouchFingerEvent & tfinger)
{
	bool otherFingerFound = false;
	double otherX;
	double otherY;

	int fingersCount = 0;
	SDL_Finger ** fingersList = SDL_GetTouchFingers(tfinger.touchID, &fingersCount);

	if (fingersCount < 2)
	{
		SDL_free(fingersList);
		return;
	}

	for (int i = 0; fingersList && i < fingersCount; ++i)
	{
		SDL_Finger * finger = fingersList[i];

		if (finger && finger->id != tfinger.fingerID)
		{
			otherX = finger->x * ENGINE->screenDimensions().x;
			otherY = finger->y * ENGINE->screenDimensions().y;
			otherFingerFound = true;
			break;
		}
	}

	SDL_free(fingersList);

	if (!otherFingerFound)
		return; // should be impossible, but better to avoid weird edge cases

	float thisX = tfinger.x * ENGINE->screenDimensions().x;
	float thisY = tfinger.y * ENGINE->screenDimensions().y;
	float deltaX = motionAccumulatedX[tfinger.fingerID] * ENGINE->screenDimensions().x;
	float deltaY = motionAccumulatedY[tfinger.fingerID] * ENGINE->screenDimensions().y;

	float oldX = thisX - deltaX - otherX;
	float oldY = thisY - deltaY - otherY;
	float newX = thisX - otherX;
	float newY = thisY - otherY;

	double distanceOld = std::sqrt(oldX * oldX + oldY * oldY);
	double distanceNew = std::sqrt(newX * newX + newY * newY);

	if (distanceOld > params.pinchSensitivityThreshold)
		ENGINE->events().dispatchGesturePinch(lastTapPosition, distanceNew / distanceOld);
}

void InputSourceTouch::hapticFeedback() {
	if(params.hapticFeedbackEnabled) {
#if defined(VCMI_ANDROID)
		CAndroidVMHelper vmHelper;
		vmHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "hapticFeedback");
#elif defined(VCMI_IOS)
		iOS_utils::hapticFeedback();
#endif
	}
}
