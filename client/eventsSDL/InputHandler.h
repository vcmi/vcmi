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
	std::vector<SDL_Event> eventsQueue;
	boost::mutex eventsMutex;

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

	/// Fetches events from SDL input system and prepares them for processing
	void fetchEvents();
	/// Performs actual processing and dispatching of previously fetched events
	void processEvents();

	/// drops all incoming events without processing them
	/// returns true if input event has been found
	bool ignoreEventsUntilInput();

	void fakeMoveCursor(float dx, float dy);

	/// Initiates text input in selected area, potentially creating IME popup (mobile systems only at the moment)
	void startTextInput(const Rect & where);

	/// Ends any existing text input state
	void stopTextInput();

	/// Returns true if selected mouse button is pressed at the moment
	bool isMouseButtonPressed(MouseButton button) const;

	/// Generates new user event that will be processed on next frame
	void pushUserEvent(EUserEvent usercode, void * userdata);

	/// Returns current position of cursor, in VCMI logical screen coordinates
	const Point & getCursorPosition() const;

	/// returns true if chosen keyboard key is currently pressed down
	bool isKeyboardAltDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;
};
