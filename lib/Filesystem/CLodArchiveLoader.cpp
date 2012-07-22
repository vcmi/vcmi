#include "StdInc.h"
#include "CLodArchiveLoader.h"
#include "CInputStream.h"
#include "CFileInputStream.h"
#include "CLodStream.h"
#include "CBinaryReader.h"
#include "CFileInfo.h"
#include <SDL_endian.h>

ArchiveEntry::ArchiveEntry()
	: offset(0), realSize(0), size(0)
{

}

CLodArchiveLoader::CLodArchiveLoader()
{

}

CLodArchiveLoader::CLodArchiveLoader(const std::string & archive)
{
	open(archive);
}

CLodArchiveLoader::CLodArchiveLoader(const CFileInfo & archive)
{
	open(archive);
}

void CLodArchiveLoader::open(const std::string & archive)
{
	// Open archive file(.snd, .vid, .lod)
	this->archive = archive;
	CFileInputStream fileStream(archive);

	// Retrieve file extension of archive in uppercase
	CFileInfo fileInfo(archive);
	std::string ext = fileInfo.getExtension();
	boost::to_upper(ext);

	// Init the specific lod container format
	if(ext == ".LOD")
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

void CLodArchiveLoader::open(const CFileInfo & archive)
{
	open(archive.getName());
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
	CBinaryReader reader(fileStream);
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
	CBinaryReader reader(fileStream);
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

		// There is no size, so check where the next file is
		if (i == totalFiles - 1)
		{
			entry.size = fileStream.getSize() - entry.offset;
		}
		else
		{
			VideoEntryBlock nextVidEntry = vidEntries[i + 1];
			entry.size = SDL_SwapLE32(nextVidEntry.offset) - entry.offset;
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
	CBinaryReader reader(fileStream);
	fileStream.seek(0);
	ui32 totalFiles = reader.readUInt32();

	// Get all entries from file
	fileStream.seek(4);
	struct SoundEntryBlock * sndEntries = new struct SoundEntryBlock[totalFiles];
	fileStream.read(reinterpret_cast<ui8 *>(sndEntries), sizeof(struct SoundEntryBlock) * totalFiles);

	// Insert entries to list
	for(ui8 i = 0; i < totalFiles; i++)
	{
		SoundEntryBlock sndEntry = sndEntries[i];
		ArchiveEntry entry;

		entry.name = sndEntry.filename;
		entry.offset = SDL_SwapLE32(sndEntry.offset);
		entry.size = SDL_SwapLE32(sndEntry.size);

		entries[entry.name] = entry;
	}

	// Delete snd entries array
	delete[] sndEntries;
}

std::unique_ptr<CInputStream> CLodArchiveLoader::load(const std::string & resourceName) const
{
	std::unique_ptr<CInputStream> stream(new CLodStream(this, resourceName));
	return stream;
}

std::list<std::string> CLodArchiveLoader::getEntries() const
{
	std::list<std::string> retList;

	for(auto it = entries.begin(); it != entries.end(); ++it)
	{
		const ArchiveEntry & entry = it->second;
		retList.push_back(entry.name);
	}

	return std::move(retList);
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
	auto it = entries.find(resourceName);
	if(it != entries.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string CLodArchiveLoader::getOrigin() const
{
	return archive;
}
