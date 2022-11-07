/*
 * CArchiveLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArchiveLoader.h"

#include "CFileInputStream.h"
#include "CCompressedStream.h"

#include "CBinaryReader.h"

VCMI_LIB_NAMESPACE_BEGIN

ArchiveEntry::ArchiveEntry()
	: offset(0), fullSize(0), compressedSize(0)
{

}

CArchiveLoader::CArchiveLoader(std::string _mountPoint, boost::filesystem::path _archive) :
    archive(std::move(_archive)),
    mountPoint(std::move(_mountPoint))
{
	// Open archive file(.snd, .vid, .lod)
	CFileInputStream fileStream(archive);

	// Fake .lod file with no data has to be silently ignored.
	if(fileStream.getSize() < 10)
		return;

	// Retrieve file extension of archive in uppercase
	const std::string ext = boost::to_upper_copy(archive.extension().string());

	// Init the specific lod container format
	if(ext == ".LOD" || ext == ".PAC")
		initLODArchive(mountPoint, fileStream);
	else if(ext == ".VID")
		initVIDArchive(mountPoint, fileStream);
	else if(ext == ".SND")
		initSNDArchive(mountPoint, fileStream);
	else
		throw std::runtime_error("LOD archive format unknown. Cannot deal with " + archive.string());

	logGlobal->trace("%sArchive \"%s\" loaded (%d files found).", ext, archive.filename(), entries.size());
}

void CArchiveLoader::initLODArchive(const std::string &mountPoint, CFileInputStream & fileStream)
{
	// Read count of total files
	CBinaryReader reader(&fileStream);

	fileStream.seek(8);
	ui32 totalFiles = reader.readUInt32();

	// Get all entries from file
	fileStream.seek(0x5c);

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; ++i)
	{
		char filename[16];
		reader.read(reinterpret_cast<ui8*>(filename), 16);

		// Create archive entry
		ArchiveEntry entry;
		entry.name     = filename;
		entry.offset   = reader.readUInt32();
		entry.fullSize = reader.readUInt32();
		fileStream.skip(4); // unused, unknown
		entry.compressedSize     = reader.readUInt32();

		// Add lod entry to local entries map
		entries[ResourceID(mountPoint + entry.name)] = entry;
	}
}

void CArchiveLoader::initVIDArchive(const std::string &mountPoint, CFileInputStream & fileStream)
{

	// Read count of total files
	CBinaryReader reader(&fileStream);
	fileStream.seek(0);
	ui32 totalFiles = reader.readUInt32();

	std::set<int> offsets;

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; ++i)
	{
		char filename[40];
		reader.read(reinterpret_cast<ui8*>(filename), 40);

		ArchiveEntry entry;
		entry.name = filename;
		entry.offset = reader.readInt32();
		entry.compressedSize = 0;

		offsets.insert(entry.offset);
		entries[ResourceID(mountPoint + entry.name)] = entry;
	}
	offsets.insert((int)fileStream.getSize());

	// now when we know position of all files their sizes can be set correctly
	for (auto & entry : entries)
	{
		auto it = offsets.find(entry.second.offset);
		it++;
		entry.second.fullSize = *it - entry.second.offset;
	}
}

void CArchiveLoader::initSNDArchive(const std::string &mountPoint, CFileInputStream & fileStream)
{
	// Read count of total files
	CBinaryReader reader(&fileStream);
	fileStream.seek(0);
	ui32 totalFiles = reader.readUInt32();

	// Insert entries to list
	for(ui32 i = 0; i < totalFiles; ++i)
	{
		char filename[40];
		reader.read(reinterpret_cast<ui8*>(filename), 40);

		//for some reason entries in snd have format NAME\0WAVRUBBISH....
		//we need to replace first \0 with dot and take the 3 chars with extension (and drop the rest)
		ArchiveEntry entry;
		entry.name  = filename; // till 1st \0
		entry.name += '.';
		entry.name += std::string(filename + entry.name.size(), 3);

		entry.offset = reader.readInt32();
		entry.fullSize = reader.readInt32();
		entry.compressedSize = 0;
		entries[ResourceID(mountPoint + entry.name)] = entry;
	}
}

std::unique_ptr<CInputStream> CArchiveLoader::load(const ResourceID & resourceName) const
{
	assert(existsResource(resourceName));

	const ArchiveEntry & entry = entries.at(resourceName);

	if (entry.compressedSize != 0) //compressed data
	{
		auto fileStream = make_unique<CFileInputStream>(archive, entry.offset, entry.compressedSize);

		return make_unique<CCompressedStream>(std::move(fileStream), false, entry.fullSize);
	}
	else
	{
		return make_unique<CFileInputStream>(archive, entry.offset, entry.fullSize);
	}
}

bool CArchiveLoader::existsResource(const ResourceID & resourceName) const
{
	return entries.count(resourceName) != 0;
}

std::string CArchiveLoader::getMountPoint() const
{
	return mountPoint;
}

std::unordered_set<ResourceID> CArchiveLoader::getFilteredFiles(std::function<bool(const ResourceID &)> filter) const
{
	std::unordered_set<ResourceID> foundID;

	for (auto & file : entries)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}

VCMI_LIB_NAMESPACE_END
