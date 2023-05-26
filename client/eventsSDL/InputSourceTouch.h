/*
* InputSourceTouch.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Point;
VCMI_LIB_NAMESPACE_END

enum class MouseButton;
struct SDL_TouchFingerEvent;

/// Class that handles touchscreen input from SDL events
class InputSourceTouch
{
	double pointerSpeedMultiplier;
	bool multifinger;
	bool isPointerRelativeMode;

	Point convertTouchToMouse(const SDL_TouchFingerEvent & current);

	void fakeMouseButtonEventRelativeMode(bool down, bool right);

public:
	InputSourceTouch();

	void handleEventFingerMotion(const SDL_TouchFingerEvent & current);
	void handleEventFingerDown(const SDL_TouchFingerEvent & current);
	void handleEventFingerUp(const SDL_TouchFingerEvent & current);

	bool isMouseButtonPressed(MouseButton button) const;
};
