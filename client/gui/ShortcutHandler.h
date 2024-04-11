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

class ShortcutHandler
{
	std::multimap<std::string, EShortcut> mappedShortcuts;
public:
	ShortcutHandler();

	/// returns list of shortcuts assigned to provided SDL keycode
	std::vector<EShortcut> translateKeycode(const std::string & key) const;

	/// attempts to find shortcut by its unique identifier. Returns EShortcut::NONE on failure
	EShortcut findShortcut(const std::string & identifier ) const;
};
