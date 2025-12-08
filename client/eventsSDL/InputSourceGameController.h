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

/// Class that handles game controller input from SDL events
class InputSourceGameController
{
	static void gameControllerDeleter(SDL_GameController * gameController);
	using GameControllerPtr = std::unique_ptr<SDL_GameController, decltype(&gameControllerDeleter)>;

	std::map<int, GameControllerPtr> gameControllerMap;
	std::set<SDL_GameControllerAxis> pressedAxes;

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

	void openGameController(int index);
	int getJoystickIndex(SDL_GameController * controller);
	double getRealAxisValue(int value) const;
	void dispatchAxisShortcuts(const std::vector<EShortcut> & shortcutsVector, SDL_GameControllerAxis axisID, int axisValue, std::string axisName);
	void tryToConvertCursor();
	void doCursorMove(int deltaX, int deltaY);
	int getMoveDis(float planDis);
	void handleCursorUpdate(int32_t deltaTimeMs);
	void handleScrollUpdate(int32_t deltaTimeMs);
	bool isScrollAxisReleased() const;

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
