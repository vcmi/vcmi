/*
 * CSaveFile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "SaveFileFormat.h"

/// Builds a saved game in memory and writes it to a file
class DLL_LINKAGE CSaveFile final
{
	std::vector<std::byte> saveData;
	size_t chunksWritten = 0;

public:
	CSaveFile();

	/// Compresses data and appends it as the next chunk. Chunks must be added in SaveFileFormat::CHUNK_ORDER
	void addChunk(const SaveFileFormat::ChunkID & id, const std::vector<std::byte> & data);

	void write(const boost::filesystem::path & fname);
};
