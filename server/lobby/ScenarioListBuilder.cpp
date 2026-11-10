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
#include "../../lib/mapping/ScenarioMetadata.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/SaveFileFormat.h"

#include <tbb/parallel_for.h>

static EResType getResourceType(ScenarioListEntry::EType type)
{
	switch(type)
	{
		case ScenarioListEntry::EType::MAP:
			return EResType::MAP;
		case ScenarioListEntry::EType::SAVE:
			return EResType::SAVEGAME;
		case ScenarioListEntry::EType::CAMPAIGN:
			return EResType::CAMPAIGN;
	}
	throw std::runtime_error("Invalid scenario list type");
}

ScenarioListBuilder::ScenarioListBuilder(ScenarioListEntry::EType type, ELoadMode loadMode)
	: type(type)
	, loadMode(loadMode)
{
	const std::string directory = type == ScenarioListEntry::EType::SAVE ? "SAVES/" : "MAPS/";
	const EResType resourceType = getResourceType(type);

	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount)
	{
		return boost::algorithm::starts_with(mount, directory);
	});

	const auto found = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == resourceType && boost::algorithm::starts_with(ident.getName(), directory);
	});

	for(const auto & file : found)
	{
		// file may be removed after the scan, for example an autosave pruned by another running game
		try
		{
			lastWriteTimes[file] = CResourceHandler::get()->getLastWriteTime(file);
			files.push_back(file);
		}
		catch(const boost::filesystem::filesystem_error & e)
		{
			logGlobal->warn("Skipping %s, failed to get its modification time: %s", file.getOriginalName(), e.what());
		}
	}

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

	if(type == ScenarioListEntry::EType::SAVE)
		result.entries = parseSaves(folderFiles.files, result.unsupportedSaves);
	else
		result.entries = parseMapsOrCampaigns(folderFiles.files);

	for(const auto & [name, subfolder] : folderFiles.subfolders)
	{
		ScenarioListFolder entry;
		entry.name = name;
		for(const auto & file : subfolder)
			entry.lastWrite = std::max(entry.lastWrite, lastWriteTimes.at(file));
		if(type == ScenarioListEntry::EType::SAVE)
		{
			const auto status = getFolderStatus(subfolder);
			if(!status)
				continue;
			entry.status = *status;
		}
		result.subfolders.push_back(entry);
	}

	return result;
}

static const ScenarioListEntry * findNewestEntry(const ScenarioList & list)
{
	const ScenarioListEntry * result = nullptr;
	for(const auto & entry : list.entries)
		if(entry.getStatus() != ScenarioListEntry::EStatus::COMPATIBLE)
			continue;
		else if(!result || entry.lastWrite > result->lastWrite || (entry.lastWrite == result->lastWrite && entry.fileURI > result->fileURI))
			result = &entry;
	return result;
}

std::optional<ScenarioList> ScenarioListBuilder::buildFolderOf(const std::string & fileName, std::string & selectedFile) const
{
	if(fileName.empty())
		return std::nullopt;

	const std::string folder = ScenarioList::getFolderOf(fileName);

	// random maps are selected in the random map tab, so their folder is not opened in the list of maps
	if(type == ScenarioListEntry::EType::MAP && boost::algorithm::iequals(folder, "RandomMaps/"))
		return std::nullopt;

	auto list = build(folder);
	const ResourcePath file(fileName, getResourceType(type));
	for(const auto & entry : list.entries)
	{
		if(entry.getStatus() == ScenarioListEntry::EStatus::COMPATIBLE && boost::algorithm::iequals(entry.fileURI, file.getName()))
		{
			selectedFile = entry.fileURI;
			return list;
		}
	}
	return std::nullopt;
}

ScenarioList ScenarioListBuilder::buildForFile(const std::string & fileName, std::string & selectedFile) const
{
	if(auto list = buildFolderOf(fileName, selectedFile))
		return *list;
	return build("");
}

ScenarioList ScenarioListBuilder::buildForSave(const std::string & saveName, std::string & selectedFile) const
{
	if(auto list = buildFolderOf(saveName, selectedFile))
		return *list;

	auto root = build("");

	struct Candidate
	{
		std::time_t lastWrite;
		std::string name;
		bool isFolder;
	};
	std::vector<Candidate> candidates;
	for(const auto & entry : root.entries)
		if(entry.getStatus() == ScenarioListEntry::EStatus::COMPATIBLE)
			candidates.push_back({entry.lastWrite, entry.fileURI, false});
	for(const auto & folder : root.subfolders)
		if(folder.status == ScenarioListEntry::EStatus::COMPATIBLE)
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

std::vector<ScenarioListEntry> ScenarioListBuilder::parseMapsOrCampaigns(const std::vector<ResourcePath> & folderFiles) const
{
	auto timeStart = std::chrono::steady_clock::now();

	// every entry is written by a single thread only, entries of failed files stay empty
	std::vector<std::optional<ScenarioListEntry>> entries(folderFiles.size());

	tbb::parallel_for(tbb::blocked_range<size_t>(0, folderFiles.size()), [&](const tbb::blocked_range<size_t> & r)
	{
		for(auto i = r.begin(); i != r.end(); i++)
		{
			const auto & file = folderFiles[i];
			try
			{
				if(type == ScenarioListEntry::EType::CAMPAIGN)
				{
					entries[i] = ScenarioListEntry::fromCampaign(file);
					continue;
				}

				auto entry = ScenarioListEntry::fromMap(file);
				if(entry.getMapFormatSettings()["supported"].Bool())
					entries[i] = std::move(entry);
			}
			catch(const std::exception & e)
			{
				logGlobal->error("File %s is invalid. Message: %s", file.getName(), e.what());
			}
		}
	});

	std::vector<ScenarioListEntry> result;
	for(auto & entry : entries)
		if(entry)
			result.push_back(std::move(*entry));

	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStart);
	logGlobal->debug("Parsing %d maps or campaigns took %d ms", folderFiles.size(), timeElapsed.count());
	return result;
}

ScenarioListEntry ScenarioListBuilder::createUnreadableEntry(const ResourcePath & file, ScenarioListEntry::EProblem problem, const std::string & error) const
{
	ScenarioListEntry result;
	result.type = type;
	result.fileURI = type == ScenarioListEntry::EType::SAVE ? file.getName() : file.getOriginalName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = lastWriteTimes.at(file);
	result.problem = problem;
	result.error = error;
	return result;
}

ScenarioListEntry ScenarioListBuilder::readSave(const ResourcePath & file, JsonValidator & validator) const
{
	try
	{
		auto entry = ScenarioListEntry::fromSave(file, validator);
		entry.mods = ScenarioMetadata::verifyMods(entry.metadata["mods"], true);
		if(!entry.mods.empty())
			entry.problem = ScenarioListEntry::EProblem::MODS;
		return entry;
	}
	catch(const SaveTooOldException & e)
	{
		logGlobal->warn("Saved game %s is too old: %s", file.getOriginalName(), e.what());
		return createUnreadableEntry(file, ScenarioListEntry::EProblem::SAVE_TOO_OLD, e.what());
	}
	catch(const SaveTooNewException & e)
	{
		logGlobal->warn("Saved game %s is too new: %s", file.getOriginalName(), e.what());
		auto entry = createUnreadableEntry(file, ScenarioListEntry::EProblem::SAVE_TOO_NEW, e.what());
		try
		{
			entry.metadata = SaveMetadata::parse(CLoadFile::readMetadata(*CResourceHandler::get()->getResourceName(file)), file.getOriginalName(), validator);
		}
		catch(const std::exception & metadataError) // NOSONAR: metadata of a newer version may be invalid in several ways
		{
			logGlobal->warn("Failed to read metadata of %s: %s", file.getOriginalName(), metadataError.what());
		}
		return entry;
	}
	catch(const std::exception & e) // NOSONAR: malformed saves can throw several exception types
	{
		logGlobal->error("Failed to read saved game %s: %s", file.getOriginalName(), e.what());
		return createUnreadableEntry(file, ScenarioListEntry::EProblem::INVALID_FILE, e.what());
	}
}

std::vector<ScenarioListEntry> ScenarioListBuilder::parseSaves(const std::vector<ResourcePath> & folderFiles, std::vector<std::string> & unsupportedSaves) const
{
	auto timeStart = std::chrono::steady_clock::now();

	// every entry is written by a single thread only, entries of saves of other load modes stay empty
	std::vector<std::optional<ScenarioListEntry>> entries(folderFiles.size());

	tbb::parallel_for(tbb::blocked_range<size_t>(0, folderFiles.size()), [&](const tbb::blocked_range<size_t> & r)
	{
		// validator can not be shared between threads
		JsonValidator validator;
		for(auto i = r.begin(); i != r.end(); i++)
		{
			auto entry = readSave(folderFiles[i], validator);
			if(isSaveCompatible(entry, loadMode))
				entries[i] = std::move(entry);
		}
	});

	std::vector<ScenarioListEntry> result;
	for(auto & entry : entries)
	{
		if(!entry)
			continue;
		if(entry->problem == ScenarioListEntry::EProblem::SAVE_TOO_OLD)
			unsupportedSaves.push_back(entry->fileURI);
		result.push_back(std::move(*entry));
	}

	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStart);
	logGlobal->debug("Parsing %d saves took %d ms", folderFiles.size(), timeElapsed.count());
	return result;
}

std::optional<ScenarioListEntry::EStatus> ScenarioListBuilder::getFolderStatus(std::vector<ResourcePath> folderFiles) const
{
	// Autosaves always stay in their game's directory, unlike stray manual saves.
	std::ranges::sort(folderFiles, [this](const ResourcePath & lhs, const ResourcePath & rhs)
	{
		const bool lhsAutosave = SavegamePath::isAutosaveName(lhs.getOriginalName());
		const bool rhsAutosave = SavegamePath::isAutosaveName(rhs.getOriginalName());
		if(lhsAutosave != rhsAutosave)
			return lhsAutosave;

		const auto lhsTime = lastWriteTimes.at(lhs);
		const auto rhsTime = lastWriteTimes.at(rhs);
		if(lhsTime != rhsTime)
			return lhsTime > rhsTime;
		return lhs.getName() > rhs.getName();
	});

	JsonValidator validator;
	for(const auto & file : folderFiles)
	{
		const auto entry = readSave(file, validator);
		if(entry.metadata["game"].isNull())
			continue;
		if(!isSaveCompatible(entry, loadMode))
			return std::nullopt;
		return entry.getStatus();
	}

	// Keep unreadable folders visible so that their saves are never hidden from the player.
	return ScenarioListEntry::EStatus::COMPATIBLE;
}

bool ScenarioListBuilder::isSaveCompatible(const ScenarioListEntry & entry, ELoadMode loadMode)
{
	if(entry.metadata["game"].isNull())
		return true;

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
