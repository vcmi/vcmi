/*
 * SaveFileFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ESerializationVersion.h"

/// Thrown if saved game was created by a version of VCMI that is no longer supported
class DLL_LINKAGE SaveTooOldException : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
	~SaveTooOldException() override;
};

/// Layout of a saved game: file magic, serialization version, then chunks listed in CHUNK_ORDER.
/// Every chunk consists of its ID, compressed size, uncompressed size and zlib-compressed data.
/// All integers are stored as little-endian
namespace SaveFileFormat
{
using ChunkID = std::array<char, 4>;

constexpr ChunkID FILE_MAGIC = {'V', 'C', 'M', 'I'};

/// Json with information on the save that can be read without loading mods used by the save
constexpr ChunkID CHUNK_METADATA = {'M', 'E', 'T', 'A'};
/// Map header and start info, required to select the save in lobby
constexpr ChunkID CHUNK_HEADER = {'H', 'E', 'A', 'D'};
/// State of the game and of the server
constexpr ChunkID CHUNK_GAMESTATE = {'G', 'A', 'M', 'E'};

inline constexpr std::array CHUNK_ORDER = {CHUNK_METADATA, CHUNK_HEADER, CHUNK_GAMESTATE};

constexpr size_t FILE_HEADER_SIZE = 8;
constexpr size_t CHUNK_HEADER_SIZE = 12;

/// Limit for size of uncompressed chunk, to reject corrupted size before allocating memory for it
constexpr uint32_t MAX_CHUNK_SIZE = 1024 * 1024 * 1024;

DLL_LINKAGE std::string toString(const ChunkID & id);

DLL_LINKAGE void writeUInt32(std::vector<std::byte> & target, uint32_t value);
DLL_LINKAGE uint32_t readUInt32(const std::byte * source);
}
