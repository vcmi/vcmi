/*
 * EditorFiles.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorFiles.h"

#include "EditorMap.h"

#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/filesystem/CZipSaver.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/modding/ModIncompatibility.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../../lib/serializer/JsonSerializer.h"
#include "../../lib/texts/CompositeTranslator.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace
{
ResourcePath addFilesystemAndGetResource(const boost::filesystem::path & file, EResType type, const std::string & typeName)
{
	const std::string fileName = TextOperations::filesystemPathToUtf8(file.filename());
	const auto directory = file.parent_path();

	ResourcePath resId("MAPEDITOR/" + fileName, type);

	//addFilesystem takes care about memory deallocation if case of failure, no memory leak here
	auto mapEditorFilesystem = std::make_unique<CFilesystemLoader>("MAPEDITOR/", directory, 0);
	CResourceHandler::removeFilesystem("local", "mapEditor");
	CResourceHandler::addFilesystem("local", "mapEditor", std::move(mapEditorFilesystem));

	if(!CResourceHandler::get("mapEditor")->existsResource(resId))
		throw std::runtime_error("Cannot open " + typeName + " from this folder");

	return resId;
}
}

namespace EditorFiles
{

std::unique_ptr<CMap> openMap(const boost::filesystem::path & file, IGameInfoCallback * cb)
{
	auto resId = addFilesystemAndGetResource(file, EResType::MAP, "map");

	CMapService mapService;
	if(auto header = mapService.loadMapHeader(resId, true))
	{
		auto missingMods = CMapService::verifyMapHeaderMods(*header);
		ModIncompatibility::ModList modList;
		for(const auto & m : missingMods)
			modList.push_back(m.second.name);

		if(!modList.empty())
			throw ModIncompatibility(modList);

		return mapService.loadMap(resId, cb);
	}
	else
		throw std::runtime_error("Corrupted map");
}

std::shared_ptr<CampaignState> openCampaign(const boost::filesystem::path & file)
{
	auto resId = addFilesystemAndGetResource(file, EResType::CAMPAIGN, "campaign");

	if(auto campaign = CampaignHandler::getCampaign(resId.getName()))
		return campaign;
	else
		throw std::runtime_error("Corrupted campaign");
}

std::map<std::string, std::shared_ptr<CRmgTemplate>> openTemplate(const boost::filesystem::path & file)
{
	auto resId = addFilesystemAndGetResource(file, EResType::JSON, "template");

	auto data = CResourceHandler::get()->load(resId)->readAll();
	JsonNode nodes(reinterpret_cast<std::byte *>(data.first.get()), data.second, resId.getName());

	nodes.setModScope(ModScope::scopeGame());

	std::map<std::string, std::shared_ptr<CRmgTemplate>> templates;
	for(auto & node : nodes.Struct())
	{
		JsonDeserializer handler(nullptr, node.second);
		auto rmg = std::make_shared<CRmgTemplate>();
		rmg->serializeJson(handler);
		rmg->validate();
		templates[node.first] = rmg;
	}

	return templates;
}

void saveMap(const std::unique_ptr<CMap> & map, const boost::filesystem::path & file)
{
	CMapService mapService;
	mapService.saveMap(map, file);
}

void saveCampaign(const std::shared_ptr<CampaignState> & campaignState, const boost::filesystem::path & file)
{
	CompositeTranslator translator;
	translator.install(campaignState->getTexts());
	auto jsonCampaign = CampaignHandler::writeHeaderToJson(*campaignState, &translator);

	auto io = std::make_shared<CDefaultIOApi>();
	auto saver = std::make_shared<CZipSaver>(io, file);
	for(auto & scenario : campaignState->allScenarios())
	{
		EditorCallback cb(nullptr);
		auto map = campaignState->getMap(scenario, &cb);
		cb.setMap(map.get());
		EditorMap::repairMap(map.get());
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

		jsonCampaign["scenarios"].Vector().push_back(CampaignHandler::writeScenarioToJson(campaignState->scenario(scenario), &translator));
		jsonCampaign["scenarios"].Vector().back()["map"].String() = mapName;
	}

	auto jsonCampaignStr = jsonCampaign.toString();
	saver->addFile("header.json")->write(reinterpret_cast<const ui8 *>(jsonCampaignStr.data()), jsonCampaignStr.length());
}

void saveTemplate(const std::map<std::string, std::shared_ptr<CRmgTemplate>> & templates, const boost::filesystem::path & file)
{
	JsonMap data;

	for(auto & node : templates)
	{
		JsonNode actual;
		{
			JsonSerializer handler(nullptr, actual);
			node.second->serializeJson(handler);
		}
		data[node.first] = actual;
	}

	auto bytes = JsonNode(data).toBytes();

	std::ofstream stream(file.c_str(), std::ios::binary | std::ios::trunc);
	if(!stream)
		throw std::runtime_error("Cannot write " + TextOperations::filesystemPathToUtf8(file));

	stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

}
