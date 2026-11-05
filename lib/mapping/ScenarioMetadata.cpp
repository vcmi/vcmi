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

#include "../GameLibrary.h"
#include "../json/JsonNode.h"
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
