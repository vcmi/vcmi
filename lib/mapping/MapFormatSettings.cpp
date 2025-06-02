/*
 * MapFormatSettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapFormatSettings.h"

#include "MapFeaturesH3M.h"

#include "../GameLibrary.h"
#include "../IGameSettings.h"
#include "../json/JsonUtils.h"
#include "../modding/ModScope.h"

VCMI_LIB_NAMESPACE_BEGIN

MapIdentifiersH3M MapFormatSettings::generateMapping(EMapFormat format)
{
	auto features = MapFormatFeaturesH3M::find(format, 0);
	MapIdentifiersH3M identifierMapper;

	if(features.levelROE)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA));
	if(features.levelAB)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE));
	if(features.levelSOD)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH));
	if(features.levelCHR)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_CHRONICLES));
	if(features.levelWOG)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS));
	if(features.levelHOTA0)
		identifierMapper.loadMapping(LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS));

	return identifierMapper;
}

std::map<CampaignVersion, EMapFormat> MapFormatSettings::generateCampaignMapping()
{
	return {
		{CampaignVersion::RoE,  EMapFormat::ROE },
		{CampaignVersion::AB,   EMapFormat::AB  },
		{CampaignVersion::SoD,  EMapFormat::SOD },
		{CampaignVersion::WoG,  EMapFormat::WOG },
		{CampaignVersion::Chr,  EMapFormat::CHR },
		{CampaignVersion::HotA, EMapFormat::HOTA}
	};
}

std::map<EMapFormat, MapIdentifiersH3M> MapFormatSettings::generateMappings()
{
	std::map<EMapFormat, MapIdentifiersH3M> result;
	auto addMapping = [&result](EMapFormat format)
	{
		try
		{
			result[format] = generateMapping(format);
			logMod->trace("Loaded map format support for %d", static_cast<int>(format));
		}
		catch(const std::runtime_error &)
		{
			// unsupported map format - skip
			logMod->debug("Failed to load map format support for %d", static_cast<int>(format));
		}
	};

	addMapping(EMapFormat::ROE);
	addMapping(EMapFormat::AB);
	addMapping(EMapFormat::SOD);
	addMapping(EMapFormat::CHR);
	addMapping(EMapFormat::HOTA);
	addMapping(EMapFormat::WOG);

	return result;
}

MapFormatSettings::MapFormatSettings()
	: mapping(generateMappings())
	, campaignToMap(generateCampaignMapping())
	, campaignOverridesConfig(JsonUtils::assembleFromFiles("config/campaignOverrides.json"))
	, mapOverridesConfig(JsonUtils::assembleFromFiles("config/mapOverrides.json"))
{
	for (auto & entry : mapOverridesConfig.Struct())
		JsonUtils::validate(entry.second, "vcmi:mapHeader", "patch for " + entry.first);

	campaignOverridesConfig.setModScope(ModScope::scopeMap());
	mapOverridesConfig.setModScope(ModScope::scopeMap());
}

VCMI_LIB_NAMESPACE_END
