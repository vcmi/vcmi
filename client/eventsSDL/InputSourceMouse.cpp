/*
* InputSourceMouse.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputSourceMouse.h"
#include "InputHandler.h"

#include "../GameEngine.h"
#include "../gui/EventDispatcher.h"
#include "../gui/MouseButton.h"

#include "../render/IScreenHandler.h"

#include "../../lib/Point.h"
#include "../../lib/CConfigHandler.h"

#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_version.h>

InputSourceMouse::InputSourceMouse()
	:mouseToleranceDistance(settings["input"]["mouseToleranceDistance"].Integer())
	,motionAccumulatedX(.0f)
	,motionAccumulatedY(.0f)
{
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
}

void InputSourceMouse::handleEventMouseMotion(const SDL_MouseMotionEvent & motion)
{
	Point newPosition = Point(motion.x, motion.y) / ENGINE->screenHandler().getScalingFactor();
	motionAccumulatedX += static_cast<float>(-motion.xrel) / ENGINE->screenHandler().getScalingFactor();
	motionAccumulatedY += static_cast<float>(-motion.yrel) / ENGINE->screenHandler().getScalingFactor();
	Point distance = Point(motionAccumulatedX, motionAccumulatedY);
	motionAccumulatedX -= distance.x;
	motionAccumulatedY -= distance.y;

	mouseButtonsMask = motion.state;

	if (mouseButtonsMask & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		ENGINE->events().dispatchGesturePanning(middleClickPosition, newPosition, distance);
	else if (mouseButtonsMask & SDL_BUTTON(SDL_BUTTON_LEFT))
		ENGINE->events().dispatchMouseDragged(newPosition, distance);
	else if (mouseButtonsMask & SDL_BUTTON(SDL_BUTTON_RIGHT))
		ENGINE->events().dispatchMouseDraggedPopup(newPosition, distance);
	else
		ENGINE->input().setCursorPosition(newPosition);
}

void InputSourceMouse::handleEventMouseButtonDown(const SDL_MouseButtonEvent & button)
{
	Point position = Point(button.x, button.y) / ENGINE->screenHandler().getScalingFactor();

	switch(button.button)
	{
		case SDL_BUTTON_LEFT:
			if(button.clicks > 1)
				ENGINE->events().dispatchMouseDoubleClick(position, mouseToleranceDistance);
			else
				ENGINE->events().dispatchMouseLeftButtonPressed(position, mouseToleranceDistance);
			break;
		case SDL_BUTTON_RIGHT:
			ENGINE->events().dispatchShowPopup(position, mouseToleranceDistance);
			break;
		case SDL_BUTTON_MIDDLE:
			middleClickPosition = position;
			ENGINE->events().dispatchGesturePanningStarted(position);
			break;
	}
}

void InputSourceMouse::handleEventMouseWheel(const SDL_MouseWheelEvent & wheel)
{
	//NOTE: while mouseX / mouseY properties are available since 2.26.0, they are not converted into logical coordinates so don't account for resolution scaling
	// This SDL bug was fixed in 2.30.1: https://github.com/libsdl-org/SDL/issues/9097
#if SDL_VERSION_ATLEAST(2,30,1)
	ENGINE->events().dispatchMouseScrolled(Point(wheel.x, wheel.y), Point(wheel.mouseX, wheel.mouseY) / ENGINE->screenHandler().getScalingFactor());
#else
	ENGINE->events().dispatchMouseScrolled(Point(wheel.x, wheel.y), ENGINE->getCursorPosition());
#endif
}

void InputSourceMouse::handleEventMouseButtonUp(const SDL_MouseButtonEvent & button)
{
	Point position = Point(button.x, button.y) / ENGINE->screenHandler().getScalingFactor();

	switch(button.button)
	{
		case SDL_BUTTON_LEFT:
			ENGINE->events().dispatchMouseLeftButtonReleased(position, mouseToleranceDistance);
			break;
		case SDL_BUTTON_RIGHT:
			ENGINE->events().dispatchClosePopup(position);
			break;
		case SDL_BUTTON_MIDDLE:
			ENGINE->events().dispatchGesturePanningEnded(middleClickPosition, position);
			break;
	}
}
