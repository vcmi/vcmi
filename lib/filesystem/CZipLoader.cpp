/*
 * CZipLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CZipLoader.h"
#include "FileStream.h"

#include "../ScopeGuard.h"

VCMI_LIB_NAMESPACE_BEGIN

CZipStream::CZipStream(std::shared_ptr<CIOApi> api, const boost::filesystem::path & archive, unz64_file_pos filepos)
{
	zlib_filefunc64_def zlibApi;

	zlibApi = api->getApiStructure();

	file = unzOpen2_64(archive.c_str(), &zlibApi);
	unzGoToFilePos64(file, &filepos);
	unzOpenCurrentFile(file);
}

CZipStream::~CZipStream()
{
	unzCloseCurrentFile(file);
	unzClose(file);
}

si64 CZipStream::readMore(ui8 * data, si64 size)
{
	return unzReadCurrentFile(file, data, (unsigned int)size);
}

si64 CZipStream::getSize()
{
	unz_file_info64 info;
	unzGetCurrentFileInfo64 (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	return info.uncompressed_size;
}

ui32 CZipStream::calculateCRC32()
{
	unz_file_info64 info;
	unzGetCurrentFileInfo64 (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	return info.crc;
}

///CZipLoader
CZipLoader::CZipLoader(const std::string & mountPoint, const boost::filesystem::path & archive, std::shared_ptr<CIOApi> api):
	ioApi(api),
    zlibApi(ioApi->getApiStructure()),
    archiveName(archive),
    mountPoint(mountPoint),
    files(listFiles(mountPoint, archive))
{
	logGlobal->trace("Zip archive loaded, %d files found", files.size());
}

std::unordered_map<ResourceID, unz64_file_pos> CZipLoader::listFiles(const std::string & mountPoint, const boost::filesystem::path & archive)
{
	std::unordered_map<ResourceID, unz64_file_pos> ret;

	unzFile file = unzOpen2_64(archive.c_str(), &zlibApi);

	if(file == nullptr)
		logGlobal->error("%s failed to open", archive.string());

	if (unzGoToFirstFile(file) == UNZ_OK)
	{
		do
		{
			unz_file_info64 info;
			std::vector<char> filename;
			// Fill unz_file_info structure with current file info
			unzGetCurrentFileInfo64 (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			filename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo64 (file, &info, filename.data(), (uLong)filename.size(), nullptr, 0, nullptr, 0);

			std::string filenameString(filename.data(), filename.size());
			unzGetFilePos64(file, &ret[ResourceID(mountPoint + filenameString)]);
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
	std::array<char, 8 * 1024> buffer{};

	unzOpenCurrentFile(file);

	while (true)
	{
		int readSize = unzReadCurrentFile(file, buffer.data(), (unsigned int)buffer.size());

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

std::vector<std::string> ZipArchive::listFiles(boost::filesystem::path filename)
{
	std::vector<std::string> ret;

	unzFile file = unzOpen2_64(filename.c_str(), FileStream::GetMinizipFilefunc());

	if (unzGoToFirstFile(file) == UNZ_OK)
	{
		do
		{
			unz_file_info64 info;
			std::vector<char> zipFilename;

			unzGetCurrentFileInfo64 (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			zipFilename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo64 (file, &info, zipFilename.data(), (uLong)zipFilename.size(), nullptr, 0, nullptr, 0);

			ret.emplace_back(zipFilename.data(), zipFilename.size());
		}
		while (unzGoToNextFile(file) == UNZ_OK);
	}
	unzClose(file);

	return ret;
}

bool ZipArchive::extract(boost::filesystem::path from, boost::filesystem::path where)
{
	// Note: may not be fast enough for large archives (should NOT happen with mods)
	// because locating each file by name may be slow. Unlikely slower than decompression though
	return extract(from, where, listFiles(from));
}

bool ZipArchive::extract(boost::filesystem::path from, boost::filesystem::path where, std::vector<std::string> what)
{
	unzFile archive = unzOpen2_64(from.c_str(), FileStream::GetMinizipFilefunc());

	auto onExit = vstd::makeScopeGuard([&]()
	{
		unzClose(archive);
	});

	for (const std::string & file : what)
	{
		if (unzLocateFile(archive, file.c_str(), 1) != UNZ_OK)
			return false;

		const boost::filesystem::path fullName = where / file;
		const boost::filesystem::path fullPath = fullName.parent_path();

		boost::filesystem::create_directories(fullPath);
		// directory. No file to extract
		// TODO: better way to detect directory? Probably check return value of unzOpenCurrentFile?
		if (boost::algorithm::ends_with(file, "/"))
			continue;

		FileStream destFile(fullName, std::ios::out | std::ios::binary);
		if (!destFile.good())
			return false;

		if (!extractCurrent(archive, destFile))
			return false;
	}
	return true;
}

VCMI_LIB_NAMESPACE_END
