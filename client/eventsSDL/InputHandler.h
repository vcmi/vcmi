/*
* InputHandler.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../lib/Rect.h"

enum class EUserEvent;
enum class MouseButton;
union SDL_Event;

class InputSourceMouse;
class InputSourceKeyboard;
class InputSourceTouch;
class InputSourceText;
class UserEventHandler;

class InputHandler
{
	Point cursorPosition;
	float pointerSpeedMultiplier;
	int mouseButtonsMask;

	void preprocessEvent(const SDL_Event & event);
	void handleCurrentEvent(const SDL_Event & current);

	std::unique_ptr<InputSourceMouse> mouseHandler;
	std::unique_ptr<InputSourceKeyboard> keyboardHandler;
	std::unique_ptr<InputSourceTouch> fingerHandler;
	std::unique_ptr<InputSourceText> textHandler;
	std::unique_ptr<UserEventHandler> userHandler;

public:
	InputHandler();
	~InputHandler();

	void fetchEvents();
	void processEvents();

	void fakeMoveCursor(float dx, float dy);
	void startTextInput(const Rect & where);
	void stopTextInput();
	bool isMouseButtonPressed(MouseButton button) const;
	void pushUserEvent(EUserEvent usercode, void * userdata);

	const Point & getCursorPosition() const;

	/// returns true if chosen keyboard key is currently pressed down
	bool isKeyboardAltDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;
};
