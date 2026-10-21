/*
 * ScenarioMetadata.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../modding/ModVerificationInfo.h"

class CMapHeader;
class ITranslator;
class JsonNode;
class MetaString;
class TextLocalizationContainer;

/// Information on a map or a saved game that can be read without loading mods used by it.
/// Format is described by config/schemas/scenarioMetadata.json
namespace ScenarioMetadata
{
/// Describes text in a form that can be displayed without texts of the map or campaign,
/// using translators of every language provided by the map or campaign
DLL_LINKAGE JsonNode textToJson(const MetaString & text, const std::map<std::string, TextLocalizationContainer> & translators);

/// Returns text described by textToJson, in preferred language if available, same as CMapHeader::registerMapStrings
DLL_LINKAGE std::string textToString(const JsonNode & text, const ITranslator * translator);

/// Describes map with provided header, as "map" section of metadata
DLL_LINKAGE JsonNode mapToJson(const CMapHeader & header);

/// Describes mods from provided list that affect gameplay, as "mods" section of metadata
DLL_LINKAGE JsonNode modsToJson(const ModCompatibilityInfo & mods);
}
