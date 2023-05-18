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

#include <SDL_events.h>
#include <SDL_render.h>

InputSourceTouch::InputSourceTouch()
	: multifinger(false)
	, isPointerRelativeMode(settings["general"]["userRelativePointer"].Bool())
{
}

void InputSourceTouch::handleEventFingerMotion(const SDL_TouchFingerEvent & tfinger)
{
	if(isPointerRelativeMode)
	{
		GH.input().fakeMoveCursor(tfinger.dx, tfinger.dy);
	}
}

void InputSourceTouch::handleEventFingerDown(const SDL_TouchFingerEvent & tfinger)
{
	auto fingerCount = SDL_GetNumTouchFingers(tfinger.touchId);

	multifinger = fingerCount > 1;

	if(isPointerRelativeMode)
	{
		if(tfinger.x > 0.5)
		{
			bool isRightClick = tfinger.y < 0.5;

			fakeMouseButtonEventRelativeMode(true, isRightClick);
		}
	}
#ifndef VCMI_IOS
	else if(fingerCount == 2)
	{
		Point position = convertTouchToMouse(tfinger);

		GH.events().dispatchMouseMoved(position);
		GH.events().dispatchMouseButtonPressed(MouseButton::RIGHT, position);
	}
#endif //VCMI_IOS
}

void InputSourceTouch::handleEventFingerUp(const SDL_TouchFingerEvent & tfinger)
{
#ifndef VCMI_IOS
	auto fingerCount = SDL_GetNumTouchFingers(tfinger.touchId);
#endif //VCMI_IOS

	if(isPointerRelativeMode)
	{
		if(tfinger.x > 0.5)
		{
			bool isRightClick = tfinger.y < 0.5;

			fakeMouseButtonEventRelativeMode(false, isRightClick);
		}
	}
#ifndef VCMI_IOS
	else if(multifinger)
	{
		Point position = convertTouchToMouse(tfinger);
		GH.events().dispatchMouseMoved(position);
		GH.events().dispatchMouseButtonReleased(MouseButton::RIGHT, position);
		multifinger = fingerCount != 0;
	}
#endif //VCMI_IOS
}

Point InputSourceTouch::convertTouchToMouse(const SDL_TouchFingerEvent & tfinger)
{
	return Point(tfinger.x * GH.screenDimensions().x, tfinger.y * GH.screenDimensions().y);
}

void InputSourceTouch::fakeMouseButtonEventRelativeMode(bool down, bool right)
{
	SDL_Event event;
	SDL_MouseButtonEvent sme = {SDL_MOUSEBUTTONDOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if(!down)
	{
		sme.type = SDL_MOUSEBUTTONUP;
	}

	sme.button = right ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;

	sme.x = GH.getCursorPosition().x;
	sme.y = GH.getCursorPosition().y;

	float xScale, yScale;
	int w, h, rLogicalWidth, rLogicalHeight;

	SDL_GetWindowSize(mainWindow, &w, &h);
	SDL_RenderGetLogicalSize(mainRenderer, &rLogicalWidth, &rLogicalHeight);
	SDL_RenderGetScale(mainRenderer, &xScale, &yScale);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	moveCursorToPosition(Point((int)(sme.x * xScale) + (w - rLogicalWidth * xScale) / 2, (int)(sme.y * yScale + (h - rLogicalHeight * yScale) / 2)));
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

	event.button = sme;
	SDL_PushEvent(&event);
}

void InputSourceTouch::moveCursorToPosition(const Point & position)
{
	SDL_WarpMouseInWindow(mainWindow, position.x, position.y);
}
