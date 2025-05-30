/*
 * CampaignRegionsHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CampaignRegionsHandler.h"

#include "../json/JsonNode.h"

std::vector<JsonNode> CampaignRegionsHandler::loadLegacyData()
{
	return {};
}

void CampaignRegionsHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = std::make_shared<CampaignRegions>(data);
	registerObject(scope, "campaignRegion", name, objects.size());
	objects.push_back(object);
}

void CampaignRegionsHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("CampaignRegionsHandler::loadObject - load by index is not supported!");
}
