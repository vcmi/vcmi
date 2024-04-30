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

#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include "../../lib/Point.h"
#include "../gui/Shortcut.h"

constexpr int AXIS_DEAD_ZOOM = 6000;
constexpr int AXIS_MAX_ZOOM = 32000;
constexpr int AXIS_MOVE_SPEED = 500;
constexpr int TRIGGER_PRESS_THRESHOLD = 8000;

/// Class that handles game controller input from SDL events
class InputSourceGameController
{
	static void gameControllerDeleter(SDL_GameController * gameController);
	using GameControllerPtr = std::unique_ptr<SDL_GameController, decltype(&gameControllerDeleter)>;

	std::map<int, GameControllerPtr> gameControllerMap;
	std::set<SDL_GameControllerAxis> pressedAxes;

	std::chrono::steady_clock::time_point lastCheckTime;
	int cursorAxisValueX;
	int cursorAxisValueY;
	float cursorPlanDisX;
	float cursorPlanDisY;

	bool scrollAxisMoved;
	Point scrollStart;
	Point scrollCurrent;
	int scrollAxisValueX;
	int scrollAxisValueY;
	float scrollPlanDisX;
	float scrollPlanDisY;

	void openGameController(int index);
	int getJoystickIndex(SDL_GameController * controller);
	int getRealAxisValue(int value);
	void dispatchAxisShortcuts(const std::vector<EShortcut> & shortcutsVector, SDL_GameControllerAxis axisID, int axisValue);
	void tryToConvertCursor();
	void doCursorMove(int deltaX, int deltaY);
	int getMoveDis(float planDis);
	void handleCursorUpdate(int32_t deltaTimeMs);
	void handleScrollUpdate(int32_t deltaTimeMs);
	bool isScrollAxisReleased();

public:
	InputSourceGameController();
	void tryOpenAllGameControllers();
	void handleEventDeviceAdded(const SDL_ControllerDeviceEvent & device);
	void handleEventDeviceRemoved(const SDL_ControllerDeviceEvent & device);
	void handleEventDeviceRemapped(const SDL_ControllerDeviceEvent & device);
	void handleEventAxisMotion(const SDL_ControllerAxisEvent & axis);
	void handleEventButtonDown(const SDL_ControllerButtonEvent & button);
	void handleEventButtonUp(const SDL_ControllerButtonEvent & button);
	void handleUpdate();
};
