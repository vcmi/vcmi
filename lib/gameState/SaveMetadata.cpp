/*
 * SaveMetadata.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SaveMetadata.h"

#include "CGameState.h"

#include "../GameLibrary.h"
#include "../StartInfo.h"
#include "../campaign/CampaignState.h"
#include "../constants/StringConstants.h"
#include "../json/JsonValidator.h"
#include "../mapping/CMap.h"
#include "../mapping/ScenarioMetadata.h"
#include "../modding/CModHandler.h"
#include "../modding/ModDescription.h"
#include "../modding/ModIncompatibility.h"
#include "../texts/MetaString.h"
#include "../texts/TextLocalizationContainer.h"

static JsonNode gameToJson(const StartInfo & startInfo)
{
	int humanPlayers = 0;
	for(const auto & player : startInfo.playerInfos)
		if(player.second.isControlledByHuman())
			++humanPlayers;

	JsonNode result;
	if(startInfo.campState)
	{
		const std::string language = LIBRARY->modh->getModLanguage(startInfo.campState->getCampaign().getModName());
		result["mode"].String() = "campaign";
		result["campaignName"] = ScenarioMetadata::textToJson(startInfo.campState->getCampaign().getName(), {{language, *startInfo.campState->getCampaign().getTexts()}});
		result["campaignScenario"].Integer() = startInfo.campState->currentScenario().value().getNum();
	}
	else if(boost::iequals(startInfo.mapname, "Maps/Tutorial"))
		result["mode"].String() = "tutorial";
	else
		result["mode"].String() = "single";

	result["mapFile"].String() = startInfo.mapname;
	result["difficulty"].Integer() = startInfo.difficulty;
	result["humanPlayers"].Integer() = humanPlayers;
	result["saveDirectory"].String() = startInfo.saveDirectory;
	return result;
}

JsonNode SaveMetadata::create(const CGameState & gameState)
{
	JsonNode result;
	result["vcmiVersion"].String() = GameConstants::VCMI_VERSION;
	result["saveTime"].Integer() = std::time(nullptr);
	result["map"] = ScenarioMetadata::mapToJson(gameState.getMap());
	result["game"] = gameToJson(*gameState.getStartInfo());
	result["game"]["day"].Integer() = gameState.day;
	result["mods"] = ScenarioMetadata::activeModsToJson();
	return result;
}

JsonNode SaveMetadata::createLegacy(const CMapHeader & mapHeader, const StartInfo & startInfo, const ModCompatibilityInfo & mods)
{
	JsonNode result;
	result["map"] = ScenarioMetadata::mapToJson(mapHeader);
	result["game"] = gameToJson(startInfo);
	result["mods"] = ScenarioMetadata::modsToJson(mods);
	return result;
}

JsonNode SaveMetadata::parse(const std::vector<std::byte> & data, const std::string & fileName, JsonValidator & validator)
{
	JsonNode result(data.data(), data.size(), fileName);

	const std::string errors = validator.check("vcmi:scenarioMetadata", result);
	if(!errors.empty())
		throw std::runtime_error("Error: metadata of saved game '" + fileName + "' is invalid: " + errors);

	for(const auto & field : {"vcmiVersion", "saveTime", "game"})
		if(!result.Struct().contains(field))
			throw std::runtime_error("Error: metadata of saved game '" + fileName + "' has no field '" + field + "'!");

	return result;
}

void SaveMetadata::verifyMods(const JsonNode & metadata)
{
	ModIncompatibility::ModList missingMods;
	ModIncompatibility::ModList excessiveMods;

	for(const auto & [modID, status] : ScenarioMetadata::verifyMods(metadata["mods"], true))
	{
		if(status == ModVerificationStatus::NOT_INSTALLED)
			missingMods.push_back(metadata["mods"][modID]["name"].String());
		else if(status == ModVerificationStatus::EXCESSIVE)
			excessiveMods.push_back(LIBRARY->modh->getModInfo(modID).getName());
		else
			missingMods.push_back(LIBRARY->modh->getModInfo(modID).getName());
	}

	if(!missingMods.empty() || !excessiveMods.empty())
		throw ModIncompatibility(missingMods, excessiveMods);
}
