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
#include "../render/GpuResources.h"
#include "InputHandler.h"

#include "CMT.h"
#include "GameEngine.h"
#include "gui/EventDispatcher.h"
#include "gui/MouseButton.h"

#include "../render/IScreenHandler.h"

#include "lib/Point.h"
#include "lib/CConfigHandler.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

InputSourceMouse::InputSourceMouse()
	:mouseToleranceDistance(settings["input"]["mouseToleranceDistance"].Integer())
	,motionAccumulatedX(.0f)
	,motionAccumulatedY(.0f)
	,wheelAccumulatedX(.0f)
	,wheelAccumulatedY(.0f)
{
#ifndef VCMI_MAC
	// macOS apps do not pass the click that activates a window to that window
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif
}

void InputSourceMouse::convertToRenderCoordinates(SDL_Event & event)
{
	if(GpuResources::get().renderer() == nullptr)
		return;

	const bool logCoordinates = event.type == SDL_EVENT_MOUSE_MOTION && SDL_GetTicks() % 250 < 16;
	const float windowX = event.motion.x;
	const float windowY = event.motion.y;

	switch(event.type)
	{
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_WHEEL:
			SDL_ConvertEventToRenderCoordinates(GpuResources::get().renderer(), &event);
			break;
	}

	if(logCoordinates)
	{
		SDL_Renderer * renderer = GpuResources::get().renderer();
		SDL_Window * window = SDL_GetRenderWindow(renderer);
		SDL_FRect presentation;
		int windowWidth;
		int windowHeight;
		int outputWidth;
		int outputHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		SDL_GetRenderOutputSize(renderer, &outputWidth, &outputHeight);
		SDL_GetRenderLogicalPresentationRect(renderer, &presentation);
		logGlobal->info("INPUT_DIAG window %.1fx%.1f render %.1fx%.1f windowSize %dx%d output %dx%d presentation %.1f,%.1f %.1fx%.1f", windowX, windowY, event.motion.x, event.motion.y, windowWidth, windowHeight, outputWidth, outputHeight, presentation.x, presentation.y, presentation.w, presentation.h);
	}
}

void InputSourceMouse::handleEventMouseMotion(const SDL_MouseMotionEvent & motion)
{
	Point newPosition = Point(motion.x, motion.y) / ENGINE->screenHandler().getScalingFactor();
	motionAccumulatedX += -motion.xrel / ENGINE->screenHandler().getScalingFactor();
	motionAccumulatedY += -motion.yrel / ENGINE->screenHandler().getScalingFactor();
	Point distance = Point(motionAccumulatedX, motionAccumulatedY);
	motionAccumulatedX -= distance.x;
	motionAccumulatedY -= distance.y;

	mouseButtonsMask = motion.state;

	if (mouseButtonsMask & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE))
		ENGINE->events().dispatchGesturePanning(middleClickPosition, newPosition, distance);
	else if (mouseButtonsMask & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
		ENGINE->events().dispatchMouseDragged(newPosition, distance);
	else if (mouseButtonsMask & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT))
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
	// On Wayland (and some other platforms), SDL delivers smooth/precise scroll events
	// carrying only a fractional value each - accumulate those and dispatch whole steps only
	wheelAccumulatedX += wheel.x;
	wheelAccumulatedY += wheel.y;

	int stepsX = static_cast<int>(wheelAccumulatedX);
	wheelAccumulatedX -= static_cast<float>(stepsX);
	int stepsY = static_cast<int>(wheelAccumulatedY);
	wheelAccumulatedY -= static_cast<float>(stepsY);

	if(stepsX == 0 && stepsY == 0)
		return;

	ENGINE->events().dispatchMouseScrolled(Point(stepsX, stepsY), Point(wheel.mouse_x, wheel.mouse_y) / ENGINE->screenHandler().getScalingFactor());
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
