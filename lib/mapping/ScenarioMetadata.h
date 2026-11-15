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

class Campaign;
class CMapHeader;
class ITranslator;
class JsonNode;
class MetaString;
class ModSnapshot;
class TextLocalizationContainer;

/// Information on a map or a saved game that can be read without loading mods used by it.
/// Format is described by config/schemas/scenarioMetadata.json
namespace ScenarioMetadata
{
/// Describes text in a form that can be displayed without texts of the map or campaign,
/// using translators of every language provided by the map or campaign
DLL_LINKAGE JsonNode textToJson(const MetaString & text, const std::map<std::string, TextLocalizationContainer> & translators);

/// Returns text described by textToJson: text of a translation mod if present, otherwise text of the map in preferred language if available
DLL_LINKAGE std::string textToString(const JsonNode & text, const ITranslator * translator);

/// Describes map with provided header, as "map" section of metadata
DLL_LINKAGE JsonNode mapToJson(const CMapHeader & header);

/// Describes campaign with provided header, as "campaign" section of metadata
DLL_LINKAGE JsonNode campaignToJson(const Campaign & campaign);

/// Describes mods from provided list that affect gameplay, as "mods" section of metadata
DLL_LINKAGE JsonNode modsToJson(const ModCompatibilityInfo & mods);

/// Describes active mods of provided mod list that affect gameplay, as "mods" section of metadata
DLL_LINKAGE JsonNode activeModsToJson(const ModSnapshot & modList);

/// Returns mods from "mods" section of metadata that do not match active mods of provided mod list, by mod ID.
/// exact: list is complete, active gameplay mods and submods missing from it are reported (saves); otherwise list is a minimum (maps).
/// Listed submods that are not installed are not reported. VERSION_MISMATCH does not prevent loading, other statuses do
DLL_LINKAGE ModListVerificationStatus verifyMods(const JsonNode & mods, bool exact, const ModSnapshot & modList);
}
