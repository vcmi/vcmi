/*
 * SaveMetadata.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../modding/ModVerificationInfo.h"

class CGameState;
class CMapHeader;
class JsonNode;
struct JsonValidator;
struct StartInfo;

/// Information on a saved game that can be read without loading mods used by the save.
/// Stored in saved game as json, format is described by config/schemas/scenarioMetadata.json, which is shared with metadata of maps
namespace SaveMetadata
{
/// Creates metadata for a save of provided game
DLL_LINKAGE JsonNode create(const CGameState & gameState);

/// Creates metadata for a save created before ESerializationVersion::SAVE_CHUNKS, from its map header, start info and mods.
/// Such metadata has no fields that are not stored in legacy saves: VCMI version, save time and day
DLL_LINKAGE JsonNode createLegacy(const CMapHeader & mapHeader, const StartInfo & startInfo, const ModCompatibilityInfo & mods);

/// Parses metadata of a saved game; throws std::runtime_error if data does not match the schema or lacks fields required for saved games
DLL_LINKAGE JsonNode parse(const std::vector<std::byte> & data, const std::string & fileName, JsonValidator & validator);

/// Throws ModIncompatibility if mods listed in metadata do not match active mods
DLL_LINKAGE void verifyMods(const JsonNode & metadata);
}
