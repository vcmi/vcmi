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
#include "../gui/CGuiHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/MouseButton.h"

#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_timer.h>

InputSourceTouch::InputSourceTouch()
	: lastTapTimeTicks(0)
{
	params.useRelativeMode = settings["general"]["userRelativePointer"].Bool();
	params.relativeModeSpeedFactor = settings["general"]["relativePointerSpeedMultiplier"].Float();

	if (params.useRelativeMode)
		state = TouchState::RELATIVE_MODE;
	else
		state = TouchState::IDLE;

	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
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

			Point distance = GH.getCursorPosition() - lastTapPosition;
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
		{
			// no-op
			break;
		}
	}
}

void InputSourceTouch::handleEventFingerDown(const SDL_TouchFingerEvent & tfinger)
{
	lastTapTimeTicks = tfinger.timestamp;

	switch(state)
	{
		case TouchState::RELATIVE_MODE:
		{
			if(tfinger.x > 0.5)
			{
				MouseButton button =  tfinger.y < 0.5 ? MouseButton::RIGHT : MouseButton::LEFT;
				GH.events().dispatchMouseButtonPressed(button, GH.getCursorPosition());
			}
			break;
		}
		case TouchState::IDLE:
		{
			GH.input().setCursorPosition(convertTouchToMouse(tfinger));
			lastTapPosition = GH.getCursorPosition();
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
				MouseButton button =  tfinger.y < 0.5 ? MouseButton::RIGHT : MouseButton::LEFT;
				GH.events().dispatchMouseButtonReleased(button, GH.getCursorPosition());
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
			GH.events().dispatchMouseButtonPressed(MouseButton::LEFT, convertTouchToMouse(tfinger));
			GH.events().dispatchMouseButtonReleased(MouseButton::LEFT, convertTouchToMouse(tfinger));
			state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_PANNING:
		{
			state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_DOUBLE:
		{
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 1)
				state = TouchState::TAP_DOWN_PANNING;
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 0)
				state = TouchState::IDLE;
			break;
		}
		case TouchState::TAP_DOWN_LONG:
		{
			if (SDL_GetNumTouchFingers(tfinger.touchId) == 0)
			{
				GH.input().setCursorPosition(convertTouchToMouse(tfinger));
				GH.events().dispatchMouseButtonReleased(MouseButton::RIGHT, convertTouchToMouse(tfinger));
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
		if (currentTime > lastTapTimeTicks + params.longPressTimeMilliseconds)
		{
			state = TouchState::TAP_DOWN_LONG;
			GH.events().dispatchMouseButtonPressed(MouseButton::RIGHT, GH.getCursorPosition());
		}
	}
}

Point InputSourceTouch::convertTouchToMouse(const SDL_TouchFingerEvent & tfinger)
{
	return Point(tfinger.x * GH.screenDimensions().x, tfinger.y * GH.screenDimensions().y);
}

bool InputSourceTouch::isMouseButtonPressed(MouseButton button) const
{
	if (state == TouchState::TAP_DOWN_LONG)
	{
		if (button == MouseButton::RIGHT)
			return true;
	}

	return false;
}

void InputSourceTouch::emitPanningEvent(const SDL_TouchFingerEvent & tfinger)
{
	Point distance(-tfinger.dx * GH.screenDimensions().x, -tfinger.dy * GH.screenDimensions().y);

	GH.events().dispatchGesturePanning(distance);
}

void InputSourceTouch::emitPinchEvent(const SDL_TouchFingerEvent & tfinger)
{
	// TODO
}
