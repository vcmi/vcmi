#include "Global.h"
#include "CZipLoader.h"

/*
 * CZipLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CZipStream::CZipStream(const std::string & archive, unz_file_pos filepos)
{
	file = unzOpen(archive.c_str());
	unzGoToFilePos(file, &filepos);
	unzOpenCurrentFile(file);
}

CZipStream::~CZipStream()
{
	unzCloseCurrentFile(file);
	unzClose(file);
}

si64 CZipStream::readMore(ui8 * data, si64 size)
{
	return unzReadCurrentFile(file, data, size);
}

si64 CZipStream::getSize()
{
	unz_file_info info;
	unzGetCurrentFileInfo (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	return info.uncompressed_size;
}

CZipLoader::CZipLoader(const std::string & mountPoint, const std::string & archive):
    archiveName(archive),
    mountPoint(mountPoint),
    files(listFiles(mountPoint, archive))
{
	logGlobal->traceStream() << "Zip archive loaded, " << files.size() << " files found";
}

std::unordered_map<ResourceID, unz_file_pos> CZipLoader::listFiles(const std::string & mountPoint, const std::string & archive)
{
	std::unordered_map<ResourceID, unz_file_pos> ret;

	unzFile file = unzOpen(archive.c_str());

	if (unzGoToFirstFile(file) == UNZ_OK)
	{
		do
		{
			unz_file_info info;
			std::vector<char> filename;
			// Fill unz_file_info structure with current file info
			unzGetCurrentFileInfo (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			filename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo (file, &info, filename.data(), filename.size(), nullptr, 0, nullptr, 0);

			std::string filenameString(filename.data(), filename.size());
			unzGetFilePos(file, &ret[ResourceID(mountPoint + filenameString)]);
		}
		while (unzGoToNextFile(file) == UNZ_OK);
	}
	unzClose(file);

	return ret;
}

std::unique_ptr<CInputStream> CZipLoader::load(const ResourceID & resourceName) const
{
	return std::unique_ptr<CInputStream>(new CZipStream(archiveName, files.at(resourceName)));
}

bool CZipLoader::existsResource(const ResourceID & resourceName) const
{
	return files.count(resourceName) != 0;
}

std::string CZipLoader::getMountPoint() const
{
	return mountPoint;
}

std::unordered_set<ResourceID> CZipLoader::getFilteredFiles(std::function<bool(const ResourceID &)> filter) const
{
	std::unordered_set<ResourceID> foundID;

	for (auto & file : files)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}