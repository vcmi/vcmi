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

struct SDL_MouseWheelEvent;
struct SDL_MouseMotionEvent;
struct SDL_MouseButtonEvent;

/// Class that handles mouse input from SDL events
class InputSourceMouse
{
public:
	void handleEventMouseMotion(const SDL_MouseMotionEvent & current);
	void handleEventMouseButtonDown(const SDL_MouseButtonEvent & current);
	void handleEventMouseWheel(const SDL_MouseWheelEvent & current);
	void handleEventMouseButtonUp(const SDL_MouseButtonEvent & current);
};
