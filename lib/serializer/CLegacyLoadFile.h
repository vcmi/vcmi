/*
 * CLegacyLoadFile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinaryDeserializer.h"

class CMapHeader;
struct StartInfo;

/// Reads saved games created before ESerializationVersion::SAVE_CHUNKS.
/// Such saves contain map header, start info, list of active mods and game state, uncompressed, in a single serializer stream
class DLL_LINKAGE CLegacyLoadFile final : public IBinaryReader
{
	BinaryDeserializer serializer;
	std::fstream sfile;

	int read(std::byte * data, unsigned size) override; //throws!

public:
	CLegacyLoadFile(const boost::filesystem::path & fname, IGameInfoCallback * cb); //throws!

	/// Loads map header and start info located at the start of the file
	void loadHeader(CMapHeader & mapHeader, StartInfo & startInfo);

	/// Skips data located before game state and returns deserializer of game state.
	/// Throws ModIncompatibility if mods of the save do not match active mods
	BinaryDeserializer & skipToGameState();
};
