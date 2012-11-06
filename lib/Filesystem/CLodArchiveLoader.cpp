#include "StdInc.h"
#include "CLodArchiveLoader.h"
#include "CInputStream.h"
#include "CFileInputStream.h"
#include "CCompressedStream.h"
#include "CBinaryReader.h"
#include "CFileInfo.h"
#include <SDL_endian.h>

ArchiveEntry::ArchiveEntry()
	: offset(0), realSize(0), size(0)
{

}

CLodArchiveLoader::CLodArchiveLoader(const std::string & archive)
{
	// Open archive file(.snd, .vid, .lod)
	this->archive = archive;
	CFileInputStream fileStream(archive);

	// Fake .lod file with no data has to be silently ignored.
	if(fileStream.getSize() < 10)
		return;

	// Retrieve file extension of archive in uppercase
	CFileInfo fileInfo(archive);
	std::string ext = fileInfo.getExtension();
	boost::to_upper(ext);

	// Init the specific lod container format
	if(ext == ".LOD" || ext == ".PAC")
	{
		initLODArchive(fileStream);
	}
	else if(ext == ".VID")
	{
		initVIDArchive(fileStream);
	}
	else if(ext == ".SND")
	{
		initSNDArchive(fileStream);
	}
	else
	{
		throw std::runtime_error("LOD archive format unknown. Cannot deal with " + archive);
	}
}

void CLodArchiveLoader::initLODArchive(CFileInputStream & fileStream)
{
	// Define LodEntryBlock struct
	struct LodEntryBlock
	{
		char filename[16];
		ui32 offset;
		ui32 uncompressedSize;
		ui32 unused;
		ui32 size;
	};

	// Read count of total files
    CBinaryReader reader(&fileStream);

	fileStream.seek(8);
	ui32 totalFiles = reader.readUInt32();

	// Get all entries from file
	fileStream.seek(0x5c);
	struct LodEntryBlock * lodEntries = new struct LodEntryBlock[totalFiles];
	fileStream.read(reinterpret_cast<ui8 *>(lodEntries), sizeof(struct LodEntryBlock) * totalFiles);

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; i++)
	{
		// Create archive entry
		ArchiveEntry entry;
		entry.name = lodEntries[i].filename;
		entry.offset= SDL_SwapLE32(lodEntries[i].offset);
		entry.realSize = SDL_SwapLE32(lodEntries[i].uncompressedSize);
		entry.size = SDL_SwapLE32(lodEntries[i].size);

		// Add lod entry to local entries map
		entries[entry.name] = entry;
	}

	// Delete lod entries array
	delete[] lodEntries;
}

void CLodArchiveLoader::initVIDArchive(CFileInputStream & fileStream)
{
	// Define VideoEntryBlock struct
	struct VideoEntryBlock
	{
		char filename[40];
		ui32 offset;
	};

	// Read count of total files
    CBinaryReader reader(&fileStream);
	fileStream.seek(0);
	ui32 totalFiles = reader.readUInt32();

	// Get all entries from file
	fileStream.seek(4);
	struct VideoEntryBlock * vidEntries = new struct VideoEntryBlock[totalFiles];
	fileStream.read(reinterpret_cast<ui8 *>(vidEntries), sizeof(struct VideoEntryBlock) * totalFiles);

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; i++)
	{
		VideoEntryBlock vidEntry = vidEntries[i];
		ArchiveEntry entry;

		entry.name = vidEntry.filename;
		entry.offset = SDL_SwapLE32(vidEntry.offset);
		entry.size = 0;

		// There is no size, so check where the next file is
		if (i == totalFiles - 1)
		{
			entry.realSize = fileStream.getSize() - entry.offset;
		}
		else
		{
			VideoEntryBlock nextVidEntry = vidEntries[i + 1];
			entry.realSize = SDL_SwapLE32(nextVidEntry.offset) - entry.offset;
		}

		entries[entry.name] = entry;
	}

	// Delete vid entries array
	delete[] vidEntries;
}

void CLodArchiveLoader::initSNDArchive(CFileInputStream & fileStream)
{
	// Define SoundEntryBlock struct
	struct SoundEntryBlock
	{
		char filename[40];
		ui32 offset;
		ui32 size;
	};

	// Read count of total files
    CBinaryReader reader(&fileStream);
	fileStream.seek(0);
	ui32 totalFiles = reader.readUInt32();

	// Get all entries from file
	fileStream.seek(4);
	struct SoundEntryBlock * sndEntries = new struct SoundEntryBlock[totalFiles];
	fileStream.read(reinterpret_cast<ui8 *>(sndEntries), sizeof(struct SoundEntryBlock) * totalFiles);

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; i++)
	{
		SoundEntryBlock sndEntry = sndEntries[i];
		ArchiveEntry entry;

		//for some reason entries in snd have format NAME\0WAVRUBBISH....
		//we need to replace first \0 with dot and take the 3 chars with extension (and drop the rest)
		entry.name = std::string(sndEntry.filename, 40);
		entry.name.resize(entry.name.find_first_of('\0') + 4); //+4 because we take dot and 3-char extension
		entry.name[entry.name.find_first_of('\0')] = '.';

		entry.offset = SDL_SwapLE32(sndEntry.offset);
		entry.realSize = SDL_SwapLE32(sndEntry.size);
		entry.size = 0;
		entries[entry.name] = entry;
	}

	// Delete snd entries array
	delete[] sndEntries;
}

std::unique_ptr<CInputStream> CLodArchiveLoader::load(const std::string & resourceName) const
{
	assert(existsEntry(resourceName));

	const ArchiveEntry & entry = entries.find(resourceName)->second;

	if (entry.size != 0) //compressed data
	{
		std::unique_ptr<CInputStream> fileStream(new CFileInputStream(getOrigin(), entry.offset, entry.size));

		return std::unique_ptr<CInputStream>(new CCompressedStream(std::move(fileStream), false, entry.realSize));
	}
	else
	{
		return std::unique_ptr<CInputStream>(new CFileInputStream(getOrigin(), entry.offset, entry.realSize));
	}
}

boost::unordered_map<ResourceID, std::string> CLodArchiveLoader::getEntries() const
{
	boost::unordered_map<ResourceID, std::string> retList;

	for(auto it = entries.begin(); it != entries.end(); ++it)
	{
		const ArchiveEntry & entry = it->second;
		retList[ResourceID(entry.name)] = entry.name;
	}

	return retList;
}

const ArchiveEntry * CLodArchiveLoader::getArchiveEntry(const std::string & resourceName) const
{
	auto it = entries.find(resourceName);

	if(it != entries.end())
	{
		return &(it->second);
	}

	return nullptr;
}

bool CLodArchiveLoader::existsEntry(const std::string & resourceName) const
{
	return entries.find(resourceName) != entries.end();
}

std::string CLodArchiveLoader::getOrigin() const
{
	return archive;
}
