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

#include <memory>
#include <SDL.h>

#include "../../lib/Point.h"
#include "../gui/Shortcut.h"


const int AXIS_DEAD_ZOOM = 4000;
const int AXIS_MAX_ZOOM = 32000;
const int AXIS_MOVE_SPEED = 500;
const int AXIS_CURSOR_MOVE_INTERVAL = 1000;
const int TRIGGER_PRESS_THRESHOLD = 8000;


/// Class that handles game controller input from SDL events
class InputSourceGameController
{
    static void gameControllerDeleter(SDL_GameController * gameController);
    using GameControllerPtr = std::unique_ptr<SDL_GameController, decltype(&gameControllerDeleter)>;

    std::map<int, GameControllerPtr> gameControllerMap;
    long long lastCheckTime;
    int axisValueX;
    int axisValueY;
    float planDisX;
    float planDisY;

    void openGameController(int index);
    int getRealAxisValue(int value);
    void dispatchTriggerShortcuts(const std::vector<EShortcut> & shortcutsVector, int axisValue);
    void doCursorMove(int deltaX, int deltaY);
    int getMoveDis(float planDis);

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
