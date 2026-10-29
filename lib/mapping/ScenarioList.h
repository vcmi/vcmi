/*
 * ScenarioList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ScenarioListEntry.h"

/// Subfolder of a folder with maps or saved games
struct DLL_LINKAGE ScenarioListFolder
{
	std::string name;
	std::time_t lastWrite = 0; ///< newest file in the subfolder
	std::optional<bool> compatible; ///< compatibility of its newest readable save, autosaves first; empty if none could be read or for maps

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & lastWrite;
		h & compatible;
	}
};

/// Contents of one folder with maps or saved games
struct DLL_LINKAGE ScenarioList
{
	ScenarioListEntry::EType type = ScenarioListEntry::EType::MAP;
	std::string folder; ///< relative to Maps/ or Saves/, ends with '/' unless empty
	std::vector<ScenarioListEntry> entries;
	std::vector<ScenarioListFolder> subfolders;
	std::vector<std::string> unsupportedSaves; ///< saves too old to be loaded

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & type;
		h & folder;
		h & entries;
		h & subfolders;
		h & unsupportedSaves;
	}
};
