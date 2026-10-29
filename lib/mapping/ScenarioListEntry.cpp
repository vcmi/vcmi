/*
 * ScenarioListEntry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScenarioListEntry.h"

#include "CMapHeader.h"
#include "CMapService.h"
#include "MapFormat.h"
#include "ScenarioMetadata.h"

#include "../GameLibrary.h"
#include "../IGameSettings.h"
#include "../StartInfo.h"
#include "../filesystem/Filesystem.h"
#include "../gameState/SaveMetadata.h"
#include "../serializer/CLegacyLoadFile.h"
#include "../serializer/CLoadFile.h"

ScenarioListEntry ScenarioListEntry::fromMap(const ResourcePath & file)
{
	CMapService mapService;
	const auto header = mapService.loadMapHeader(file);

	ScenarioListEntry result;
	result.type = EType::MAP;
	result.fileURI = file.getOriginalName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = CResourceHandler::get()->getLastWriteTime(file);
	result.metadata["map"] = ScenarioMetadata::mapToJson(*header);
	result.metadata["mods"] = ScenarioMetadata::modsToJson(header->mods);
	return result;
}

ScenarioListEntry ScenarioListEntry::fromSave(const ResourcePath & file, JsonValidator & validator)
{
	const auto saveFile = CResourceHandler::get()->getResourceName(file);
	if(!saveFile)
		throw std::runtime_error("Error: saved game '" + file.getOriginalName() + "' not found!");

	ScenarioListEntry result;
	result.type = EType::SAVE;
	result.fileURI = file.getName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = CResourceHandler::get()->getLastWriteTime(file);

	if(CLoadFile::readVersion(*saveFile) < ESerializationVersion::SAVE_CHUNKS)
	{
		CMapHeader mapHeader;
		StartInfo startInfo;
		CLegacyLoadFile loader(*saveFile, nullptr);
		loader.loadHeader(mapHeader, startInfo);
		result.metadata = SaveMetadata::createLegacy(mapHeader, startInfo, loader.loadMods());
	}
	else
	{
		CLoadFile loader(*saveFile);
		result.metadata = SaveMetadata::parse(loader.readChunk(SaveFileFormat::CHUNK_METADATA), file.getOriginalName(), validator);
	}
	return result;
}

std::string ScenarioListEntry::getMapName(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(metadata["map"]["name"], translator);
}

std::string ScenarioListEntry::getMapAuthor(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(metadata["map"]["author"], translator);
}

std::string ScenarioListEntry::getMapAuthorContact(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(metadata["map"]["authorContact"], translator);
}

std::string ScenarioListEntry::getMapVersion(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(metadata["map"]["mapVersion"], translator);
}

std::time_t ScenarioListEntry::getMapCreationTime() const
{
	return metadata["map"]["creationDateTime"].Integer();
}

EMapFormat ScenarioListEntry::getMapFormat() const
{
	return static_cast<EMapFormat>(metadata["map"]["format"].Integer());
}

const JsonNode & ScenarioListEntry::getMapFormatSettings() const
{
	switch(getMapFormat())
	{
		case EMapFormat::ROE:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA);
		case EMapFormat::AB:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE);
		case EMapFormat::SOD:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH);
		case EMapFormat::CHR:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_CHRONICLES);
		case EMapFormat::WOG:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS);
		case EMapFormat::HOTA:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS);
		case EMapFormat::VCMI:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_JSON_VCMI);
	}
	static const JsonNode unknownFormat;
	return unknownFormat;
}

int ScenarioListEntry::getMapWidth() const
{
	return metadata["map"]["width"].Integer();
}

int ScenarioListEntry::getVictoryIcon() const
{
	return metadata["map"]["victoryIcon"].Integer();
}

int ScenarioListEntry::getDefeatIcon() const
{
	return metadata["map"]["defeatIcon"].Integer();
}

int ScenarioListEntry::getPlayers() const
{
	return metadata["map"]["players"].Integer();
}

int ScenarioListEntry::getHumanPlayers() const
{
	return metadata["map"]["humanPlayers"].Integer();
}

bool ScenarioListEntry::isCampaignSave() const
{
	return metadata["game"]["mode"].String() == "campaign";
}

bool ScenarioListEntry::isTutorialSave() const
{
	return metadata["game"]["mode"].String() == "tutorial";
}

int ScenarioListEntry::getHumanPlayersInSave() const
{
	return metadata["game"]["humanPlayers"].Integer();
}
