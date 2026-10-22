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
#include "../modding/CModHandler.h"
#include "../modding/ModDescription.h"
#include "../modding/ModIncompatibility.h"
#include "../modding/ModScope.h"
#include "../texts/MetaString.h"
#include "../texts/TextLocalizationContainer.h"

/// Returns active mods that affect gameplay: root mod -> its gameplay-affecting submods, as paths relative to the root.
/// Root mod is present if it affects gameplay itself or has at least one such submod
static std::map<TModID, std::vector<std::string>> getActiveGameplayMods()
{
	std::map<TModID, std::vector<std::string>> result;

	for(const auto & modID : LIBRARY->modh->getActiveMods())
	{
		if(modID == ModScope::scopeBuiltin())
			continue;

		const auto & mod = LIBRARY->modh->getModInfo(modID);
		if(!mod.affectsGameplay())
			continue;

		const TModID rootID = mod.getTopParentID();
		if(rootID.empty())
			result[modID];
		else
			result[rootID].push_back(modID.substr(rootID.size() + 1));
	}
	return result;
}

/// Stores text in form that can be displayed without texts of the map or campaign:
/// the string itself and its text in every language for which translator is provided
static JsonNode textToJson(const MetaString & text, const std::map<std::string, TextLocalizationContainer> & translators)
{
	JsonNode result;
	text.jsonSerialize(result["text"]);
	result["translations"].Struct();

	const auto & textIDs = result["text"]["stringsTextID"].Vector();
	if(textIDs.empty() || !result["text"]["localStrings"].Vector().empty())
		return result;

	for(const auto & [language, translator] : translators)
	{
		bool translated = std::ranges::all_of(textIDs, [&translator](const JsonNode & textID)
		{
			return translator.identifierExists(TextIdentifier(textID.String()));
		});

		if(translated)
			result["translations"][language].String() = text.toString(&translator);
	}
	return result;
}

static JsonNode mapToJson(const CMapHeader & header)
{
	std::map<std::string, TextLocalizationContainer> translators;
	for(const auto & [language, strings] : header.translations.Struct())
		for(const auto & [textID, localized] : strings.Struct())
			translators[language].registerString(ModScope::scopeMap(), TextIdentifier(textID), localized.String());

	int players = 0;
	int humanPlayers = 0;
	for(const auto & player : header.players)
	{
		if(player.canHumanPlay)
			++humanPlayers;
		if(player.canAnyonePlay())
			++players;
	}

	JsonNode result;
	result["name"] = textToJson(header.name, translators);
	result["description"] = textToJson(header.description, translators);
	result["author"] = textToJson(header.author, translators);
	result["authorContact"] = textToJson(header.authorContact, translators);
	result["mapVersion"] = textToJson(header.mapVersion, translators);
	result["format"].Integer() = static_cast<int>(header.version);
	result["width"].Integer() = header.width;
	result["height"].Integer() = header.height;
	result["levels"].Integer() = header.levels();
	result["difficulty"].Integer() = static_cast<int>(header.difficulty);
	result["victoryIcon"].Integer() = header.victoryIconIndex;
	result["defeatIcon"].Integer() = header.defeatIconIndex;
	result["players"].Integer() = players;
	result["humanPlayers"].Integer() = humanPlayers;
	return result;
}

static JsonNode gameToJson(const CGameState & gameState)
{
	const StartInfo & startInfo = *gameState.getStartInfo();

	int humanPlayers = 0;
	for(const auto & player : startInfo.playerInfos)
		if(player.second.isControlledByHuman())
			++humanPlayers;

	JsonNode result;
	if(startInfo.campState)
	{
		const std::string language = LIBRARY->modh->getModLanguage(startInfo.campState->getModName());
		result["mode"].String() = "campaign";
		result["campaignName"] = textToJson(startInfo.campState->getName(), {{language, *startInfo.campState->getTexts()}});
	}
	else if(boost::iequals(startInfo.mapname, "Maps/Tutorial"))
		result["mode"].String() = "tutorial";
	else
		result["mode"].String() = "single";

	result["mapFile"].String() = startInfo.mapname;
	result["difficulty"].Integer() = startInfo.difficulty;
	result["humanPlayers"].Integer() = humanPlayers;
	result["saveDirectory"].String() = startInfo.saveDirectory;
	result["day"].Integer() = gameState.day;
	return result;
}

static JsonNode modsToJson()
{
	JsonNode result;
	result.Struct();

	for(const auto & [rootID, submods] : getActiveGameplayMods())
	{
		const auto & mod = LIBRARY->modh->getModInfo(rootID);
		JsonNode & entry = result[rootID];
		entry["name"].String() = mod.getName();
		entry["version"].String() = mod.getVersion().toString();
		entry["submods"].Vector();
		for(const auto & submod : submods)
			entry["submods"].Vector().emplace_back(submod);
	}
	return result;
}

JsonNode SaveMetadata::create(const CGameState & gameState)
{
	JsonNode result;
	result["vcmiVersion"].String() = GameConstants::VCMI_VERSION;
	result["saveTime"].Integer() = std::time(nullptr);
	result["map"] = mapToJson(gameState.getMap());
	result["game"] = gameToJson(gameState);
	result["mods"] = modsToJson();
	return result;
}

JsonNode SaveMetadata::parse(const std::vector<std::byte> & data, const std::string & fileName)
{
	JsonNode result(data.data(), data.size(), fileName);

	JsonValidator validator;
	const std::string errors = validator.check("vcmi:saveMetadata", result);
	if(!errors.empty())
		throw std::runtime_error("Error: metadata of saved game '" + fileName + "' is invalid: " + errors);

	return result;
}

void SaveMetadata::verifyMods(const JsonNode & metadata)
{
	const auto localMods = getActiveGameplayMods();
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

	for(const auto & [modID, submods] : localMods)
		if(!savedMods.contains(modID))
			excessiveMods.push_back(LIBRARY->modh->getModInfo(modID).getName());

	if(!missingMods.empty() || !excessiveMods.empty())
		throw ModIncompatibility(missingMods, excessiveMods);
}
