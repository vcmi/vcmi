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
	fileMetadataResource = resource;
	mapHeader = mapService.loadMapHeader(resource);
	countPlayers();
	mapEntry = std::make_unique<MapListEntry>(mapHeader->makeListEntry());
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
	fileMetadataResource = file;
	countPlayers();
	mapEntry = std::make_unique<MapListEntry>(mapHeader->makeListEntry());

	// We absolutely not need this data for lobby and server will read it from save
	// FIXME: actually we don't want them in CMapHeader!
	mapHeader->triggeredEvents.clear();
}

void CMapInfo::campaignInit()
{
	ResourcePath resource = ResourcePath(fileURI, EResType::CAMPAIGN);
	originalFileURI = resource.getOriginalName();
	fileMetadataResource = resource;
	campaign = CampaignHandler::getHeader(fileURI);
	campaignEntry = std::make_unique<CampaignListEntry>(campaign->makeListEntry());
}

void CMapInfo::initFromCache(const std::string & fileURI_, BinaryDeserializer & h)
{
	fileURI = fileURI_;

	mapEntry = std::make_unique<MapListEntry>();
	h & *mapEntry;
	amountOfPlayersOnMap = mapEntry->amountOfPlayersOnMap;
	amountOfHumanControllablePlayers = mapEntry->amountOfHumanControllablePlayers;

	mapHeader = [this]() {
		CMapService mapService;
		return mapService.loadMapHeader(ResourcePath(fileURI, EResType::MAP));
	};

	ResourcePath resource = ResourcePath(fileURI, EResType::MAP);
	originalFileURI = resource.getOriginalName();
	fileMetadataResource = resource;
}

void CMapInfo::initCampaignFromCache(const std::string & fileURI_, BinaryDeserializer & h)
{
	fileURI = fileURI_;

	campaignEntry = std::make_unique<CampaignListEntry>();
	h & *campaignEntry;

	campaign = [this]() {
		return CampaignHandler::getHeader(fileURI);
	};

	ResourcePath resource = ResourcePath(fileURI, EResType::CAMPAIGN);
	originalFileURI = resource.getOriginalName();
	fileMetadataResource = resource;
}

void CMapInfo::ensureFileMetadata() const
{
	if (fileMetadataLoaded || fileMetadataResource.empty())
		return;

	fullFileURI = CResourceHandler::get()->getFullFileURI(fileMetadataResource);
	lastWrite = CResourceHandler::get()->getLastWriteTime(fileMetadataResource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);
	fileMetadataLoaded = true;
}

const std::string & CMapInfo::getFullFileURI() const
{
	ensureFileMetadata();
	return fullFileURI;
}

std::time_t CMapInfo::getLastWrite() const
{
	ensureFileMetadata();
	return lastWrite;
}

const std::string & CMapInfo::getDate() const
{
	ensureFileMetadata();
	return date;
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

	if(campaignEntry)
		return campaignEntry->name.toString();

	if(mapEntry)
		return mapEntry->name.toString();

	return getNameTranslated();
}

std::string CMapInfo::getDescriptionTranslated() const
{
	if(campaign)
		return campaign->getDescriptionTranslated();
	return mapHeader->description.toString();
}

int CMapInfo::getMapSizeIconId() const
{
	if(!mapEntry && !mapHeader)
		return 4;

	const si32 width = mapEntry ? mapEntry->width : mapHeader->width;
	switch(width)
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
	const EMapFormat version = mapEntry ? mapEntry->version : mapHeader->version;
	switch(version)
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
	const si32 width = mapEntry ? mapEntry->width : mapHeader->width;
	switch(width)
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
