/*
 * CMapInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapInfo.h"

#include "../filesystem/ResourcePath.h"
#include "../StartInfo.h"
#include "../GameConstants.h"
#include "CMapService.h"
#include "CMapHeader.h"
#include "MapFormat.h"
#include "../json/JsonNode.h"

#include "../campaign/CampaignHandler.h"
#include "../filesystem/Filesystem.h"
#include "../GameLibrary.h"
#include "../rmg/CMapGenOptions.h"
#include "../serializer/CLoadFile.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextOperations.h"
#include "../CCreatureHandler.h"
#include "../IGameSettings.h"
#include "../CConfigHandler.h"

CMapInfo::CMapInfo()
	: amountOfPlayersOnMap(0), amountOfHumanControllablePlayers(0),	amountOfHumanPlayersInSave(0), isRandomMap(false)
{

}

CMapInfo::~CMapInfo() = default;


void CMapInfo::mapInit(const std::string & fname)
{
	fileURI = fname;
	CMapService mapService;
	ResourcePath resource = ResourcePath(fname, EResType::MAP);
	originalFileURI = resource.getOriginalName();
	fullFileURI = CResourceHandler::get()->getFullFileURI(resource);
	mapHeader = mapService.loadMapHeader(resource);
	lastWrite = CResourceHandler::get()->getLastWriteTime(resource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);
	countPlayers();
}

void CMapInfo::saveInit(const ResourcePath & file)
{
	CLoadFile lf(*CResourceHandler::get()->getResourceName(file), nullptr);

	mapHeader = std::make_unique<CMapHeader>();
	scenarioOptionsOfSave = std::make_unique<StartInfo>();
	lf.load(*mapHeader);
	lf.load(*scenarioOptionsOfSave);

	fileURI = file.getName(); // Name without file extension
	originalFileURI = file.getOriginalName(); // Same as file.getName() but keep letter case
	fullFileURI = CResourceHandler::get()->getFullFileURI(file); // Includes absolute path + extension
	countPlayers();
	lastWrite = CResourceHandler::get()->getLastWriteTime(file);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);

	// We absolutely not need this data for lobby and server will read it from save
	// FIXME: actually we don't want them in CMapHeader!
	mapHeader->triggeredEvents.clear();
}

void CMapInfo::campaignInit()
{
	ResourcePath resource = ResourcePath(fileURI, EResType::CAMPAIGN);
	originalFileURI = resource.getOriginalName();
	fullFileURI = CResourceHandler::get()->getFullFileURI(resource);
	campaign = CampaignHandler::getHeader(fileURI);
	lastWrite = CResourceHandler::get()->getLastWriteTime(resource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);
}

void CMapInfo::initFromCache(const JsonNode & data)
{
	fileURI = data["fileURI"].String();
	ResourcePath resource = ResourcePath(fileURI, EResType::MAP);
	originalFileURI = resource.getOriginalName();
	fullFileURI = CResourceHandler::get()->getFullFileURI(resource);
	lastWrite = CResourceHandler::get()->getLastWriteTime(resource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);

	mapHeader = std::make_unique<CMapHeader>();

	if (!data["version"].isNull())
		mapHeader->version = static_cast<EMapFormat>(data["version"].Integer());

	mapHeader->width = data["width"].Integer();
	mapHeader->height = data["height"].Integer();

	mapHeader->name.appendRawString(data["name"].String());
	if (!data["description"].isNull())
		mapHeader->description.appendRawString(data["description"].String());
	if (!data["author"].isNull())
		mapHeader->author.appendRawString(data["author"].String());
	if (!data["authorContact"].isNull())
		mapHeader->authorContact.appendRawString(data["authorContact"].String());
	if (!data["mapVersion"].isNull())
		mapHeader->mapVersion.appendRawString(data["mapVersion"].String());

	if (!data["creationDateTime"].isNull())
		mapHeader->creationDateTime = static_cast<std::time_t>(data["creationDateTime"].Integer());

	if (!data["difficulty"].isNull())
		mapHeader->difficulty = static_cast<EMapDifficulty>(data["difficulty"].Integer());

	if (!data["levelLimit"].isNull())
		mapHeader->levelLimit = static_cast<ui8>(data["levelLimit"].Integer());

	if (!data["howManyTeams"].isNull())
		mapHeader->howManyTeams = static_cast<ui8>(data["howManyTeams"].Integer());

	// Populate player info
	const JsonNode & playersNode = data["players"];
	mapHeader->players.resize(PlayerColor::PLAYER_LIMIT_I);
	size_t playerCount = std::min(playersNode.Vector().size(), static_cast<size_t>(PlayerColor::PLAYER_LIMIT_I));
	for (size_t i = 0; i < playerCount; ++i)
	{
		bool isPlayable = playersNode.Vector()[i].Bool();
		mapHeader->players[i].canHumanPlay = isPlayable;
		mapHeader->players[i].canComputerPlay = isPlayable;
	}

	// Populate required mods
	if (!data["requiredMods"].isNull())
	{
		for (const auto & modNode : data["requiredMods"].Vector())
		{
			ModVerificationInfo modInfo;
			modInfo.name = modNode["name"].String();
			if (!modNode["version"].isNull())
				modInfo.version = CModVersion::fromString(modNode["version"].String());
			mapHeader->mods[modInfo.name] = modInfo;
		}
	}

	countPlayers();
}

void CMapInfo::countPlayers()
{
	for(int i=0; i<PlayerColor::PLAYER_LIMIT_I; i++)
	{
		if(mapHeader->players[i].canHumanPlay)
		{
			amountOfPlayersOnMap++;
			amountOfHumanControllablePlayers++;
		}
		else if(mapHeader->players[i].canComputerPlay)
		{
			amountOfPlayersOnMap++;
		}
	}

	if(scenarioOptionsOfSave)
		for(const auto & playerInfo : scenarioOptionsOfSave->playerInfos)
			if(playerInfo.second.isControlledByHuman())
				amountOfHumanPlayersInSave++;
}

std::string CMapInfo::getNameTranslated() const
{
	if(campaign && !campaign->getNameTranslated().empty())
		return campaign->getNameTranslated();
	else if(mapHeader && !mapHeader->name.empty())
	{
		mapHeader->registerMapStrings();
		return mapHeader->name.toString();
	}
	else
		return LIBRARY->generaltexth->allTexts[508];
}

std::string CMapInfo::getNameForList() const
{
	if(scenarioOptionsOfSave)
	{
		// TODO: this could be handled differently
		std::vector<std::string> path;
		boost::split(path, originalFileURI, boost::is_any_of("\\/"));
		return path[path.size()-1];
	}
	else
	{
		return getNameTranslated();
	}
}

std::string CMapInfo::getDescriptionTranslated() const
{
	if(campaign)
		return campaign->getDescriptionTranslated();
	else
		return mapHeader->description.toString();
}

int CMapInfo::getMapSizeIconId() const
{
	if(!mapHeader)
		return 4;

	switch(mapHeader->width)
	{
	case CMapHeader::MAP_SIZE_SMALL:
		return 0;
	case CMapHeader::MAP_SIZE_MIDDLE:
		return 1;
	case CMapHeader::MAP_SIZE_LARGE:
		return 2;
	case CMapHeader::MAP_SIZE_XLARGE:
		return 3;
	case CMapHeader::MAP_SIZE_HUGE:
		return 4;
	case CMapHeader::MAP_SIZE_XHUGE:
		return 5;
	case CMapHeader::MAP_SIZE_GIANT:
		return 6;
	default:
		return 4;
	}
}

int CMapInfo::getMapSizeFormatIconId() const
{
	switch(mapHeader->version)
	{
		case EMapFormat::ROE:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA)["iconIndex"].Integer();
		case EMapFormat::AB:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE)["iconIndex"].Integer();
		case EMapFormat::SOD:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH)["iconIndex"].Integer();
		case EMapFormat::CHR:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_CHRONICLES)["iconIndex"].Integer();
		case EMapFormat::WOG:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS)["iconIndex"].Integer();
		case EMapFormat::HOTA:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS)["iconIndex"].Integer();
		case EMapFormat::VCMI:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_JSON_VCMI)["iconIndex"].Integer();
	}
	return 0;
}

std::string CMapInfo::getMapSizeName() const
{
	switch(mapHeader->width)
	{
	case CMapHeader::MAP_SIZE_SMALL:
		return "S";
	case CMapHeader::MAP_SIZE_MIDDLE:
		return "M";
	case CMapHeader::MAP_SIZE_LARGE:
		return "L";
	case CMapHeader::MAP_SIZE_XLARGE:
		return "XL";
	case CMapHeader::MAP_SIZE_HUGE:
		return "H";
	case CMapHeader::MAP_SIZE_XHUGE:
		return "XH";
	case CMapHeader::MAP_SIZE_GIANT:
		return "G";
	default:
		return "C";
	}
}
