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

enum class EShortcut;

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class ShortcutHandler
{
	std::multimap<std::string, EShortcut> mappedKeyboardShortcuts;
	std::multimap<std::string, EShortcut> mappedJoystickShortcuts;
	std::multimap<std::string, EShortcut> mappedJoystickAxes;

	std::multimap<std::string, EShortcut> loadShortcuts(const JsonNode & data) const;
	std::vector<EShortcut> translateShortcut(const std::multimap<std::string, EShortcut> & options, const std::string & key) const;

public:
	ShortcutHandler();

	void reloadShortcuts();

	/// returns list of shortcuts assigned to provided SDL keycode
	std::vector<EShortcut> translateKeycode(const std::string & key) const;

	std::vector<EShortcut> translateJoystickButton(const std::string & key) const;

	std::vector<EShortcut> translateJoystickAxis(const std::string & key) const;

	/// attempts to find shortcut by its unique identifier. Returns EShortcut::NONE on failure
	EShortcut findShortcut(const std::string & identifier ) const;
};
