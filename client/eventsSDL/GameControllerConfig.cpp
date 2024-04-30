/*
* GameControllerConfig.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include <SDL.h>

#include "StdInc.h"
#include "GameControllerConfig.h"
#include "../gui/CGuiHandler.h"
#include "../gui/ShortcutHandler.h"


GameControllerConfig::GameControllerConfig(): leftAxisType(AxisType::NONE), rightAxisType(AxisType::NONE)
{
	load();
}

void GameControllerConfig::load()
{
	const JsonNode config = JsonUtils::assembleFromFiles("config/shortcutsConfig");
	for(auto const & entry : config["joystick"].Struct())
	{
		std::string configName = entry.first;
		if(configName == "leftaxis")
			leftAxisType = parseAxis(entry.first, entry.second);
		else if (configName == "rightaxis")
			rightAxisType = parseAxis(entry.first, entry.second);
		else if (configName == "lefttrigger" || configName == "righttrigger")
			parseTrigger(entry.first, entry.second);
		else
			parseButton(entry.first, entry.second);
	}
}

AxisType GameControllerConfig::parseAxis(const std::string & key, const JsonNode & value)
{
	if(!value.isString())
	{
		logGlobal->error("The value of joystick config key %s should be a string!", key);
		return AxisType::NONE;
	}

	std::string featureName = value.String();
	if(featureName == "cursorMotion")
		return AxisType::CURSOR_MOTION;
	else if(featureName == "mapScroll")
		return AxisType::MAP_SCROLL;
	else if(featureName != "")
		logGlobal->error("Unknown value %s of joystick config key %s!", featureName, key);
	return AxisType::NONE;
}

void GameControllerConfig::parseTrigger(const std::string & key, const JsonNode & value)
{
	std::vector<std::string> operations = getOperations(key, value);
	SDL_GameControllerAxis triggerAxis = key == "lefttrigger" ?
											 SDL_CONTROLLER_AXIS_TRIGGERLEFT : SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
	std::vector<EShortcut> shortcuts;
	for(const auto & operation : operations)
	{
		if(operation == "mouseLeftClick")
		{
			leftClickTriggerSet.insert(triggerAxis);
		}
		else if(operation == "mouseRightClick")
		{
			rightClickTriggerSet.insert(triggerAxis);
		}
		else
		{
			EShortcut shortcut = GH.shortcuts().findShortcut(operation);
			if(shortcut == EShortcut::NONE)
				logGlobal->error("Shortcut %s in joystick config key %s is invalid.", operation, key);
			else
				shortcuts.push_back(shortcut);
		}
	}

	if(!shortcuts.empty())
		triggerShortcutsMap.emplace(triggerAxis, std::move(shortcuts));
}

void GameControllerConfig::parseButton(const std::string & key, const JsonNode & value)
{
	std::vector<std::string> operations = getOperations(key, value);
	SDL_GameControllerButton button = SDL_GameControllerGetButtonFromString(key.c_str());
	if(button == SDL_CONTROLLER_BUTTON_INVALID)
	{
		logGlobal->error("Joystick config key %s is invalid.", key);
		return;
	}

	std::vector<EShortcut> shortcuts;
	for(const auto & operation : operations)
	{
		if(operation == "mouseLeftClick")
		{
			leftClickButtonSet.insert(button);
		}
		else if(operation == "mouseRightClick")
		{
			rightClickButtonSet.insert(button);
		}
		else
		{
			EShortcut shortcut = GH.shortcuts().findShortcut(operation);
			if(shortcut == EShortcut::NONE)
				logGlobal->error("Shortcut %s in joystick config key %s is invalid.", operation, key);
			else
				shortcuts.push_back(shortcut);
		}
	}

	if(!shortcuts.empty())
		buttonShortcutsMap.emplace(button, std::move(shortcuts));
}

const AxisType & GameControllerConfig::getLeftAxisType()
{
	return leftAxisType;
}

const AxisType & GameControllerConfig::getRightAxisType()
{
	return rightAxisType;
}

std::vector<std::string> GameControllerConfig::getOperations(const std::string & key, const JsonNode & value)
{
	std::vector<std::string> operations;
	if(value.isString())
	{
		operations.push_back(value.String());
	}
	else if(value.isVector())
	{
		for(auto const & entryVector : value.Vector())
		{
			if(!entryVector.isString())
				logGlobal->error("The vector of joystick config key %s can not contain non-string element.", key);
			else
				operations.push_back(entryVector.String());
		}
	}
	else
	{
		logGlobal->error("The value of joystick config key %s should be string or string vector.", key);
	}
	return operations;
}

bool GameControllerConfig::isLeftClickButton(int buttonValue)
{
	SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(buttonValue);
	return leftClickButtonSet.find(button) != leftClickButtonSet.end();
}

bool GameControllerConfig::isRightClickButton(int buttonValue)
{
	SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(buttonValue);
	return rightClickButtonSet.find(button) != rightClickButtonSet.end();
}

bool GameControllerConfig::isShortcutsButton(int buttonValue)
{
	SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(buttonValue);
	return buttonShortcutsMap.find(button) != buttonShortcutsMap.end();
}

const std::vector<EShortcut> & GameControllerConfig::getButtonShortcuts(int buttonValue)
{
	SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(buttonValue);
	auto it = buttonShortcutsMap.find(button);
	if(it != buttonShortcutsMap.end())
		return it->second;
	static std::vector<EShortcut> emptyVec;
	return emptyVec;
}

bool GameControllerConfig::isLeftClickTrigger(int axisValue)
{
	SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(axisValue);
	return leftClickTriggerSet.find(axis) != leftClickTriggerSet.end();
}

bool GameControllerConfig::isRightClickTrigger(int axisValue)
{
	SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(axisValue);
	return rightClickTriggerSet.find(axis) != rightClickTriggerSet.end();
}

bool GameControllerConfig::isShortcutsTrigger(int axisValue)
{
	SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(axisValue);
	return triggerShortcutsMap.find(axis) != triggerShortcutsMap.end();
}

const std::vector<EShortcut> & GameControllerConfig::getTriggerShortcuts(int axisValue)
{
	SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(axisValue);
	auto it = triggerShortcutsMap.find(axis);
	if(it != triggerShortcutsMap.end())
		return it->second;
	static std::vector<EShortcut> emptyVec;
	return emptyVec;
}

