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
#include <SDL_render.h>
#include <SDL_hints.h>

InputSourceTouch::InputSourceTouch()
	: multifinger(false)
	, isPointerRelativeMode(settings["general"]["userRelativePointer"].Bool())
	, pointerSpeedMultiplier(settings["general"]["relativePointerSpeedMultiplier"].Float())
{
	if(isPointerRelativeMode)
	{
		SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
		SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	}
}

void InputSourceTouch::handleEventFingerMotion(const SDL_TouchFingerEvent & tfinger)
{
	if(isPointerRelativeMode)
	{
		Point screenSize = GH.screenDimensions();

		Point moveDistance {
			static_cast<int>(screenSize.x * pointerSpeedMultiplier * tfinger.dx),
			static_cast<int>(screenSize.y * pointerSpeedMultiplier * tfinger.dy)
		};

		GH.input().moveCursorPosition(moveDistance);
	}
}

void InputSourceTouch::handleEventFingerDown(const SDL_TouchFingerEvent & tfinger)
{
	if(isPointerRelativeMode)
	{
		if(tfinger.x > 0.5)
		{
			bool isRightClick = tfinger.y < 0.5;

			if (isRightClick)
				GH.events().dispatchMouseButtonPressed(MouseButton::RIGHT, GH.getCursorPosition());
			else
				GH.events().dispatchMouseButtonPressed(MouseButton::LEFT, GH.getCursorPosition());
		}
	}
#ifndef VCMI_IOS
	else
	{
		auto fingerCount = SDL_GetNumTouchFingers(tfinger.touchId);
		multifinger = fingerCount > 1;

		if(fingerCount == 2)
		{
			Point position = convertTouchToMouse(tfinger);

			GH.input().setCursorPosition(position);
			GH.events().dispatchMouseButtonPressed(MouseButton::RIGHT, position);
		}
	}
#endif //VCMI_IOS
}

void InputSourceTouch::handleEventFingerUp(const SDL_TouchFingerEvent & tfinger)
{
	if(isPointerRelativeMode)
	{
		if(tfinger.x > 0.5)
		{
			bool isRightClick = tfinger.y < 0.5;

			if (isRightClick)
				GH.events().dispatchMouseButtonReleased(MouseButton::RIGHT, GH.getCursorPosition());
			else
				GH.events().dispatchMouseButtonReleased(MouseButton::LEFT, GH.getCursorPosition());
		}
	}
#ifndef VCMI_IOS
	else
	{
		if(multifinger)
		{
			auto fingerCount = SDL_GetNumTouchFingers(tfinger.touchId);
			Point position = convertTouchToMouse(tfinger);
			GH.input().setCursorPosition(position);
			GH.events().dispatchMouseButtonReleased(MouseButton::RIGHT, position);
			multifinger = fingerCount != 0;
		}
	}
#endif //VCMI_IOS
}

Point InputSourceTouch::convertTouchToMouse(const SDL_TouchFingerEvent & tfinger)
{
	return Point(tfinger.x * GH.screenDimensions().x, tfinger.y * GH.screenDimensions().y);
}

bool InputSourceTouch::isMouseButtonPressed(MouseButton button) const
{
	return false;
}
