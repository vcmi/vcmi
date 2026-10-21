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

/// Returns all active mods, gameplay-affecting or not
static ModCompatibilityInfo getActiveMods()
{
	ModCompatibilityInfo result;
	for(const auto & modID : LIBRARY->modh->getActiveMods())
		result[modID] = LIBRARY->modh->getModInfo(modID).getVerificationInfo();
	return result;
}

static JsonNode gameToJson(const StartInfo & startInfo)
{
	int humanPlayers = 0;
	for(const auto & player : startInfo.playerInfos)
		if(player.second.isControlledByHuman())
			++humanPlayers;

	JsonNode result;
	if(startInfo.campState)
	{
		const std::string language = LIBRARY->modh->getModLanguage(startInfo.campState->getModName());
		result["mode"].String() = "campaign";
		result["campaignName"] = ScenarioMetadata::textToJson(startInfo.campState->getName(), {{language, *startInfo.campState->getTexts()}});
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
	result["mods"] = ScenarioMetadata::modsToJson(getActiveMods());
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

JsonNode SaveMetadata::parse(const std::vector<std::byte> & data, const std::string & fileName)
{
	JsonNode result(data.data(), data.size(), fileName);

	JsonValidator validator;
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
	const JsonNode localMods = ScenarioMetadata::modsToJson(getActiveMods());
	const auto & savedMods = metadata["mods"].Struct();
	const auto installedMods = LIBRARY->modh->getAllMods();
	const auto & activeMods = LIBRARY->modh->getActiveMods();

	// local version of a mod may no longer affect gameplay, in which case it does not need to be enabled
	const auto affectsGameplay = [&installedMods](const TModID & rootID)
	{
		return std::ranges::any_of(installedMods, [&rootID](const TModID & modID)
		{
			const auto & mod = LIBRARY->modh->getModInfo(modID);
			return (modID == rootID || mod.getTopParentID() == rootID) && mod.affectsGameplay();
		});
	};

	ModIncompatibility::ModList missingMods;
	ModIncompatibility::ModList excessiveMods;

	// only root mods are compared, since list of submods often changes between versions of a mod
	for(const auto & [modID, savedMod] : savedMods)
	{
		if(!vstd::contains(installedMods, modID))
			missingMods.push_back(savedMod["name"].String());
		else if(!vstd::contains(activeMods, modID) && affectsGameplay(modID))
			missingMods.push_back(LIBRARY->modh->getModInfo(modID).getName());
	}

	for(const auto & [modID, localMod] : localMods.Struct())
		if(!savedMods.contains(modID))
			excessiveMods.push_back(LIBRARY->modh->getModInfo(modID).getName());

	if(!missingMods.empty() || !excessiveMods.empty())
		throw ModIncompatibility(missingMods, excessiveMods);
}
