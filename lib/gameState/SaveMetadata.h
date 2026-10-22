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

class CGameState;
class JsonNode;

/// Information on a saved game that can be read without loading mods used by the save.
/// Stored in saved game as json, format is described by config/schemas/saveMetadata.json
namespace SaveMetadata
{
/// Creates metadata for a save of provided game
DLL_LINKAGE JsonNode create(const CGameState & gameState);

/// Parses metadata; throws std::runtime_error if data does not match the schema
DLL_LINKAGE JsonNode parse(const std::vector<std::byte> & data, const std::string & fileName);

/// Throws ModIncompatibility if mods listed in metadata do not match active mods
DLL_LINKAGE void verifyMods(const JsonNode & metadata);
}
