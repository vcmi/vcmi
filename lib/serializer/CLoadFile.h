/*
 * CLoadFile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "SaveFileFormat.h"

class CMemorySerializer;
class IGameInfoCallback;

/// Reads chunks of a saved game
class DLL_LINKAGE CLoadFile final
{
	struct ChunkLocation
	{
		uint64_t dataOffset = 0;
		uint32_t compressedSize = 0;
		uint32_t uncompressedSize = 0;
	};

	boost::filesystem::path fileName;
	std::ifstream file;
	ESerializationVersion version;
	std::array<ChunkLocation, SaveFileFormat::CHUNK_ORDER.size()> chunks;

	static ESerializationVersion readVersion(const boost::filesystem::path & fname, bool allowNewer);

	/// Opens saved game for reading of metadata only, which is also possible for saves of newer versions
	CLoadFile(const boost::filesystem::path & fname, bool metadataOnly);

	void readBytes(std::byte * target, size_t size);
	[[noreturn]] void throwInvalidFile(const std::string & reason) const;

public:
	/// Returns serialization version of a saved game.
	/// Throws SaveTooOldException or SaveTooNewException if version is not supported, std::runtime_error if file is not a saved game
	static ESerializationVersion readVersion(const boost::filesystem::path & fname);

	/// Returns uncompressed metadata chunk of a saved game, including saves of newer versions
	static std::vector<std::byte> readMetadata(const boost::filesystem::path & fname);

	/// Opens saved game and validates location of all chunks; saves older than SAVE_CHUNKS are loaded by CLegacyLoadFile
	explicit CLoadFile(const boost::filesystem::path & fname);

	/// Returns uncompressed content of a chunk
	std::vector<std::byte> readChunk(const SaveFileFormat::ChunkID & id);

	/// Returns deserializer for content of a chunk
	std::unique_ptr<CMemorySerializer> openChunk(const SaveFileFormat::ChunkID & id, IGameInfoCallback * cb);
};
