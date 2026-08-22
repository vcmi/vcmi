/*
 * SavegamePath.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SavegamePath.h"

#include "../GameLibrary.h"
#include "../StartInfo.h"
#include "../callback/Calendar.h"
#include "../campaign/CampaignState.h"
#include "Filesystem.h"
#include "../mapping/CMapHeader.h"
#include "../texts/CompositeTranslator.h"
#include "../texts/TextOperations.h"

#include <vstd/DateUtils.h>

namespace
{
constexpr int MAP_NAME_MAX_LENGTH = 50;

std::string sanitizeMapName(std::string name)
{
	const int textLength = TextOperations::getUnicodeCharactersCount(name);
	TextOperations::trimRightUnicode(name, std::max(0, textLength - MAP_NAME_MAX_LENGTH));

	const auto isIllegalCharacter = [](char character)
	{
		static const std::string forbiddenCharacters(R"(\/:*?"<>|)");
		const bool forbidden = forbiddenCharacters.find(character) != std::string::npos;
		const bool nonprintable = static_cast<unsigned char>(character) < static_cast<unsigned char>(' ');
		return forbidden || nonprintable;
	};
	std::replace_if(name.begin(), name.end(), isIllegalCharacter, '_');

	if(!name.empty() && (name.back() == '.' || name.back() == ' '))
		name.back() = '_';

	return name.empty() ? "map" : name;
}

std::string getGameName(const StartInfo & startInfo, const CMapHeader & mapHeader)
{
	// a save directory is named on whichever machine writes it, so it has to resolve the
	// map's own embedded text rather than wait for a player-facing translator
	CompositeTranslator translator;
	if(startInfo.campState)
	{
		// the campaign name is registered in the campaign's own container, not in the map's
		translator.install(startInfo.campState->getTexts());
		return sanitizeMapName(startInfo.getCampaignName(&translator));
	}

	translator.install(mapHeader.texts);
	return sanitizeMapName(mapHeader.name.toString(&translator));
}

std::string getStoredDirectory(const StartInfo & startInfo)
{
	if(!startInfo.saveDirectory.empty())
		return startInfo.saveDirectory;
	if(startInfo.campState)
		return startInfo.campState->getSaveDirectory();
	return {};
}

std::string getDirectoryBase(const StartInfo & startInfo, const CMapHeader & mapHeader)
{
	const std::string startDate = vstd::getFormattedDateTime(startInfo.startTime, "%Y-%m-%d");
	return getGameName(startInfo, mapHeader) + " " + startDate;
}

int getNextDirectoryIndex(const std::string & directoryBase)
{
	const std::string pathPrefix = boost::to_upper_copy("Saves/" + directoryBase + " ");
	int highestIndex = 0;

	const auto resources = CResourceHandler::get("local")->getFilteredFiles([&pathPrefix](const ResourcePath & resource)
	{
		return boost::starts_with(resource.getName(), pathPrefix);
	});

	for(const auto & resource : resources)
	{
		const std::string path = resource.getName();
		const size_t indexEnd = path.find('/', pathPrefix.size());
		if(indexEnd == std::string::npos)
			continue;

		const std::string index = path.substr(pathPrefix.size(), indexEnd - pathPrefix.size());
		if(index.empty() || !std::ranges::all_of(index, [](unsigned char character) { return std::isdigit(character); }))
			continue;

		try
		{
			highestIndex = std::max(highestIndex, std::stoi(index));
		}
		catch(const std::out_of_range & e)
		{
			logGlobal->trace("Ignoring out-of-range save directory index %s: %s", index, e.what());
		}
	}

	return highestIndex + 1;
}
}

std::string SavegamePath::generateGameDirectoryName(const StartInfo & startInfo, const CMapHeader & mapHeader)
{
	const std::string storedDirectory = getStoredDirectory(startInfo);
	if(!storedDirectory.empty())
		return storedDirectory;

	const std::string directoryBase = getDirectoryBase(startInfo, mapHeader);
	return directoryBase + " " + std::to_string(getNextDirectoryIndex(directoryBase));
}

std::string SavegamePath::getGameDirectoryName(const StartInfo & startInfo, const CMapHeader & mapHeader)
{
	std::string directory = getStoredDirectory(startInfo);
	if(directory.empty())
		directory = getDirectoryBase(startInfo, mapHeader) + " 1";
	return directory + "/";
}

std::string SavegamePath::getAutosavePath(const StartInfo & startInfo,
	const CMapHeader & mapHeader,
	const Calendar & calendar)
{
	const std::string filename = "Autosave-" + std::to_string(calendar.getMonth())
		+ std::to_string(calendar.getWeek()) + std::to_string(calendar.getDayOfWeek());
	return getPath(startInfo, mapHeader, filename);
}

bool SavegamePath::isAutosaveName(const std::string & filename)
{
	const size_t separator = filename.find_last_of("/\\");
	std::string name = separator == std::string::npos ? filename : filename.substr(separator + 1);
	if(boost::iends_with(name, ".vsgm1"))
		name.resize(name.size() - 6);

	static const std::string prefix = "Autosave-";
	if(!boost::istarts_with(name, prefix) || name.size() == prefix.size())
		return false;

	return std::ranges::all_of(name.substr(prefix.size()), [](unsigned char character)
	{
		return std::isdigit(character);
	});
}

std::string SavegamePath::getPath(const StartInfo & startInfo,
	const CMapHeader & mapHeader,
	const std::string & filename)
{
	return "Saves/" + getGameDirectoryName(startInfo, mapHeader) + filename;
}
