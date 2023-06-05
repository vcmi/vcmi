/*
* InputSourceKeyboard.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

struct SDL_KeyboardEvent;

/// Class that handles keyboard input from SDL events
class InputSourceKeyboard
{
public:
	InputSourceKeyboard();

	void handleEventKeyDown(const SDL_KeyboardEvent & current);
	void handleEventKeyUp(const SDL_KeyboardEvent & current);

	bool isKeyboardAltDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;
};
