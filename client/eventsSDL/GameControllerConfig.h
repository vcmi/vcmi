/*
* GameControllerConfig.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include <SDL.h>

#include "../gui/Shortcut.h"
#include "../../lib/json/JsonUtils.h"

enum AxisType
{
	CURSOR_MOTION,
	MAP_SCROLL,
	NONE
};

class GameControllerConfig {
	using ButtonShortcutsMap = std::map<SDL_GameControllerButton, std::vector<EShortcut> >;
	using TriggerShortcutsMap = std::map<SDL_GameControllerAxis, std::vector<EShortcut> >;
	ButtonShortcutsMap buttonShortcutsMap;
	TriggerShortcutsMap triggerShortcutsMap;
	std::set<SDL_GameControllerButton> leftClickButtonSet;
	std::set<SDL_GameControllerButton> rightClickButtonSet;
	std::set<SDL_GameControllerAxis> leftClickTriggerSet;
	std::set<SDL_GameControllerAxis> rightClickTriggerSet;
	AxisType leftAxisType;
	AxisType rightAxisType;

	void load();
	std::vector<std::string> getOperations(const std::string & key, const JsonNode & value);
	AxisType parseAxis(const std::string & key, const JsonNode & value);
	void parseTrigger(const std::string & key, const JsonNode & value);
	void parseButton(const std::string & key, const JsonNode & value);

public:
	GameControllerConfig();
	~GameControllerConfig() = default;

	const AxisType & getLeftAxisType();
	const AxisType & getRightAxisType();

	bool isLeftClickButton(int buttonValue);
	bool isRightClickButton(int buttonValue);
	bool isShortcutsButton(int buttonValue);
	const std::vector<EShortcut> & getButtonShortcuts(int buttonValue);

	bool isLeftClickTrigger(int axisValue);
	bool isRightClickTrigger(int axisValue);
	bool isShortcutsTrigger(int axisValue);
	const std::vector<EShortcut> & getTriggerShortcuts(int axisValue);
};
