/*
 * CLoadFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CLoadFile.h"

#include "CMemorySerializer.h"

#include "../texts/TextOperations.h"

#include <zlib.h>

ESerializationVersion CLoadFile::readVersion(const boost::filesystem::path & fname)
{
	return readVersion(fname, false);
}

ESerializationVersion CLoadFile::readVersion(const boost::filesystem::path & fname, bool allowNewer)
{
	const std::string fileNameUtf8 = TextOperations::filesystemPathToUtf8(fname);

	std::ifstream file(fname.c_str(), std::ios::in | std::ios::binary);
	if(!file)
		throw std::runtime_error("Error: cannot open file '" + fileNameUtf8 + "' for reading!");

	std::array<std::byte, SaveFileFormat::FILE_HEADER_SIZE> header;
	file.read(reinterpret_cast<char *>(header.data()), header.size());
	if(!file)
		throw std::runtime_error("Error: '" + fileNameUtf8 + "' is not a VCMI file!");

	for(size_t i = 0; i < SaveFileFormat::FILE_MAGIC.size(); ++i)
		if(std::to_integer<char>(header[i]) != SaveFileFormat::FILE_MAGIC[i])
			throw std::runtime_error("Error: '" + fileNameUtf8 + "' is not a VCMI file!");

	const auto version = static_cast<ESerializationVersion>(SaveFileFormat::readUInt32(header.data() + SaveFileFormat::FILE_MAGIC.size()));

	if(version < ESerializationVersion::MINIMAL)
		throw SaveTooOldException("Error: too old file format detected in '" + fileNameUtf8 + "'!");

	if(version > ESerializationVersion::CURRENT && !allowNewer)
		throw SaveTooNewException("Error: too new file format detected in '" + fileNameUtf8 + "'!");

	return version;
}

std::vector<std::byte> CLoadFile::readMetadata(const boost::filesystem::path & fname)
{
	CLoadFile loader(fname, true);
	return loader.readChunk(SaveFileFormat::CHUNK_METADATA);
}

CLoadFile::CLoadFile(const boost::filesystem::path & fname)
	: CLoadFile(fname, false)
{
}

CLoadFile::CLoadFile(const boost::filesystem::path & fname, bool metadataOnly)
	: fileName(fname)
	, file(fname.c_str(), std::ios::in | std::ios::binary)
	, version(readVersion(fname, metadataOnly))
{
	if(version < ESerializationVersion::SAVE_CHUNKS)
		throwInvalidFile("file uses legacy save format");

	if(!file)
		throwInvalidFile("cannot open file for reading");

	const uint64_t fileSize = boost::filesystem::file_size(fname);
	uint64_t position = SaveFileFormat::FILE_HEADER_SIZE;

	// only metadata chunk is guaranteed to have the same location in saves of newer versions
	const size_t chunksToLocate = metadataOnly ? 1 : chunks.size();

	for(size_t i = 0; i < chunksToLocate; ++i)
	{
		const std::string chunkName = SaveFileFormat::toString(SaveFileFormat::CHUNK_ORDER[i]);

		if(fileSize - position < SaveFileFormat::CHUNK_HEADER_SIZE)
			throwInvalidFile("file is truncated before chunk " + chunkName);

		std::array<std::byte, SaveFileFormat::CHUNK_HEADER_SIZE> header;
		file.seekg(position);
		readBytes(header.data(), header.size());
		position += header.size();

		for(size_t j = 0; j < SaveFileFormat::CHUNK_ORDER[i].size(); ++j)
			if(std::to_integer<char>(header[j]) != SaveFileFormat::CHUNK_ORDER[i][j])
				throwInvalidFile("chunk " + chunkName + " not found");

		chunks[i].dataOffset = position;
		chunks[i].compressedSize = SaveFileFormat::readUInt32(header.data() + 4);
		chunks[i].uncompressedSize = SaveFileFormat::readUInt32(header.data() + 8);

		if(chunks[i].compressedSize > fileSize - position)
			throwInvalidFile("file is truncated inside of chunk " + chunkName);

		if(chunks[i].uncompressedSize == 0 || chunks[i].uncompressedSize > SaveFileFormat::MAX_CHUNK_SIZE)
			throwInvalidFile("chunk " + chunkName + " has invalid size " + std::to_string(chunks[i].uncompressedSize));

		position += chunks[i].compressedSize;
	}

	if(!metadataOnly && position != fileSize)
		throwInvalidFile("unexpected data after the last chunk");
}

void CLoadFile::throwInvalidFile(const std::string & reason) const
{
	throw std::runtime_error("Error: failed to load '" + TextOperations::filesystemPathToUtf8(fileName) + "': " + reason + "!");
}

void CLoadFile::readBytes(std::byte * target, size_t size)
{
	file.read(reinterpret_cast<char *>(target), size);
	if(!file)
		throwInvalidFile("failed to read data from file");
}

std::vector<std::byte> CLoadFile::readChunk(const SaveFileFormat::ChunkID & id)
{
	const std::string chunkName = SaveFileFormat::toString(id);
	const auto chunkIt = std::ranges::find(SaveFileFormat::CHUNK_ORDER, id);
	if(chunkIt == SaveFileFormat::CHUNK_ORDER.end())
		throw std::logic_error("Save chunk " + chunkName + " does not exist!");

	const ChunkLocation & chunk = chunks[chunkIt - SaveFileFormat::CHUNK_ORDER.begin()];

	std::vector<std::byte> compressed(chunk.compressedSize);
	file.seekg(chunk.dataOffset);
	readBytes(compressed.data(), compressed.size());

	std::vector<std::byte> result(chunk.uncompressedSize);
	uLongf resultSize = result.size();

	// not uncompress2 - it is missing in zlib shipped with older Android NDK
	int status = uncompress(reinterpret_cast<Bytef *>(result.data()), &resultSize, reinterpret_cast<const Bytef *>(compressed.data()), compressed.size());
	if(status != Z_OK)
		throwInvalidFile("chunk " + chunkName + " is corrupted, zlib error " + std::to_string(status));

	if(resultSize != result.size())
		throwInvalidFile("chunk " + chunkName + " has unexpected size");

	return result;
}

std::unique_ptr<CMemorySerializer> CLoadFile::openChunk(const SaveFileFormat::ChunkID & id, IGameInfoCallback * cb)
{
	auto result = std::make_unique<CMemorySerializer>(readChunk(id));
	result->iser.version = version;
	result->iser.cb = cb;
	result->iser.loadingGamestate = true;
	return result;
}
