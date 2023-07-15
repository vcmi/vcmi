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

#include "../../lib/CConfigHandler.h"
#include "../CMT.h"
#include "../CGameInfo.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/MouseButton.h"
#include "../gui/WindowHandler.h"

#if defined(VCMI_ANDROID)
#include "../../lib/CAndroidVMHelper.h"
#elif defined(VCMI_IOS)
#include "../ios/utils.h"
#endif

#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_timer.h>

InputSourceTouch::InputSourceTouch()
	: lastTapTimeTicks(0)
{
	params.useRelativeMode = settings["general"]["userRelativePointer"].Bool();
	params.relativeModeSpeedFactor = settings["general"]["relativePointerSpeedMultiplier"].Float();
	params.longTouchTimeMilliseconds = settings["general"]["longTouchTimeMilliseconds"].Float();
	params.hapticFeedbackEnabled = settings["general"]["hapticFeedback"].Bool();

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
	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			Point screenSize = GH.screenDimensions();

			Point moveDistance {
				static_cast<int>(screenSize.x * params.relativeModeSpeedFactor * tfinger.dx),
				static_cast<int>(screenSize.y * params.relativeModeSpeedFactor * tfinger.dy)
			};

			GH.input().moveCursorPosition(moveDistance);
			if (CCS && CCS->curh)
				CCS->curh->cursorMove(GH.getCursorPosition().x, GH.getCursorPosition().y);

			break;
		}
		case TouchState::IDLE:
		{
			// no-op, might happen in some edge cases, e.g. when fingerdown event was ignored
			break;
		}
		case TouchState::TAP_DOWN_SHORT:
		{
			Point distance = convertTouchToMouse(tfinger) - lastTapPosition;
			if ( std::abs(distance.x) > params.panningSensitivityThreshold || std::abs(distance.y) > params.panningSensitivityThreshold)
				state = TouchState::TAP_DOWN_PANNING;
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
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
		case TouchState::TAP_DOWN_LONG_AWAIT:
		{
			// no-op
			break;
		}
	}
}

void InputSourceTouch::handleEventFingerDown(const SDL_TouchFingerEvent & tfinger)
{
	// FIXME: better place to update potentially changed settings?
	params.longTouchTimeMilliseconds = settings["general"]["longTouchTimeMilliseconds"].Float();
	params.hapticFeedbackEnabled = settings["general"]["hapticFeedback"].Bool();

	lastTapTimeTicks = tfinger.timestamp;

	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			if(tfinger.x > 0.5)
			{
				if (tfinger.y < 0.5)
					GH.events().dispatchShowPopup(GH.getCursorPosition());
				else
					GH.events().dispatchMouseLeftButtonPressed(GH.getCursorPosition());
			}
			break;
		}
		case TouchState::IDLE:
		{
			lastTapPosition = convertTouchToMouse(tfinger);
			GH.input().setCursorPosition(lastTapPosition);
			GH.events().dispatchGesturePanningStarted(lastTapPosition);
			state = TouchState::TAP_DOWN_SHORT;
			break;
		}
		case TouchState::TAP_DOWN_SHORT:
		case TouchState::TAP_DOWN_PANNING:
		{
			GH.input().setCursorPosition(convertTouchToMouse(tfinger));
			state = TouchState::TAP_DOWN_DOUBLE;
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		case TouchState::TAP_DOWN_LONG:
		case TouchState::TAP_DOWN_LONG_AWAIT:
		{
			// no-op
			break;
		}
	}
}

void InputSourceTouch::handleEventFingerUp(const SDL_TouchFingerEvent & tfinger)
{
	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			if(tfinger.x > 0.5)
			{
				if (tfinger.y < 0.5)
					GH.events().dispatchClosePopup(GH.getCursorPosition());
				else
					GH.events().dispatchMouseLeftButtonReleased(GH.getCursorPosition());
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
			GH.input().setCursorPosition(convertTouchToMouse(tfinger));
			GH.events().dispatchMouseLeftButtonPressed(convertTouchToMouse(tfinger));
			GH.events().dispatchMouseLeftButtonReleased(convertTouchToMouse(tfinger));
			state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
		{
			GH.events().dispatchGesturePanningEnded(lastTapPosition, convertTouchToMouse(tfinger));
			state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		{
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 1)
				state = TouchState::TAP_DOWN_PANNING;
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 0)
			{
				GH.events().dispatchGesturePanningEnded(lastTapPosition, convertTouchToMouse(tfinger));
				state = TouchState::IDLE;
			}
			break;
		}
		case TouchState::TAP_DOWN_LONG:
		{
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 0)
			{
				state = TouchState::TAP_DOWN_LONG_AWAIT;
			}
			break;
		}
		case TouchState::TAP_DOWN_LONG_AWAIT:
		{
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 0)
			{
				GH.input().setCursorPosition(convertTouchToMouse(tfinger));
				GH.events().dispatchClosePopup(convertTouchToMouse(tfinger));
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
		uint32_t currentTime = SDL_GetTicks();
		if (currentTime > lastTapTimeTicks + params.longTouchTimeMilliseconds)
		{
			GH.events().dispatchShowPopup(GH.getCursorPosition());

			if (GH.windows().isTopWindowPopup())
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
	return Point(x * GH.screenDimensions().x, y * GH.screenDimensions().y);
}

bool InputSourceTouch::hasTouchInputDevice() const
{
	return SDL_GetNumTouchDevices() > 0;
}

void InputSourceTouch::emitPanningEvent(const SDL_TouchFingerEvent & tfinger)
{
	Point distance = convertTouchToMouse(-tfinger.dx, -tfinger.dy);

	GH.events().dispatchGesturePanning(lastTapPosition, convertTouchToMouse(tfinger), distance);
}

void InputSourceTouch::emitPinchEvent(const SDL_TouchFingerEvent & tfinger)
{
	int fingers = SDL_GetNumTouchFingers(tfinger.touchId);

	if (fingers < 2)
		return;

	bool otherFingerFound = false;
	double otherX;
	double otherY;

	for (int i = 0; i < fingers; ++i)
	{
		SDL_Finger * finger = SDL_GetTouchFinger(tfinger.touchId, i);

		if (finger && finger->id != tfinger.fingerId)
		{
			otherX = finger->x * GH.screenDimensions().x;
			otherY = finger->y * GH.screenDimensions().y;
			otherFingerFound = true;
			break;
		}
	}

	if (!otherFingerFound)
		return; // should be impossible, but better to avoid weird edge cases

	float thisX = tfinger.x * GH.screenDimensions().x;
	float thisY = tfinger.y * GH.screenDimensions().y;
	float deltaX = tfinger.dx * GH.screenDimensions().x;
	float deltaY = tfinger.dy * GH.screenDimensions().y;

	float oldX = thisX - deltaX - otherX;
	float oldY = thisY - deltaY - otherY;
	float newX = thisX - otherX;
	float newY = thisY - otherY;

	double distanceOld = std::sqrt(oldX * oldX + oldY + oldY);
	double distanceNew = std::sqrt(newX * newX + newY + newY);

	if (distanceOld > params.pinchSensitivityThreshold)
		GH.events().dispatchGesturePinch(lastTapPosition, distanceNew / distanceOld);
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
