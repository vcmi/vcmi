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

#include <tbb/concurrent_queue.h>

enum class EUserEvent;
enum class MouseButton;
union SDL_Event;
struct SDL_UserEvent;

class InputSourceMouse;
class InputSourceKeyboard;
class InputSourceTouch;
class InputSourceText;
class InputSourceGameController;

enum class InputMode
{
	KEYBOARD_AND_MOUSE,
	TOUCH,
	CONTROLLER
};

enum class PowerStateMode
{
	UNKNOWN,
	CHARGING,
	ON_BATTERY
};

struct PowerState {
	PowerStateMode powerState;
	int seconds;
	int percent;
};

class InputHandler
{
	std::vector<SDL_Event> eventsQueue;
	tbb::concurrent_queue<std::unique_ptr<std::function<void()>>> dispatchedTasks;
	std::mutex eventsMutex;

	Point cursorPosition;

	const bool enableMouse;
	const bool enableTouch;
	const bool enableController;

	InputMode currentInputMode;
	void setCurrentInputMode(InputMode modi);

	std::vector<SDL_Event> acquireEvents();

	void preprocessEvent(const SDL_Event & event);
	void handleCurrentEvent(const SDL_Event & current);
	void handleUserEvent(const SDL_UserEvent & current);

	std::unique_ptr<InputSourceMouse> mouseHandler;
	std::unique_ptr<InputSourceKeyboard> keyboardHandler;
	std::unique_ptr<InputSourceTouch> fingerHandler;
	std::unique_ptr<InputSourceText> textHandler;
	std::unique_ptr<InputSourceGameController> gameControllerHandler;

	// Cached power state updated asynchronously via TBB
	std::atomic<int> cachedPowerStateMode;
	std::atomic<int> cachedPowerStateSeconds;
	std::atomic<int> cachedPowerStatePercent;
	uint32_t powerStateFrameCounter;
	void updatePowerState();

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

	/// Moves cursor by specified distance
	void moveCursorPosition(const Point & distance);

	/// Moves cursor to a specified position
	void setCursorPosition(const Point & position);

	/// Initiates text input in selected area, potentially creating IME popup (mobile systems only at the moment)
	void startTextInput(const Rect & where);

	/// Ends any existing text input state
	void stopTextInput();

	/// do a haptic feedback
	void hapticFeedback();

	/// Get the number of milliseconds since SDL library initialization
	uint32_t getTicks();

	/// returns true if system has active touchscreen
	bool hasTouchInputDevice() const;

	/// returns number of fingers on touchscreen
	int getNumTouchFingers() const;

	/// Calls provided functor in main thread on next execution frame
	void dispatchMainThread(const std::function<void()> & functor);

	/// Returns current position of cursor, in VCMI logical screen coordinates
	const Point & getCursorPosition() const;

	/// returns true if chosen keyboard key is currently pressed down
	bool isKeyboardAltDown() const;
	bool isKeyboardCmdDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;

	InputMode getCurrentInputMode();

	void copyToClipBoard(const std::string & text);
	PowerState getPowerState();
};
