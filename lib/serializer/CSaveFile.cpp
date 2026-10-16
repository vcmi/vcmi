/*
 * CSaveFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSaveFile.h"

#include "../texts/TextOperations.h"

#include <zlib.h>

CSaveFile::CSaveFile()
{
	saveData.reserve(128 * 1024);

	for(char symbol : SaveFileFormat::FILE_MAGIC)
		saveData.push_back(static_cast<std::byte>(symbol));
	SaveFileFormat::writeUInt32(saveData, static_cast<uint32_t>(ESerializationVersion::CURRENT));
}

void CSaveFile::addChunk(const SaveFileFormat::ChunkID & id, const std::vector<std::byte> & data)
{
	const std::string chunkName = SaveFileFormat::toString(id);

	if(chunksWritten >= SaveFileFormat::CHUNK_ORDER.size() || SaveFileFormat::CHUNK_ORDER[chunksWritten] != id)
		throw std::logic_error("Save chunk " + chunkName + " is written out of order!");

	if(data.empty() || data.size() > SaveFileFormat::MAX_CHUNK_SIZE)
		throw std::runtime_error("Save chunk " + chunkName + " has invalid size " + std::to_string(data.size()) + "!");

	uLongf compressedSize = compressBound(data.size());
	std::vector<std::byte> compressed(compressedSize);

	int result = compress2(reinterpret_cast<Bytef *>(compressed.data()), &compressedSize, reinterpret_cast<const Bytef *>(data.data()), data.size(), Z_DEFAULT_COMPRESSION);
	if(result != Z_OK)
		throw std::runtime_error("Failed to compress save chunk " + chunkName + ", zlib error " + std::to_string(result));

	if(compressedSize > std::numeric_limits<uint32_t>::max())
		throw std::runtime_error("Save chunk " + chunkName + " is too large after compression!");

	for(char symbol : id)
		saveData.push_back(static_cast<std::byte>(symbol));
	SaveFileFormat::writeUInt32(saveData, compressedSize);
	SaveFileFormat::writeUInt32(saveData, data.size());
	saveData.insert(saveData.end(), compressed.begin(), compressed.begin() + compressedSize);
	++chunksWritten;

	logGlobal->info("Save chunk %s compressed: %d -> %d bytes", chunkName, data.size(), compressedSize);
}

void CSaveFile::write(const boost::filesystem::path & fileName)
{
	if(chunksWritten != SaveFileFormat::CHUNK_ORDER.size())
		throw std::logic_error("Attempt to write incomplete saved game!");

	std::ofstream sfile(fileName.c_str(), std::ios::out | std::ios::binary);

	sfile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

	if(!sfile)
		throw std::runtime_error("Error: cannot open file '" + TextOperations::filesystemPathToUtf8(fileName) + "' for writing!");

	sfile.write(reinterpret_cast<const char *>(saveData.data()), saveData.size());
	sfile.close(); // reports buffered data that could not be written, e.g. if disk is full
	logGlobal->info("Saved game written: %d bytes", saveData.size());
}
