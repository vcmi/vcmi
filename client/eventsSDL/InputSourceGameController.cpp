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
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/EventDispatcher.h"
#include "../gui/ShortcutHandler.h"

void InputSourceGameController::gameControllerDeleter(SDL_GameController * gameController)
{
	if(gameController)
		SDL_GameControllerClose(gameController);
}

InputSourceGameController::InputSourceGameController():
	cursorAxisValueX(0),
	cursorAxisValueY(0),
	cursorPlanDisX(0.0),
	cursorPlanDisY(0.0),
	scrollAxisMoved(false),
	scrollStart(Point(0,0)),
	scrollCurrent(Point(0,0)),
	scrollAxisValueX(0),
	scrollAxisValueY(0),
	scrollPlanDisX(0.0),
	scrollPlanDisY(0.0)
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
	SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controller);
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
	int base = value > 0 ? AXIS_DEAD_ZOOM : -AXIS_DEAD_ZOOM;
	return (value - base) * AXIS_MAX_ZOOM / (AXIS_MAX_ZOOM - AXIS_DEAD_ZOOM);
}

void InputSourceGameController::dispatchAxisShortcuts(const std::vector<EShortcut> & shortcutsVector, SDL_GameControllerAxis axisID, int axisValue)
{
	if(axisValue >= TRIGGER_PRESS_THRESHOLD)
	{
		if(!pressedAxes.count(axisID))
		{
			GH.events().dispatchShortcutPressed(shortcutsVector);
			pressedAxes.insert(axisID);
		}
	}
	else
	{
		if(pressedAxes.count(axisID))
		{
			GH.events().dispatchShortcutReleased(shortcutsVector);
			pressedAxes.erase(axisID);
		}
	}
}

void InputSourceGameController::handleEventAxisMotion(const SDL_ControllerAxisEvent & axis)
{
	tryToConvertCursor();

	SDL_GameControllerAxis axisID = static_cast<SDL_GameControllerAxis>(axis.axis);
	std::string axisName = SDL_GameControllerGetStringForAxis(axisID);

	auto axisActions = GH.shortcuts().translateJoystickAxis(axisName);
	auto buttonActions = GH.shortcuts().translateJoystickButton(axisName);

	for(const auto & action : axisActions)
	{
		switch(action)
		{
			case EShortcut::MOUSE_CURSOR_X:
				cursorAxisValueX = getRealAxisValue(axis.value);
				break;
			case EShortcut::MOUSE_CURSOR_Y:
				cursorAxisValueY = getRealAxisValue(axis.value);
				break;
			case EShortcut::MOUSE_SWIPE_X:
				scrollAxisValueX = getRealAxisValue(axis.value);
				break;
			case EShortcut::MOUSE_SWIPE_Y:
				scrollAxisValueY = getRealAxisValue(axis.value);
				break;
		}
	}

	dispatchAxisShortcuts(buttonActions, axisID, axis.value);
}

void InputSourceGameController::tryToConvertCursor()
{
	assert(CCS);
	assert(CCS->curh);
	if(CCS->curh->getShowType() == Cursor::ShowType::HARDWARE)
	{
		const Point & cursorPosition = GH.getCursorPosition();
		CCS->curh->ChangeCursor(Cursor::ShowType::SOFTWARE);
		CCS->curh->cursorMove(cursorPosition.x, cursorPosition.y);
		GH.input().setCursorPosition(cursorPosition);
	}
}

void InputSourceGameController::handleEventButtonDown(const SDL_ControllerButtonEvent & button)
{
	std::string buttonName = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button.button));
	const auto & shortcutsVector = GH.shortcuts().translateJoystickButton(buttonName);
	GH.events().dispatchShortcutPressed(shortcutsVector);
}

void InputSourceGameController::handleEventButtonUp(const SDL_ControllerButtonEvent & button)
{
	std::string buttonName = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button.button));
	const auto & shortcutsVector = GH.shortcuts().translateJoystickButton(buttonName);
	GH.events().dispatchShortcutReleased(shortcutsVector);
}

void InputSourceGameController::doCursorMove(int deltaX, int deltaY)
{
	if(deltaX == 0 && deltaY == 0)
		return;
	const Point & screenSize = GH.screenDimensions();
	const Point & cursorPosition = GH.getCursorPosition();
	int newX = std::min(std::max(cursorPosition.x + deltaX, 0), screenSize.x);
	int newY = std::min(std::max(cursorPosition.y + deltaY, 0), screenSize.y);
	Point targetPosition{newX, newY};
	GH.input().setCursorPosition(targetPosition);
	if(CCS && CCS->curh)
		CCS->curh->cursorMove(GH.getCursorPosition().x, GH.getCursorPosition().y);
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
	std::chrono::steady_clock::time_point nowMs = std::chrono::steady_clock::now();

	if(lastCheckTime == std::chrono::steady_clock::time_point())
	{
		lastCheckTime = nowMs;
		return;
	}

	int32_t deltaTime = std::chrono::duration_cast<std::chrono::seconds>(nowMs - lastCheckTime).count();
	handleCursorUpdate(deltaTime);
	handleScrollUpdate(deltaTime);
	lastCheckTime = nowMs;
}

void InputSourceGameController::handleCursorUpdate(int32_t deltaTimeMs)
{
	if(cursorAxisValueX == 0)
		cursorPlanDisX = 0;
	else
		cursorPlanDisX += ((float)deltaTimeMs / 1000) * ((float)cursorAxisValueX / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;

	if(cursorAxisValueY == 0)
		cursorPlanDisY = 0;
	else
		cursorPlanDisY += ((float)deltaTimeMs / 1000) * ((float)cursorAxisValueY / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;

	int moveDisX = getMoveDis(cursorPlanDisX);
	int moveDisY = getMoveDis(cursorPlanDisY);
	cursorPlanDisX -= moveDisX;
	cursorPlanDisY -= moveDisY;
	doCursorMove(moveDisX, moveDisY);
}

void InputSourceGameController::handleScrollUpdate(int32_t deltaTimeMs)
{
	if(!scrollAxisMoved && isScrollAxisReleased())
	{
		return;
	}
	else if(!scrollAxisMoved && !isScrollAxisReleased())
	{
		scrollAxisMoved = true;
		scrollCurrent = scrollStart = GH.input().getCursorPosition();
		GH.events().dispatchGesturePanningStarted(scrollStart);
	}
	else if(scrollAxisMoved && isScrollAxisReleased())
	{
		GH.events().dispatchGesturePanningEnded(scrollStart, scrollCurrent);
		scrollAxisMoved = false;
		scrollPlanDisX = scrollPlanDisY = 0;
		return;
	}
	scrollPlanDisX += ((float)deltaTimeMs / 1000) * ((float)scrollAxisValueX / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;
	scrollPlanDisY += ((float)deltaTimeMs / 1000) * ((float)scrollAxisValueY / AXIS_MAX_ZOOM) * AXIS_MOVE_SPEED;
	int moveDisX = getMoveDis(scrollPlanDisX);
	int moveDisY = getMoveDis(scrollPlanDisY);
	if(moveDisX != 0 || moveDisY != 0)
	{
		scrollPlanDisX -= moveDisX;
		scrollPlanDisY -= moveDisY;
		scrollCurrent.x += moveDisX;
		scrollCurrent.y += moveDisY;
		Point distance(moveDisX, moveDisY);
		GH.events().dispatchGesturePanning(scrollStart, scrollCurrent, distance);
	}
}

bool InputSourceGameController::isScrollAxisReleased()
{
	return scrollAxisValueX == 0 && scrollAxisValueY == 0;
}
