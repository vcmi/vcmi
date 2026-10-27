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
#include "../serializer/CLegacyLoadFile.h"
#include "../serializer/CLoadFile.h"
#include "../serializer/CMemorySerializer.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextOperations.h"
#include "../CCreatureHandler.h"
#include "../IGameSettings.h"
#include "../CConfigHandler.h"

CMapInfo::CMapInfo()
	: isRandomMap(false)
{

}

CMapInfo::~CMapInfo() = default;


void CMapInfo::mapInit(const std::string & fname)
{
	fileURI = fname;
	CMapService mapService;
	ResourcePath resource = ResourcePath(fname, EResType::MAP);
	originalFileURI = resource.getOriginalName();
	mapHeader = mapService.loadMapHeader(resource);
	mapHeader->registerMapStrings();
	lastWrite = CResourceHandler::get()->getLastWriteTime(resource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);
}

void CMapInfo::saveInit(const ResourcePath & file)
{
	const auto saveFileName = CResourceHandler::get()->getResourceName(file);
	if(!saveFileName)
		throw std::runtime_error("Error: saved game '" + file.getOriginalName() + "' not found!");

	const auto & saveFile = *saveFileName;

	mapHeader = std::make_unique<CMapHeader>();
	scenarioOptionsOfSave = std::make_unique<StartInfo>();

	if(CLoadFile::readVersion(saveFile) < ESerializationVersion::SAVE_CHUNKS)
	{
		CLegacyLoadFile lf(saveFile, nullptr);
		lf.loadHeader(*mapHeader, *scenarioOptionsOfSave);
	}
	else
	{
		CLoadFile lf(saveFile);
		auto header = lf.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
		header->iser & *mapHeader;
		header->iser & *scenarioOptionsOfSave;
		if(!header->isFullyRead())
			throw std::runtime_error("Error: saved game '" + file.getOriginalName() + "' contains unexpected data after map header!");
	}
	mapHeader->registerMapStrings();

	fileURI = file.getName(); // Name without file extension
	originalFileURI = file.getOriginalName(); // Same as file.getName() but keep letter case
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
	campaign = CampaignHandler::getHeader(fileURI);
	lastWrite = CResourceHandler::get()->getLastWriteTime(resource);
	date = TextOperations::getFormattedDateTimeLocal(lastWrite);
}

std::string CMapInfo::getNameTranslated(const ITranslator * translator) const
{
	if(campaign && !campaign->getNameTranslated(translator).empty())
		return campaign->getNameTranslated(translator);
	else if(mapHeader && !mapHeader->name.empty())
		return mapHeader->name.toString(translator);
	else
		return LIBRARY->generaltexth->allTexts[508];
}

std::string CMapInfo::getDescriptionTranslated(const ITranslator * translator) const
{
	if(campaign)
		return campaign->getDescriptionTranslated(translator);
	else
		return mapHeader->description.toString(translator);
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

