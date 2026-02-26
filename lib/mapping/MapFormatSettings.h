/*
 * MapFormatSettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "MapIdentifiersH3M.h"
#include "MapFormat.h"
#include "../campaign/CampaignConstants.h"
#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class MapFormatSettings : boost::noncopyable
{
	static MapIdentifiersH3M generateMapping(EMapFormat format);
	static std::map<EMapFormat, MapIdentifiersH3M> generateMappings();
	static std::map<CampaignVersion, EMapFormat> generateCampaignMapping();

	std::map<EMapFormat, MapIdentifiersH3M> mapping;
	std::map<CampaignVersion, EMapFormat> campaignToMap;

	JsonNode campaignOverridesConfig;
	JsonNode mapOverridesConfig;
public:
	MapFormatSettings();

	bool isSupported(EMapFormat format) const
	{
		return mapping.count(format) != 0;
	}

	bool isSupported(CampaignVersion format) const
	{
		return isSupported(campaignToMap.at(format));
	}

	const MapIdentifiersH3M & getMapping(EMapFormat format) const
	{
		return mapping.at(format);
	}

	const MapIdentifiersH3M & getMapping(CampaignVersion format) const
	{
		return mapping.at(campaignToMap.at(format));
	}

	const JsonNode & campaignOverrides(const std::string & campaignName)
	{
		return campaignOverridesConfig[campaignName];
	}

	const JsonNode & mapOverrides(const std::string & mapName)
	{
		return mapOverridesConfig[mapName];
	}
};

VCMI_LIB_NAMESPACE_END
