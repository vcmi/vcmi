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

	/// Returns folder of a file relative to Maps/ or Saves/, ends with '/' unless empty
	static std::string getFolderOf(const std::string & fileURI)
	{
		const std::string relativePath = fileURI.substr(fileURI.find('/') + 1);
		const auto separator = relativePath.rfind('/');
		return separator == std::string::npos ? "" : relativePath.substr(0, separator + 1);
	}

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

/// Map, saved game or campaign selected in lobby; does not refer to a list, so it stays valid after the list or the server is replaced
struct DLL_LINKAGE ScenarioSelection
{
	enum class EType : uint8_t
	{
		NONE,
		MAP,
		SAVE,
		CAMPAIGN,
		RANDOM_MAP,
		BATTLE_ONLY
	};

	EType type = EType::NONE;
	std::string fileURI; ///< for maps, saved games and campaigns

	bool operator==(const ScenarioSelection & other) const = default;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & type;
		h & fileURI;
	}
};
