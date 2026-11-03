/*
 * ScenarioListBuilder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/mapping/ScenarioList.h"

enum class ELoadMode : ui8;

/// Lists maps or saved games available on the server, one folder at a time
class ScenarioListBuilder
{
	ScenarioListEntry::EType type;
	ELoadMode loadMode;
	std::vector<ResourcePath> files;

	std::vector<ScenarioListEntry> parseMaps(const std::vector<ResourcePath> & folderFiles) const;
	std::vector<ScenarioListEntry> parseSaves(const std::vector<ResourcePath> & folderFiles, std::vector<std::string> & unsupportedSaves) const;
	std::optional<bool> isFolderCompatible(std::vector<ResourcePath> folderFiles) const;
	/// Returns folder of the file and its file URI in selectedFile, empty if the file is not listed
	std::optional<ScenarioList> buildFolderOf(const std::string & fileName, std::string & selectedFile) const;

public:
	/// Files of one folder, grouped by their direct subfolders
	struct FolderFiles
	{
		std::string folder; ///< with letter case of the files
		std::vector<ResourcePath> files;
		std::map<std::string, std::vector<ResourcePath>> subfolders;
	};

	/// Groups files under folder, relative to their top-level directory such as Maps/ or Saves/; letter case of the folder is ignored
	static FolderFiles splitFolder(const std::vector<ResourcePath> & files, const std::string & folder);

	/// Scans the filesystem for all maps or saved games; saves are filtered by load mode
	ScenarioListBuilder(ScenarioListEntry::EType type, ELoadMode loadMode);

	/// Returns contents of a folder relative to Maps/ or Saves/, letter case of the folder is ignored
	ScenarioList build(const std::string & folder) const;

	/// Returns folder of the map to select, and its file URI in selectedFile; returns the root folder if the map is not listed
	ScenarioList buildForMap(const std::string & mapName, std::string & selectedFile) const;

	/// Returns folder of the saved game to select, and its file URI in selectedFile; if the save is not listed,
	/// selects the newest save of the root folder or of its newest subfolder with compatible saves
	ScenarioList buildForSave(const std::string & saveName, std::string & selectedFile) const;

	/// Returns true if saved game can be loaded in this load mode
	static bool isSaveCompatible(const ScenarioListEntry & entry, ELoadMode loadMode);
};
