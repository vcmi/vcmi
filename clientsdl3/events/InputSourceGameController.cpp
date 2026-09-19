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

#include "GameEngine.h"
#include "gui/CursorHandler.h"
#include "gui/EventDispatcher.h"
#include "gui/ShortcutHandler.h"
#include "gui/WindowHandler.h"
#include "../render/IScreenHandler.h"

#include "lib/CConfigHandler.h"

namespace
{

int controllerInstanceId(SDL_JoystickID nativeId)
{
	return static_cast<int>(nativeId);
}

ControllerPrompt::Family controllerPromptFamily(SDL_Gamepad * controller)
{
	switch(SDL_GetGamepadType(controller))
	{
	case SDL_GAMEPAD_TYPE_PS3:
	case SDL_GAMEPAD_TYPE_PS4:
	case SDL_GAMEPAD_TYPE_PS5:
		return ControllerPrompt::Family::PLAYSTATION;
	case SDL_GAMEPAD_TYPE_XBOX360:
	case SDL_GAMEPAD_TYPE_XBOXONE:
		return ControllerPrompt::Family::XBOX;
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
		return ControllerPrompt::Family::NINTENDO;
	default:
		return ControllerPrompt::Family::GENERIC;
	}
}

}

void InputSourceGameController::gameControllerDeleter(SDL_Gamepad * gameController)
{
	if(gameController)
		SDL_CloseGamepad(gameController);
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
	scrollPlanDisY(0.0),
	configTriggerThreshold(settings["input"]["controllerTriggerThreshold"].Float()),
	configAxisDeadZone(settings["input"]["controllerAxisDeadZone"].Float()),
	configAxisFullZone(settings["input"]["controllerAxisFullZone"].Float()),
	configAxisSpeed(settings["input"]["controllerAxisSpeed"].Float()),
	configAxisScale(settings["input"]["controllerAxisScale"].Float())
{
	tryOpenAllGameControllers();
}

void InputSourceGameController::tryOpenAllGameControllers()
{
	// SDL3 enumerates gamepads directly, already filtering out unsupported joysticks
	int count = 0;
	SDL_JoystickID * gamepads = SDL_GetGamepads(&count);

	for(int i = 0; gamepads && i < count; ++i)
		openGameController(gamepads[i]);

	SDL_free(gamepads);
}

void InputSourceGameController::openGameController(SDL_JoystickID instanceID)
{
	SDL_Gamepad * controller = SDL_OpenGamepad(instanceID);
	if(!controller)
	{
		logGlobal->error("Fail to open game controller %d!", static_cast<int>(instanceID));
		return;
	}
	GameControllerPtr controllerPtr(controller, &gameControllerDeleter);

	// Need to save joystick index for event. Joystick index may not be equal to index sometimes.
	int joystickIndex = getJoystickIndex(controllerPtr.get());
	if(joystickIndex < 0)
	{
		logGlobal->error("Fail to get joystick index of game controller %d!", static_cast<int>(instanceID));
		return;
	}

	if(gameControllerMap.find(joystickIndex) != gameControllerMap.end())
	{
		logGlobal->warn("Game controller with joystick index %d is already opened.", joystickIndex);
		return;
	}

	gameControllerMap.try_emplace(joystickIndex, std::move(controllerPtr));
}

ControllerPrompt::Family InputSourceGameController::getActiveControllerPromptFamily() const
{
	const auto active = gameControllerMap.find(activeController);
	if(active == gameControllerMap.end())
		return ControllerPrompt::Family::UNKNOWN;

	return controllerPromptFamily(active->second.get());
}

int InputSourceGameController::getJoystickIndex(SDL_Gamepad * controller)
{
	SDL_Joystick * joystick = SDL_GetGamepadJoystick(controller);
	if(!joystick)
		return -1;

	SDL_JoystickID instanceID = SDL_GetJoystickID(joystick);
	if(instanceID == 0)
		return -1;
	return static_cast<int>(instanceID);
}

void InputSourceGameController::handleEventDeviceAdded(const SDL_GamepadDeviceEvent & device)
{
	const int instanceId = controllerInstanceId(device.which);
	if(gameControllerMap.find(instanceId) != gameControllerMap.end())
	{
		logGlobal->warn("Game controller %d is already opened.", instanceId);
		return;
	}
	openGameController(device.which);
}

void InputSourceGameController::handleEventDeviceRemoved(const SDL_GamepadDeviceEvent & device)
{
	const int instanceId = controllerInstanceId(device.which);
	if(gameControllerMap.find(instanceId) == gameControllerMap.end())
	{
		logGlobal->warn("Game controller %d is not opened before.", instanceId);
		return;
	}
	if(activeController == instanceId)
	{
		activeController = -1;
		resetControllerInput();
		ENGINE->windows().resetControllerInput();
	}
	std::erase_if(suppressedAxisReleases, [instanceId](const auto & entry)
	{
		return entry.first == instanceId;
	});
	std::erase_if(suppressedButtonReleases, [instanceId](const auto & entry)
	{
		return entry.first == instanceId;
	});
	gameControllerMap.erase(instanceId);
}

void InputSourceGameController::handleEventDeviceRemapped(const SDL_GamepadDeviceEvent & device)
{
	const int instanceId = controllerInstanceId(device.which);
	if(gameControllerMap.find(instanceId) == gameControllerMap.end())
	{
		logGlobal->warn("Game controller %d is not opened.", instanceId);
		return;
	}
	if(activeController == instanceId)
	{
		resetControllerInput();
		ENGINE->windows().resetControllerInput();
	}
	gameControllerMap.erase(instanceId);
	openGameController(device.which);
}

void InputSourceGameController::setActiveController(int instanceID)
{
	if(activeController != -1 && activeController != instanceID)
	{
		resetControllerInput();
		ENGINE->windows().resetControllerInput();
	}
	activeController = instanceID;
}

bool InputSourceGameController::isAxisMotionActive(const SDL_GamepadAxisEvent & axis) const
{
	return !vstd::isAlmostZero(getRealAxisValue(axis.value));
}

double InputSourceGameController::getRealAxisValue(int value) const
{
	if(configAxisDeadZone < 0.0 || configAxisFullZone > 1.0 || configAxisFullZone <= configAxisDeadZone)
		return 0.0;

	const double ratio = std::clamp(
		static_cast<double>(value) / SDL_JOYSTICK_AXIS_MAX, -1.0, 1.0);
	const double magnitude = std::abs(ratio);
	if(magnitude <= configAxisDeadZone)
		return 0.0;

	const double normalized = std::clamp(
		(magnitude - configAxisDeadZone) / (configAxisFullZone - configAxisDeadZone), 0.0, 1.0);
	return std::copysign(normalized, ratio);
}

void InputSourceGameController::dispatchAxisShortcuts(const std::vector<EShortcut> & shortcutsVector,
	int instanceId, SDL_GamepadAxis axisID, int axisValue, const std::string & axisName)
{
	const bool pressedNow = getRealAxisValue(axisValue) > configTriggerThreshold;
	const auto suppressed = suppressedAxisReleases.find({instanceId, axisID});
	if(suppressed != suppressedAxisReleases.end())
	{
		if(!pressedNow)
			suppressedAxisReleases.erase(suppressed);
		return;
	}

	if(pressedNow)
	{
		if(!pressedAxes.count(axisID) && instanceId == activeController)
		{
			ENGINE->events().dispatchKeyPressed(axisName);
			ENGINE->events().dispatchShortcutPressed(shortcutsVector);
			pressedAxes.emplace(axisID, PressedAxis{instanceId, shortcutsVector});
		}
	}
	else
	{
		const auto pressed = pressedAxes.find(axisID);
		if(pressed != pressedAxes.end()
			&& pressed->second.instanceId == instanceId
			&& instanceId == activeController)
		{
			ENGINE->events().dispatchKeyReleased(axisName);
			ENGINE->events().dispatchShortcutReleased(pressed->second.actions);
			pressedAxes.erase(pressed);
		}
	}
}

void InputSourceGameController::handleEventAxisMotion(const SDL_GamepadAxisEvent & axis)
{
	const int instanceId = controllerInstanceId(axis.which);
	SDL_GamepadAxis axisID = static_cast<SDL_GamepadAxis>(axis.axis);
	std::string axisName = SDL_GetGamepadStringForAxis(axisID);

	auto axisActions = ENGINE->shortcuts().translateJoystickAxis(axisName);
	auto buttonActions = ENGINE->shortcuts().translateJoystickButton(axisName, getActiveControllerPromptFamily());
	const double normalizedAxisValue = getRealAxisValue(axis.value);
	if(instanceId != activeController)
	{
		dispatchAxisShortcuts(buttonActions, instanceId, axisID, axis.value, axisName);
		return;
	}

	if(ENGINE->windows().dispatchControllerAxis(instanceId, axisActions, normalizedAxisValue))
	{
		clearAxisMotion();
		return;
	}

	if(isAxisMotionActive(axis))
		tryToConvertCursor();

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

	dispatchAxisShortcuts(buttonActions, instanceId, axisID, axis.value, axisName);
}

void InputSourceGameController::clearAxisMotion()
{
	cursorAxisValueX = cursorAxisValueY = 0;
	cursorPlanDisX = cursorPlanDisY = 0;
	scrollAxisValueX = scrollAxisValueY = 0;
	scrollPlanDisX = scrollPlanDisY = 0;
	if(scrollAxisMoved)
	{
		scrollAxisMoved = false;
		ENGINE->events().dispatchGesturePanningCanceled();
	}
}

void InputSourceGameController::resetControllerInput()
{
	clearAxisMotion();
	cancelControllerPresses();
}

void InputSourceGameController::cancelControllerPresses()
{
	const auto axesToRelease = std::move(pressedAxes);
	pressedAxes.clear();
	for(const auto & entry : axesToRelease)
	{
		suppressedAxisReleases.emplace(entry.second.instanceId, entry.first);
		ENGINE->events().cancelShortcutPress(entry.second.actions);
	}

	const auto buttonsToRelease = std::move(pressedButtons);
	pressedButtons.clear();
	for(const auto & [button, pressed] : buttonsToRelease)
	{
		suppressedButtonReleases.emplace(pressed.instanceId, button);
		ENGINE->events().cancelShortcutPress(pressed.actions);
	}
}

void InputSourceGameController::tryToConvertCursor()
{
	if(ENGINE->cursor().getShowType() == Cursor::ShowType::HARDWARE)
	{
		int scalingFactor = ENGINE->screenHandler().getScalingFactor();
		const Point & cursorPosition = ENGINE->getCursorPosition();
		ENGINE->cursor().changeCursor(Cursor::ShowType::SOFTWARE);
		ENGINE->cursor().cursorMove(cursorPosition.x * scalingFactor, cursorPosition.y * scalingFactor);
		ENGINE->input().setCursorPosition(cursorPosition);
	}
}

void InputSourceGameController::handleEventButtonDown(const SDL_GamepadButtonEvent & button)
{
	const int instanceId = controllerInstanceId(button.which);
	const auto buttonID = static_cast<SDL_GamepadButton>(button.button);
	std::string buttonName = SDL_GetGamepadStringForButton(buttonID);
	const auto & shortcutsVector = ENGINE->shortcuts().translateJoystickButton(buttonName, getActiveControllerPromptFamily());
	if(suppressedButtonReleases.contains({instanceId, buttonID})
		|| !pressedButtons.emplace(buttonID, PressedButton{instanceId, shortcutsVector}).second)
		return;
	
	ENGINE->events().dispatchKeyPressed(buttonName);
	ENGINE->events().dispatchShortcutPressed(shortcutsVector);
}

void InputSourceGameController::handleEventButtonUp(const SDL_GamepadButtonEvent & button)
{
	const int instanceId = controllerInstanceId(button.which);
	const auto buttonID = static_cast<SDL_GamepadButton>(button.button);
	if(suppressedButtonReleases.erase({instanceId, buttonID}) != 0)
		return;
	const auto pressed = pressedButtons.find(buttonID);
	if(pressed == pressedButtons.end()
		|| pressed->second.instanceId != instanceId
		|| instanceId != activeController)
		return;
	const auto shortcutsVector = pressed->second.actions;
	pressedButtons.erase(pressed);
	std::string buttonName = SDL_GetGamepadStringForButton(buttonID);
	ENGINE->events().dispatchKeyReleased(buttonName);
	ENGINE->events().dispatchShortcutReleased(shortcutsVector);
}

void InputSourceGameController::doCursorMove(int deltaX, int deltaY)
{
	if(deltaX == 0 && deltaY == 0)
		return;
	const Point & screenSize = ENGINE->screenDimensions();
	const Point & cursorPosition = ENGINE->getCursorPosition();
	int scalingFactor = ENGINE->screenHandler().getScalingFactor();
	int newX = std::min(std::max(cursorPosition.x + deltaX, 0), screenSize.x);
	int newY = std::min(std::max(cursorPosition.y + deltaY, 0), screenSize.y);
	Point targetPosition{newX, newY};
	ENGINE->input().setCursorPosition(targetPosition);
	ENGINE->cursor().cursorMove(ENGINE->getCursorPosition().x * scalingFactor, ENGINE->getCursorPosition().y * scalingFactor);
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

	int32_t deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowMs - lastCheckTime).count();
	handleCursorUpdate(deltaTime);
	handleScrollUpdate(deltaTime);
	lastCheckTime = nowMs;
}

static double scaleAxis(double value, double power)
{
	if (value > 0)
		return std::pow(value, power);
	else
		return -std::pow(-value, power);
}

void InputSourceGameController::handleCursorUpdate(int32_t deltaTimeMs)
{
	float deltaTimeSeconds = static_cast<float>(deltaTimeMs) / 1000;

	if(vstd::isAlmostZero(cursorAxisValueX))
		cursorPlanDisX = 0;
	else
		cursorPlanDisX += deltaTimeSeconds * configAxisSpeed * scaleAxis(cursorAxisValueX, configAxisScale);

	if (vstd::isAlmostZero(cursorAxisValueY))
		cursorPlanDisY = 0;
	else
		cursorPlanDisY += deltaTimeSeconds * configAxisSpeed * scaleAxis(cursorAxisValueY, configAxisScale);

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
		scrollCurrent = scrollStart = ENGINE->input().getCursorPosition();
		ENGINE->events().dispatchGesturePanningStarted(scrollStart);
	}
	else if(scrollAxisMoved && isScrollAxisReleased())
	{
		scrollAxisMoved = false;
		scrollPlanDisX = scrollPlanDisY = 0;
		ENGINE->events().dispatchGesturePanningEnded(scrollStart, scrollCurrent);
		return;
	}
	float deltaTimeSeconds = static_cast<float>(deltaTimeMs) / 1000;
	scrollPlanDisX += deltaTimeSeconds * configAxisSpeed * scaleAxis(scrollAxisValueX, configAxisScale);
	scrollPlanDisY += deltaTimeSeconds * configAxisSpeed * scaleAxis(scrollAxisValueY, configAxisScale);
	int moveDisX = getMoveDis(scrollPlanDisX);
	int moveDisY = getMoveDis(scrollPlanDisY);
	if(moveDisX != 0 || moveDisY != 0)
	{
		scrollPlanDisX -= moveDisX;
		scrollPlanDisY -= moveDisY;
		scrollCurrent.x += moveDisX;
		scrollCurrent.y += moveDisY;
		Point distance(moveDisX, moveDisY);
		ENGINE->events().dispatchGesturePanning(scrollStart, scrollCurrent, distance);
	}
}

bool InputSourceGameController::isScrollAxisReleased() const
{
	return vstd::isAlmostZero(scrollAxisValueX) && vstd::isAlmostZero(scrollAxisValueY);
}
