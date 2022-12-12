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

#include "../filesystem/ResourceID.h"
#include "../StartInfo.h"
#include "../GameConstants.h"
#include "CMapService.h"

#include "../filesystem/Filesystem.h"
#include "../serializer/CMemorySerializer.h"
#include "../CGeneralTextHandler.h"
#include "../rmg/CMapGenOptions.h"
#include "../CCreatureHandler.h"
#include "../CHeroHandler.h"
#include "../CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapInfo::CMapInfo()
	: scenarioOptionsOfSave(nullptr), amountOfPlayersOnMap(0), amountOfHumanControllablePlayers(0),	amountOfHumanPlayersInSave(0), isRandomMap(false)
{

}

CMapInfo::~CMapInfo()
{
	vstd::clear_pointer(scenarioOptionsOfSave);
}

void CMapInfo::mapInit(const std::string & fname)
{
	fileURI = fname;
	CMapService mapService;
	mapHeader = mapService.loadMapHeader(ResourceID(fname, EResType::MAP));
	countPlayers();
}

void CMapInfo::saveInit(ResourceID file)
{
	CLoadFile lf(*CResourceHandler::get()->getResourceName(file), MINIMAL_SERIALIZATION_VERSION);
	lf.checkMagicBytes(SAVEGAME_MAGIC);

	mapHeader = make_unique<CMapHeader>();
	lf >> *(mapHeader.get()) >> scenarioOptionsOfSave;
	fileURI = file.getName();
	countPlayers();
	std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(file));
	date = std::asctime(std::localtime(&time));
	// We absolutely not need this data for lobby and server will read it from save
	// FIXME: actually we don't want them in CMapHeader!
	mapHeader->triggeredEvents.clear();
}

void CMapInfo::campaignInit()
{
	campaignHeader = std::unique_ptr<CCampaignHeader>(new CCampaignHeader(CCampaignHandler::getHeader(fileURI)));
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
		for (auto i = scenarioOptionsOfSave->playerInfos.cbegin(); i != scenarioOptionsOfSave->playerInfos.cend(); i++)
			if(i->second.isControlledByHuman())
				amountOfHumanPlayersInSave++;
}

std::string CMapInfo::getName() const
{
	if(campaignHeader && campaignHeader->name.length())
		return campaignHeader->name;
	else if(mapHeader && mapHeader->name.length())
		return mapHeader->name;
	else
		return VLC->generaltexth->allTexts[508];
}

std::string CMapInfo::getNameForList() const
{
	if(scenarioOptionsOfSave)
	{
		// TODO: this could be handled differently
		std::vector<std::string> path;
		boost::split(path, fileURI, boost::is_any_of("\\/"));
		return path[path.size()-1];
	}
	else
	{
		return getName();
	}
}

std::string CMapInfo::getDescription() const
{
	if(campaignHeader)
		return campaignHeader->description;
	else
		return mapHeader->description;
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

std::pair<int, int> CMapInfo::getMapSizeFormatIconId() const
{
	int frame = -1, group = 0;
	switch(mapHeader->version)
	{
	case EMapFormat::ROE:
		frame = 0;
		break;
	case EMapFormat::AB:
		frame = 1;
		break;
	case EMapFormat::SOD:
		frame = 2;
		break;
	case EMapFormat::WOG:
		frame = 3;
		break;
	case EMapFormat::VCMI:
		frame = 0;
		group = 1;
		break;
	default:
		// Unknown version. Be safe and ignore that map
		//logGlobal->warn("Warning: %s has wrong version!", currentItem->fileURI);
		break;
	}
	return std::make_pair(frame, group);
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

VCMI_LIB_NAMESPACE_END
