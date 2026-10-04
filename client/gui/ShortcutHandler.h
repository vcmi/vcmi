/*
 * ShortcutHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ControllerPromptFamily.h"

enum class EShortcut;

class JsonNode;

class ShortcutHandler
{
	bool initialized = false;
	std::multimap<std::string, EShortcut> mappedKeyboardShortcuts;
	std::map<ControllerPrompt::Family, std::multimap<std::string, EShortcut>> mappedJoystickShortcuts;
	std::multimap<std::string, EShortcut> mappedJoystickAxes;

	std::multimap<std::string, EShortcut> loadShortcuts(const JsonNode & data, ControllerPrompt::Family family = ControllerPrompt::Family::UNKNOWN) const;
	std::vector<EShortcut> translateShortcut(const std::multimap<std::string, EShortcut> & options, const std::string & key) const;

public:
	ShortcutHandler();

	void reloadShortcuts();

	/// Resolves a family default; an explicit string/vector binding always takes precedence.
	const JsonNode & resolveBinding(const JsonNode & binding, ControllerPrompt::Family family) const;

	/// returns list of shortcuts assigned to provided SDL keycode
	std::vector<EShortcut> translateKeycode(const std::string & key) const;

	std::vector<EShortcut> translateJoystickButton(const std::string & key) const;

	std::vector<EShortcut> translateJoystickAxis(const std::string & key) const;

	/// Returns sorted unique joystick button bindings assigned to the provided shortcut.
	std::vector<std::string> getJoystickButtonBindings(EShortcut shortcut) const;

	/// attempts to find shortcut by its unique identifier. Returns EShortcut::NONE on failure
	EShortcut findShortcut(const std::string & identifier ) const;
};
