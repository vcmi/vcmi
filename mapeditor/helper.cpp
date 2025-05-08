/*
 * helper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "helper.h"
#include "mapcontroller.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CMemoryBuffer.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/CZipSaver.h"
#include "../lib/campaign/CampaignHandler.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/MapFormatJson.h"
#include "../lib/modding/ModIncompatibility.h"

std::unique_ptr<CMap> Helper::openMapInternal(const QString & filenameSelect)
{
	QFileInfo fi(filenameSelect);
	std::string fname = fi.fileName().toStdString();
	std::string fdir = fi.dir().path().toStdString();
	
	ResourcePath resId("MAPEDITOR/" + fname, EResType::MAP);
	
	//addFilesystem takes care about memory deallocation if case of failure, no memory leak here
	auto mapEditorFilesystem = std::make_unique<CFilesystemLoader>("MAPEDITOR/", fdir, 0);
	CResourceHandler::removeFilesystem("local", "mapEditor");
	CResourceHandler::addFilesystem("local", "mapEditor", std::move(mapEditorFilesystem));
	
	if(!CResourceHandler::get("mapEditor")->existsResource(resId))
		throw std::runtime_error("Cannot open map from this folder");
	
	CMapService mapService;
	if(auto header = mapService.loadMapHeader(resId))
	{
		auto missingMods = CMapService::verifyMapHeaderMods(*header);
		ModIncompatibility::ModList modList;
		for(const auto & m : missingMods)
			modList.push_back(m.second.name);
		
		if(!modList.empty())
			throw ModIncompatibility(modList);
		
		return mapService.loadMap(resId, nullptr);
	}
	else
		throw std::runtime_error("Corrupted map");
}

std::shared_ptr<CampaignState> Helper::openCampaignInternal(const QString & filenameSelect)
{
	QFileInfo fi(filenameSelect);
	std::string fname = fi.fileName().toStdString();
	std::string fdir = fi.dir().path().toStdString();
	
	ResourcePath resId("MAPEDITOR/" + fname, EResType::CAMPAIGN);
	
	//addFilesystem takes care about memory deallocation if case of failure, no memory leak here
	auto mapEditorFilesystem = std::make_unique<CFilesystemLoader>("MAPEDITOR/", fdir, 0);
	CResourceHandler::removeFilesystem("local", "mapEditor");
	CResourceHandler::addFilesystem("local", "mapEditor", std::move(mapEditorFilesystem));
	
	if(!CResourceHandler::get("mapEditor")->existsResource(resId))
		throw std::runtime_error("Cannot open campaign from this folder");
	if(auto campaign = CampaignHandler::getCampaign(resId.getName()))
		return campaign;
	else
		throw std::runtime_error("Corrupted campaign");
}

void Helper::saveCampaign(std::shared_ptr<CampaignState> campaignState, const QString & filename)
{
	auto jsonCampaign = CampaignHandler::writeHeaderToJson(*campaignState);
	
	auto io = std::make_shared<CDefaultIOApi>();
	auto saver = std::make_shared<CZipSaver>(io, filename.toStdString());
	for(auto & scenario : campaignState->allScenarios())
	{
		auto map = campaignState->getMap(scenario, nullptr);
		MapController::repairMap(map.get());
		CMemoryBuffer serializeBuffer;
		{
			CMapSaverJson jsonSaver(&serializeBuffer);
			jsonSaver.saveMap(map);
		}

		auto mapName = boost::algorithm::to_lower_copy(campaignState->scenario(scenario).mapName);
		if(!boost::ends_with(mapName, ".vmap"))
			mapName = boost::replace_all_copy(mapName, ".h3m", std::string("")) + ".vmap";

		auto stream = saver->addFile(mapName);
		stream->write(reinterpret_cast<const ui8 *>(serializeBuffer.getBuffer().data()), serializeBuffer.getSize());

		jsonCampaign["scenarios"].Vector().push_back(CampaignHandler::writeScenarioToJson(campaignState->scenario(scenario)));
		jsonCampaign["scenarios"].Vector().back()["map"].String() = mapName;
	}

	auto jsonCampaignStr = jsonCampaign.toString();
	saver->addFile("header.json")->write(reinterpret_cast<const ui8 *>(jsonCampaignStr.data()), jsonCampaignStr.length());
}
