/*
 * ScenarioListBuilder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScenarioListBuilder.h"

#include "../../lib/StartInfo.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/SavegamePath.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/json/JsonValidator.h"
#include "../../lib/serializer/SaveFileFormat.h"

#include <tbb/parallel_for.h>

ScenarioListBuilder::ScenarioListBuilder(ScenarioListEntry::EType type, ELoadMode loadMode)
	: type(type)
	, loadMode(loadMode)
{
	const std::string directory = type == ScenarioListEntry::EType::MAP ? "MAPS/" : "SAVES/";
	const EResType resourceType = type == ScenarioListEntry::EType::MAP ? EResType::MAP : EResType::SAVEGAME;

	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount)
	{
		return boost::algorithm::starts_with(mount, directory);
	});

	const auto found = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == resourceType && boost::algorithm::starts_with(ident.getName(), directory);
	});

	files.assign(found.begin(), found.end());

	if(type == ScenarioListEntry::EType::MAP)
	{
		std::erase(files, ResourcePath("Maps/Tutorial.tut", EResType::MAP));
		std::erase(files, ResourcePath("Maps/BattleOnlyMode.vmap", EResType::MAP));
	}
}

ScenarioListBuilder::FolderFiles ScenarioListBuilder::splitFolder(const std::vector<ResourcePath> & files, const std::string & folder)
{
	FolderFiles result;
	result.folder = folder;

	for(const auto & file : files)
	{
		const std::string & path = file.getOriginalName();
		const std::string relativePath = path.substr(path.find('/') + 1);
		if(!boost::algorithm::istarts_with(relativePath, folder))
			continue;

		result.folder = relativePath.substr(0, folder.size());

		const std::string rest = relativePath.substr(folder.size());
		const auto separator = rest.find('/');
		if(separator == std::string::npos)
			result.files.push_back(file);
		else
			result.subfolders[rest.substr(0, separator)].push_back(file);
	}
	return result;
}

ScenarioList ScenarioListBuilder::build(const std::string & folder) const
{
	const auto folderFiles = splitFolder(files, folder);

	ScenarioList result;
	result.type = type;
	result.folder = folderFiles.folder;

	if(type == ScenarioListEntry::EType::MAP)
		result.entries = parseMaps(folderFiles.files);
	else
		result.entries = parseSaves(folderFiles.files, result.unsupportedSaves);

	for(const auto & [name, subfolder] : folderFiles.subfolders)
	{
		ScenarioListFolder entry;
		entry.name = name;
		for(const auto & file : subfolder)
			entry.lastWrite = std::max(entry.lastWrite, CResourceHandler::get()->getLastWriteTime(file));
		if(type == ScenarioListEntry::EType::SAVE)
			entry.compatible = isFolderCompatible(subfolder);
		result.subfolders.push_back(entry);
	}

	return result;
}

static const ScenarioListEntry * findNewestEntry(const ScenarioList & list)
{
	const ScenarioListEntry * result = nullptr;
	for(const auto & entry : list.entries)
		if(!result || entry.lastWrite > result->lastWrite || (entry.lastWrite == result->lastWrite && entry.fileURI > result->fileURI))
			result = &entry;
	return result;
}

ScenarioList ScenarioListBuilder::buildForSave(const std::string & saveName, std::string & selectedFile) const
{
	if(!saveName.empty())
	{
		auto list = build(ScenarioList::getFolderOf(saveName));

		const ResourcePath save(saveName, EResType::SAVEGAME);
		for(const auto & entry : list.entries)
		{
			if(entry.fileURI == save.getName())
			{
				selectedFile = entry.fileURI;
				return list;
			}
		}
	}

	auto root = build("");

	struct Candidate
	{
		std::time_t lastWrite;
		std::string name;
		bool isFolder;
	};
	std::vector<Candidate> candidates;
	for(const auto & entry : root.entries)
		candidates.push_back({entry.lastWrite, entry.fileURI, false});
	for(const auto & folder : root.subfolders)
		if(folder.compatible != false)
			candidates.push_back({folder.lastWrite, folder.name, true});

	std::ranges::sort(candidates, [](const Candidate & lhs, const Candidate & rhs)
	{
		if(lhs.lastWrite != rhs.lastWrite)
			return lhs.lastWrite > rhs.lastWrite;
		return lhs.name > rhs.name;
	});

	for(const auto & candidate : candidates)
	{
		if(!candidate.isFolder)
		{
			selectedFile = candidate.name;
			return root;
		}

		auto list = build(root.folder + candidate.name + "/");
		if(const auto * newest = findNewestEntry(list))
		{
			selectedFile = newest->fileURI;
			return list;
		}
	}

	return root;
}

std::vector<ScenarioListEntry> ScenarioListBuilder::parseMaps(const std::vector<ResourcePath> & folderFiles) const
{
	auto timeStart = std::chrono::steady_clock::now();

	// every entry is written by a single thread only, entries of failed maps stay empty
	std::vector<std::optional<ScenarioListEntry>> entries(folderFiles.size());

	tbb::parallel_for(tbb::blocked_range<size_t>(0, folderFiles.size()), [&](const tbb::blocked_range<size_t> & r)
	{
		for(auto i = r.begin(); i != r.end(); i++)
		{
			const auto & file = folderFiles[i];
			try
			{
				auto entry = ScenarioListEntry::fromMap(file);
				if(entry.getMapFormatSettings()["supported"].Bool())
					entries[i] = std::move(entry);
			}
			catch(const std::exception & e)
			{
				logGlobal->error("Map %s is invalid. Message: %s", file.getName(), e.what());
			}
		}
	});

	std::vector<ScenarioListEntry> result;
	for(auto & entry : entries)
		if(entry)
			result.push_back(std::move(*entry));

	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStart);
	logGlobal->debug("Parsing %d maps took %d ms", folderFiles.size(), timeElapsed.count());
	return result;
}

std::vector<ScenarioListEntry> ScenarioListBuilder::parseSaves(const std::vector<ResourcePath> & folderFiles, std::vector<std::string> & unsupportedSaves) const
{
	auto timeStart = std::chrono::steady_clock::now();

	// every entry is written by a single thread only, entries of failed saves stay empty
	std::vector<std::optional<ScenarioListEntry>> entries(folderFiles.size());
	std::vector<uint8_t> isUnsupported(folderFiles.size(), 0);

	tbb::parallel_for(tbb::blocked_range<size_t>(0, folderFiles.size()), [&](const tbb::blocked_range<size_t> & r)
	{
		// validator can not be shared between threads
		JsonValidator validator;
		for(auto i = r.begin(); i != r.end(); i++)
		{
			const auto & file = folderFiles[i];
			try
			{
				auto entry = ScenarioListEntry::fromSave(file, validator);
				SaveMetadata::verifyMods(entry.metadata);

				if(isSaveCompatible(entry, loadMode))
					entries[i] = std::move(entry);
			}
			catch(const SaveTooOldException & e)
			{
				isUnsupported[i] = 1;
				logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
			}
			catch(const std::exception & e)
			{
				logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
			}
		}
	});

	std::vector<ScenarioListEntry> result;
	for(size_t i = 0; i < folderFiles.size(); i++)
	{
		if(entries[i])
			result.push_back(std::move(*entries[i]));
		if(isUnsupported[i])
			unsupportedSaves.push_back(folderFiles[i].getName());
	}

	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStart);
	logGlobal->debug("Parsing %d saves took %d ms", folderFiles.size(), timeElapsed.count());
	return result;
}

std::optional<bool> ScenarioListBuilder::isFolderCompatible(std::vector<ResourcePath> folderFiles) const
{
	// Autosaves always stay in their game's directory, unlike stray manual saves.
	std::ranges::sort(folderFiles, [](const ResourcePath & lhs, const ResourcePath & rhs)
	{
		const bool lhsAutosave = SavegamePath::isAutosaveName(lhs.getOriginalName());
		const bool rhsAutosave = SavegamePath::isAutosaveName(rhs.getOriginalName());
		if(lhsAutosave != rhsAutosave)
			return lhsAutosave;

		const auto lhsTime = CResourceHandler::get()->getLastWriteTime(lhs);
		const auto rhsTime = CResourceHandler::get()->getLastWriteTime(rhs);
		if(lhsTime != rhsTime)
			return lhsTime > rhsTime;
		return lhs.getName() > rhs.getName();
	});

	JsonValidator validator;
	for(const auto & file : folderFiles)
	{
		try
		{
			const auto entry = ScenarioListEntry::fromSave(file, validator);
			SaveMetadata::verifyMods(entry.metadata);
			return isSaveCompatible(entry, loadMode);
		}
		catch(const std::exception & e) // NOSONAR: malformed saves can throw several exception types
		{
			logGlobal->debug("Failed to inspect representative save %s: %s", file.getOriginalName(), e.what());
		}
	}

	// Keep unreadable folders visible so that their saves are never hidden from the player.
	return std::nullopt;
}

bool ScenarioListBuilder::isSaveCompatible(const ScenarioListEntry & entry, ELoadMode loadMode)
{
	const bool isCampaign = entry.isCampaignSave();
	const bool isMultiplayer = entry.getHumanPlayersInSave() > 1;
	const bool isTutorial = entry.isTutorialSave();

	switch(loadMode)
	{
	case ELoadMode::SINGLE:
		return !isCampaign && !isTutorial;
	case ELoadMode::CAMPAIGN:
		return isCampaign;
	case ELoadMode::TUTORIAL:
		return isTutorial;
	case ELoadMode::MULTI:
		return isMultiplayer;
	default:
		assert(0);
		return false;
	}
}
