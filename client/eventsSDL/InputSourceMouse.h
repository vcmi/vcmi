/*
* InputSourceMouse.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../../lib/Point.h"

struct SDL_MouseWheelEvent;
struct SDL_MouseMotionEvent;
struct SDL_MouseButtonEvent;

enum class MouseButton;

/// Class that handles mouse input from SDL events
class InputSourceMouse
{
	Point middleClickPosition;
	int mouseButtonsMask = 0;
public:
	void handleEventMouseMotion(const SDL_MouseMotionEvent & current);
	void handleEventMouseButtonDown(const SDL_MouseButtonEvent & current);
	void handleEventMouseWheel(const SDL_MouseWheelEvent & current);
	void handleEventMouseButtonUp(const SDL_MouseButtonEvent & current);
};
