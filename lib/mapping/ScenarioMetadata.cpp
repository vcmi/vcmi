/*
 * ScenarioMetadata.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScenarioMetadata.h"

#include "CMapHeader.h"
#include "MapFormat.h"
#include "MapFormatSettings.h"

#include "../GameLibrary.h"
#include "../campaign/CampaignState.h"
#include "../json/JsonNode.h"
#include "../modding/CModHandler.h"
#include "../modding/ModDescription.h"
#include "../modding/ModScope.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"
#include "../texts/MetaString.h"
#include "../texts/TextLocalizationContainer.h"

JsonNode ScenarioMetadata::textToJson(const MetaString & text, const std::map<std::string, TextLocalizationContainer> & translators)
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

std::string ScenarioMetadata::textToString(const JsonNode & text, const ITranslator * translator)
{
	const auto & translations = text["translations"].Struct();
	const auto & textIDs = text["text"]["stringsTextID"].Vector();

	// translation mods override texts of the map in the static text store, same as in CompositeTranslator
	const bool overriddenByMod = !textIDs.empty() && std::ranges::all_of(textIDs, [](const JsonNode & textID)
	{
		return LIBRARY->generaltexth->identifierExists(TextIdentifier(textID.String()));
	});

	// text without translations does not use texts of the map, for example, map name of a random map
	if(translations.empty() || overriddenByMod)
	{
		MetaString result;
		result.jsonDeserialize(text["text"]);
		return result.toString(translator);
	}

	const std::string preferred = Languages::getPreferredLanguage();
	if(translations.contains(preferred))
		return translations.at(preferred).String();

	const std::string english = Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).identifier;
	if(translations.contains(english))
		return translations.at(english).String();

	return translations.begin()->second.String();
}

JsonNode ScenarioMetadata::mapToJson(const CMapHeader & header)
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
	result["author"] = textToJson(header.author, translators);
	result["authorContact"] = textToJson(header.authorContact, translators);
	result["mapVersion"] = textToJson(header.mapVersion, translators);
	result["creationDateTime"].Integer() = header.creationDateTime;
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

JsonNode ScenarioMetadata::campaignToJson(const Campaign & campaign)
{
	const std::map<std::string, TextLocalizationContainer> translators = {
		{ LIBRARY->modh->getModLanguage(campaign.getModName()), *campaign.getTexts() }
	};

	JsonNode result;
	result["name"] = textToJson(campaign.getName(), translators);
	result["author"] = textToJson(campaign.getAuthor(), translators);
	result["authorContact"] = textToJson(campaign.getAuthorContact(), translators);
	result["campaignVersion"] = textToJson(campaign.getCampaignVersion(), translators);
	result["creationDateTime"].Integer() = campaign.getCreationDateTime();
	result["scenarios"].Integer() = campaign.scenariosCount();

	const CampaignVersion version = campaign.getFormat();
	result["format"].Integer() = static_cast<int>(version == CampaignVersion::VCMI ? EMapFormat::VCMI : LIBRARY->mapFormat->getMapFormat(version));
	return result;
}

JsonNode ScenarioMetadata::modsToJson(const ModCompatibilityInfo & mods)
{
	JsonNode result;
	result.Struct();

	for(const auto & [modID, mod] : mods)
	{
		// mod lists of legacy saves contain built-in mod
		if(!mod.impactsGameplay || modID == ModScope::scopeBuiltin())
			continue;

		const std::string rootID = modID.substr(0, modID.find('.'));
		JsonNode & entry = result[rootID];
		if(entry.isNull())
		{
			// list of a map may contain a submod without its root mod
			const auto rootMod = mods.find(rootID);
			entry["name"].String() = rootMod != mods.end() ? rootMod->second.name : rootID;
			entry["version"].String() = rootMod != mods.end() ? rootMod->second.version.toString() : "";
			entry["submods"].Vector();
		}

		if(modID != rootID)
			entry["submods"].Vector().emplace_back(modID.substr(rootID.size() + 1));
	}
	return result;
}

JsonNode ScenarioMetadata::activeModsToJson()
{
	ModCompatibilityInfo mods;
	for(const auto & modID : LIBRARY->modh->getActiveMods())
		mods[modID] = LIBRARY->modh->getModInfo(modID).getVerificationInfo();
	return modsToJson(mods);
}

ModListVerificationStatus ScenarioMetadata::verifyMods(const JsonNode & mods, bool isSave)
{
	const auto & installedMods = LIBRARY->modh->getAllMods();
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

	ModListVerificationStatus result;
	for(const auto & [rootID, entry] : mods.Struct())
	{
		if(!vstd::contains(installedMods, rootID))
		{
			result[rootID] = ModVerificationStatus::NOT_INSTALLED;
			continue;
		}

		if(!affectsGameplay(rootID))
			continue;

		if(!vstd::contains(activeMods, rootID))
		{
			result[rootID] = ModVerificationStatus::DISABLED;
			continue;
		}

		// list of submods and versions often change between versions of a mod without breaking saves
		if(isSave)
			continue;

		const auto requiredVersion = CModVersion::fromString(entry["version"].String());
		if(!requiredVersion.isNull() && !LIBRARY->modh->getModInfo(rootID).getVersion().compatible(requiredVersion))
		{
			result[rootID] = ModVerificationStatus::VERSION_MISMATCH;
			continue;
		}

		for(const auto & submod : entry["submods"].Vector())
		{
			const TModID modID = rootID + '.' + submod.String();
			if(!vstd::contains(installedMods, modID))
				result[modID] = ModVerificationStatus::NOT_INSTALLED;
			else if(!vstd::contains(activeMods, modID))
				result[modID] = ModVerificationStatus::DISABLED;
		}
	}

	if(isSave)
	{
		const JsonNode localMods = activeModsToJson();
		for(const auto & [rootID, localMod] : localMods.Struct())
			if(!mods.Struct().contains(rootID))
				result[rootID] = ModVerificationStatus::EXCESSIVE;
	}

	return result;
}
