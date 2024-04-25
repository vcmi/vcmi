/*
* InputSourceGameController.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputSourceGameController.h"
#include "InputHandler.h"

#include "../CGameInfo.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/ShortcutHandler.h"



void InputSourceGameController::gameControllerDeleter(SDL_GameController * gameController)
{
    if(gameController)
        SDL_GameControllerClose(gameController);
}

InputSourceGameController::InputSourceGameController():
    lastCheckTime(0),
    axisValueX(0),
    axisValueY(0),
    planDisX(0.0),
    planDisY(0.0)
{
    tryOpenAllGameControllers();
}

void InputSourceGameController::tryOpenAllGameControllers()
{
    for(int i = 0; i < SDL_NumJoysticks(); ++i)
        if(SDL_IsGameController(i))
            openGameController(i);
        else
            logGlobal->warn("Joystick %d is an unsupported game controller!", i);
}


void InputSourceGameController::openGameController(int index)
{
    SDL_GameController * controller = SDL_GameControllerOpen(index);
    if(!controller)
    {
        logGlobal->error("Fail to open game controller %d!", index);
        return;
    }
    GameControllerPtr controllerPtr(controller, gameControllerDeleter);

    // Need to save joystick index for event. Joystick index may not be equal to index sometimes.
    int joystickIndex = getJoystickIndex(controllerPtr.get());
    if(joystickIndex < 0)
    {
        logGlobal->error("Fail to get joystick index of game controller %d!", index);
        return;
    }

    if(gameControllerMap.find(joystickIndex) != gameControllerMap.end())
    {
        logGlobal->warn("Game controller with joystick index %d is already opened.", joystickIndex);
        return;
    }

    gameControllerMap.emplace(joystickIndex, std::move(controllerPtr));
}

int InputSourceGameController::getJoystickIndex(SDL_GameController * controller)
{
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
    if(!joystick)
        return -1;

    SDL_JoystickID instanceID = SDL_JoystickInstanceID(joystick);
    if(instanceID < 0)
        return -1;
    return (int)instanceID;
}

void InputSourceGameController::handleEventDeviceAdded(const SDL_ControllerDeviceEvent & device)
{
    if(gameControllerMap.find(device.which) != gameControllerMap.end())
    {
        logGlobal->warn("Game controller %d is already opened.", device.which);
        return;
    }
    openGameController(device.which);
}

void InputSourceGameController::handleEventDeviceRemoved(const SDL_ControllerDeviceEvent & device)
{
    if(gameControllerMap.find(device.which) == gameControllerMap.end())
    {
        logGlobal->warn("Game controller %d is not opened before.", device.which);
        return;
    }
    gameControllerMap.erase(device.which);
}

void InputSourceGameController::handleEventDeviceRemapped(const SDL_ControllerDeviceEvent & device)
{
    if(gameControllerMap.find(device.which) == gameControllerMap.end())
    {
        logGlobal->warn("Game controller %d is not opened.", device.which);
        return;
    }
    gameControllerMap.erase(device.which);
    openGameController(device.which);
}

int InputSourceGameController::getRealAxisValue(int value)
{
    if(value < AXIS_DEAD_ZOOM && value > -AXIS_DEAD_ZOOM)
        return 0;
    if(value > AXIS_MAX_ZOOM)
        return AXIS_MAX_ZOOM;
    if(value < -AXIS_MAX_ZOOM)
        return -AXIS_MAX_ZOOM;
    int base = value > 0 ? AXIS_DEAD_ZOOM: -AXIS_DEAD_ZOOM;
    return (value - base) * AXIS_MAX_ZOOM / (AXIS_MAX_ZOOM - AXIS_DEAD_ZOOM);
}

void InputSourceGameController::dispatchTriggerShortcuts(const std::vector<EShortcut> & shortcutsVector, int axisValue)
{
    if(axisValue >= TRIGGER_PRESS_THRESHOLD)
        GH.events().dispatchShortcutPressed(shortcutsVector);
    else
        GH.events().dispatchShortcutReleased(shortcutsVector);
}

void InputSourceGameController::dispatchTriggerLeftClick(int axisValue)
{
    const Point & position = GH.input().getCursorPosition();
    if(axisValue >= TRIGGER_PRESS_THRESHOLD)
        GH.events().dispatchMouseLeftButtonPressed(position, 0);
    else
        GH.events().dispatchMouseLeftButtonReleased(position, 0);
}

void InputSourceGameController::dispatchTriggerRightClick(int axisValue)
{
    const Point & position = GH.input().getCursorPosition();
    if(axisValue >= TRIGGER_PRESS_THRESHOLD)
        GH.events().dispatchShowPopup(position, 0);
    else
        GH.events().dispatchClosePopup(position);
}

void InputSourceGameController::handleEventAxisMotion(const SDL_ControllerAxisEvent & axis)
{
    if(axis.axis == SDL_CONTROLLER_AXIS_LEFTX)
    {
        if(config.getLeftAxisType() == AxisType::CURSOR_MOTION)
            axisValueX = getRealAxisValue(axis.value);
    }
    else if(axis.axis == SDL_CONTROLLER_AXIS_LEFTY)
    {
        if(config.getLeftAxisType() == AxisType::CURSOR_MOTION)
            axisValueY = getRealAxisValue(axis.value);
    }
    if(axis.axis == SDL_CONTROLLER_AXIS_RIGHTX)
    {
        if(config.getRightAxisType() == AxisType::CURSOR_MOTION)
            axisValueX = getRealAxisValue(axis.value);
    }
    else if(axis.axis == SDL_CONTROLLER_AXIS_RIGHTY)
    {
        if(config.getRightAxisType() == AxisType::CURSOR_MOTION)
            axisValueY = getRealAxisValue(axis.value);
    }
    else if(config.isLeftClickTrigger(axis.axis))
    {
        dispatchTriggerLeftClick(axis.value);
    }
    else if(config.isRightClickTrigger(axis.axis))
    {
        dispatchTriggerRightClick(axis.value);
    }
    else if(config.isShortcutsTrigger(axis.axis))
    {
        const auto & shortcutsVector = config.getTriggerShortcuts(axis.axis);
        dispatchTriggerShortcuts(shortcutsVector, axis.value);
    }
}

void InputSourceGameController::handleEventButtonDown(const SDL_ControllerButtonEvent & button)
{
    const Point & position = GH.input().getCursorPosition();

    if(config.isLeftClickButton(button.button))
    {
        GH.events().dispatchMouseLeftButtonPressed(position, 0);
    }

    if(config.isRightClickButton(button.button))
    {
        GH.events().dispatchShowPopup(position, 0);
    }

    if(config.isShortcutsButton(button.button))
    {
        const auto & shortcutsVector = config.getButtonShortcuts(button.button);
        GH.events().dispatchShortcutPressed(shortcutsVector);
    }
}

void InputSourceGameController::handleEventButtonUp(const SDL_ControllerButtonEvent & button)
{
    const Point & position = GH.input().getCursorPosition();

    if(config.isLeftClickButton(button.button))
    {
        GH.events().dispatchMouseLeftButtonReleased(position, 0);
    }
    if(config.isRightClickButton(button.button))
    {
        GH.events().dispatchClosePopup(position);
    }
    if(config.isShortcutsButton(button.button))
    {
        const auto & shortcutsVector = config.getButtonShortcuts(button.button);
        GH.events().dispatchShortcutReleased(shortcutsVector);
    }
}

void InputSourceGameController::doCursorMove(int deltaX, int deltaY)
{
    if(deltaX == 0 && deltaY == 0)
        return;
    const Point & screenSize = GH.screenDimensions();

    // if joystick is connected when intro video plays, using hardware position will move cursor to the right position.
    bool isHardwareCursor = CCS && CCS->curh && CCS->curh->getShowType() == Cursor::ShowType::HARDWARE;
    const Point & cursorPosition = isHardwareCursor ? CCS->curh->getCursorPosition() : GH.getCursorPosition();
    int newX = std::min(std::max(cursorPosition.x + deltaX, 0), screenSize.x);
    int newY = std::min(std::max(cursorPosition.y + deltaY, 0), screenSize.y);
    Point targetPosition{newX, newY};
    GH.input().setCursorPosition(targetPosition);
    if(CCS && CCS->curh) {
        if(CCS->curh->getShowType() == Cursor::ShowType::HARDWARE)
            CCS->curh->ChangeCursor(Cursor::ShowType::SOFTWARE);
        CCS->curh->cursorMove(GH.getCursorPosition().x, GH.getCursorPosition().y);
    }
}

int InputSourceGameController::getMoveDis(float planDis)
{
    if(planDis >= 0)
        return std::floor(planDis);
    else
        return std::ceil(planDis);
}

void InputSourceGameController::handleUpdate()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    if(lastCheckTime == 0)
    {
        lastCheckTime = nowMs;
        return;
    }

    long long deltaTime = nowMs - lastCheckTime;

    if(axisValueX == 0)
        planDisX = 0;
    else
        planDisX += ((float)deltaTime / 1000) * ((float)axisValueX / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;

    if(axisValueY == 0)
        planDisY = 0;
    else
        planDisY += ((float)deltaTime / 1000) * ((float)axisValueY / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;

    int moveDisX = getMoveDis(planDisX);
    int moveDisY = getMoveDis(planDisY);
    planDisX -= moveDisX;
    planDisY -= moveDisY;
    doCursorMove(moveDisX, moveDisY);
    lastCheckTime = nowMs;
}