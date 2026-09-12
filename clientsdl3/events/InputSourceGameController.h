/*
* InputSourceGameController.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include "lib/Point.h"
#include "ControllerPromptFamily.h"
#include "gui/Shortcut.h"

/// Class that handles game controller input from SDL events
class InputSourceGameController
{
	struct PressedAxis
	{
		int instanceId;
		std::vector<EShortcut> actions;
	};
	struct PressedButton
	{
		int instanceId;
		std::vector<EShortcut> actions;
	};

	static void gameControllerDeleter(SDL_Gamepad * gameController);
	using GameControllerPtr = std::unique_ptr<SDL_Gamepad, decltype(&gameControllerDeleter)>;

	std::map<int, GameControllerPtr> gameControllerMap;
	std::map<SDL_GamepadAxis, PressedAxis> pressedAxes;
	std::set<std::pair<int, SDL_GamepadAxis>> suppressedAxisReleases;
	std::map<SDL_GamepadButton, PressedButton> pressedButtons;
	std::set<std::pair<int, SDL_GamepadButton>> suppressedButtonReleases;
	int activeController = -1;

	std::chrono::steady_clock::time_point lastCheckTime;
	double cursorAxisValueX;
	double cursorAxisValueY;
	double cursorPlanDisX;
	double cursorPlanDisY;

	bool scrollAxisMoved;
	Point scrollStart;
	Point scrollCurrent;
	double scrollAxisValueX;
	double scrollAxisValueY;
	double scrollPlanDisX;
	double scrollPlanDisY;

	const double configTriggerThreshold;
	const double configAxisDeadZone;
	const double configAxisFullZone;
	const double configAxisSpeed;
	const double configAxisScale;

	void openGameController(SDL_JoystickID instanceID);
	int getJoystickIndex(SDL_Gamepad * controller);
	double getRealAxisValue(int value) const;
	void dispatchAxisShortcuts(const std::vector<EShortcut> & shortcutsVector, int instanceId,
		SDL_GamepadAxis axisID, int axisValue, const std::string & axisName);
	void tryToConvertCursor();
	void doCursorMove(int deltaX, int deltaY);
	int getMoveDis(float planDis);
	void handleCursorUpdate(int32_t deltaTimeMs);
	void handleScrollUpdate(int32_t deltaTimeMs);
	bool isScrollAxisReleased() const;

public:
	InputSourceGameController();
	void setActiveController(int instanceID);
	bool isAxisMotionActive(const SDL_GamepadAxisEvent & axis) const;
	ControllerPrompt::Family getActiveControllerPromptFamily() const;
	void tryOpenAllGameControllers();
	void handleEventDeviceAdded(const SDL_GamepadDeviceEvent & device);
	void handleEventDeviceRemoved(const SDL_GamepadDeviceEvent & device);
	void handleEventDeviceRemapped(const SDL_GamepadDeviceEvent & device);
	void handleEventAxisMotion(const SDL_GamepadAxisEvent & axis);
	void handleEventButtonDown(const SDL_GamepadButtonEvent & button);
	void handleEventButtonUp(const SDL_GamepadButtonEvent & button);
	void handleUpdate();
	void clearAxisMotion();
	void resetControllerInput();
	void cancelControllerPresses();
};
