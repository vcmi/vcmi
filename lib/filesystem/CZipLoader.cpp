#include "StdInc.h"
#include "../../Global.h"
#include "CZipLoader.h"

#include "../ScopeGuard.h"

/*
 * CZipLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CZipStream::CZipStream(std::shared_ptr<CIOApi> api, const std::string & archive, unz_file_pos filepos)
{
	zlib_filefunc64_def zlibApi;
	
	zlibApi = api->getApiStructure();
	
	file = unzOpen2_64(archive.c_str(), &zlibApi);
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

ui32 CZipStream::calculateCRC32()
{
	unz_file_info info;
	unzGetCurrentFileInfo (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	return info.crc;
}

CZipLoader::CZipLoader(const std::string & mountPoint, const std::string & archive, std::shared_ptr<CIOApi> api):
	ioApi(api),
    zlibApi(ioApi->getApiStructure()),	
    archiveName(archive),
    mountPoint(mountPoint),
    files(listFiles(mountPoint, archive))
{
	logGlobal->traceStream() << "Zip archive loaded, " << files.size() << " files found";
}

std::unordered_map<ResourceID, unz_file_pos> CZipLoader::listFiles(const std::string & mountPoint, const std::string & archive)
{
	std::unordered_map<ResourceID, unz_file_pos> ret;

	unzFile file = unzOpen2_64(archive.c_str(), &zlibApi);

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
	return std::unique_ptr<CInputStream>(new CZipStream(ioApi, archiveName, files.at(resourceName)));	
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

/// extracts currently selected file from zip into stream "where"
static bool extractCurrent(unzFile file, std::ostream & where)
{
	std::array<char, 8 * 1024> buffer;

	unzOpenCurrentFile(file);

	while (1)
	{
		int readSize = unzReadCurrentFile(file, buffer.data(), buffer.size());

		if (readSize < 0) // error
			break;

		if (readSize == 0) // end-of-file. Also performs CRC check
			return unzCloseCurrentFile(file) == UNZ_OK;

		if (readSize > 0) // successful read
		{
			where.write(buffer.data(), readSize);
			if (!where.good())
				break;
		}
	}

	// extraction failed. Close file and exit
	unzCloseCurrentFile(file);
	return false;
}

std::vector<std::string> ZipArchive::listFiles(std::string filename)
{
	std::vector<std::string> ret;

	unzFile file = unzOpen(filename.c_str());

	if (unzGoToFirstFile(file) == UNZ_OK)
	{
		do
		{
			unz_file_info info;
			std::vector<char> filename;

			unzGetCurrentFileInfo (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			filename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo (file, &info, filename.data(), filename.size(), nullptr, 0, nullptr, 0);

			ret.push_back(std::string(filename.data(), filename.size()));
		}
		while (unzGoToNextFile(file) == UNZ_OK);
	}
	unzClose(file);

	return ret;
}

bool ZipArchive::extract(std::string from, std::string where)
{
	// Note: may not be fast enough for large archives (should NOT happen with mods)
	// because locating each file by name may be slow. Unlikely slower than decompression though
	return extract(from, where, listFiles(from));
}

bool ZipArchive::extract(std::string from, std::string where, std::vector<std::string> what)
{
	unzFile archive = unzOpen(from.c_str());

	auto onExit = vstd::makeScopeGuard([&]()
	{
		unzClose(archive);
	});

	for (std::string & file : what)
	{
		if (unzLocateFile(archive, file.c_str(), 1) != UNZ_OK)
			return false;

		std::string fullName = where + '/' + file;
		std::string fullPath = fullName.substr(0, fullName.find_last_of("/"));

		boost::filesystem::create_directories(fullPath);
		// directory. No file to extract
		// TODO: better way to detect directory? Probably check return value of unzOpenCurrentFile?
		if (boost::algorithm::ends_with(file, "/"))
			continue;

		std::ofstream destFile(fullName, std::ofstream::binary);
		if (!destFile.good())
			return false;

		if (!extractCurrent(archive, destFile))
			return false;
	}
	return true;
}
