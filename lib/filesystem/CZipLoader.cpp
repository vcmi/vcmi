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

#include "../ScopeGuard.h"
#include "../texts/TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

CZipStream::CZipStream(const std::shared_ptr<CIOApi> & api, const boost::filesystem::path & archive, unz64_file_pos filepos)
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
	return unzReadCurrentFile(file, data, static_cast<unsigned int>(size));
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
	ioApi(std::move(api)),
	zlibApi(ioApi->getApiStructure()),
	archiveName(archive),
	mountPoint(mountPoint),
	files(listFiles(mountPoint, archive))
{
	logGlobal->trace("Zip archive loaded, %d files found", files.size());
}

std::unordered_map<ResourcePath, unz64_file_pos> CZipLoader::listFiles(const std::string & mountPoint, const boost::filesystem::path & archive)
{
	std::unordered_map<ResourcePath, unz64_file_pos> ret;

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
			unzGetCurrentFileInfo64(file, &info, filename.data(), static_cast<uLong>(filename.size()), nullptr, 0, nullptr, 0);

			std::string filenameString(filename.data(), filename.size());
			unzGetFilePos64(file, &ret[ResourcePath(mountPoint + filenameString)]);
		}
		while (unzGoToNextFile(file) == UNZ_OK);
	}
	unzClose(file);

	return ret;
}

std::unique_ptr<CInputStream> CZipLoader::load(const ResourcePath & resourceName) const
{
	return std::unique_ptr<CInputStream>(new CZipStream(ioApi, archiveName, files.at(resourceName)));
}

bool CZipLoader::existsResource(const ResourcePath & resourceName) const
{
	return files.count(resourceName) != 0;
}

std::string CZipLoader::getMountPoint() const
{
	return mountPoint;
}

std::unordered_set<ResourcePath> CZipLoader::getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const
{
	std::unordered_set<ResourcePath> foundID;

	for(const auto & file : files)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}

std::string CZipLoader::getFullFileURI(const ResourcePath& resourceName) const
{
	auto relativePath = TextOperations::Utf8TofilesystemPath(resourceName.getName());
	auto path = boost::filesystem::canonical(archiveName) / relativePath;
	return TextOperations::filesystemPathToUtf8(path);
}

std::time_t CZipLoader::getLastWriteTime(const ResourcePath& resourceName) const
{
	auto path = boost::filesystem::canonical(archiveName);
	return  boost::filesystem::last_write_time(path);
}

/// extracts currently selected file from zip into stream "where"
static bool extractCurrent(unzFile file, std::ostream & where)
{
	std::array<char, 8 * 1024> buffer{};

	unzOpenCurrentFile(file);

	while(true)
	{
		int readSize = unzReadCurrentFile(file, buffer.data(), static_cast<unsigned int>(buffer.size()));

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

std::vector<std::string> ZipArchive::listFiles()
{
	std::vector<std::string> ret;

	int result = unzGoToFirstFile(archive);

	if (result == UNZ_OK)
	{
		do
		{
			unz_file_info64 info;
			std::vector<char> zipFilename;

			unzGetCurrentFileInfo64 (archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			zipFilename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo64(archive, &info, zipFilename.data(), static_cast<uLong>(zipFilename.size()), nullptr, 0, nullptr, 0);

			ret.emplace_back(zipFilename.data(), zipFilename.size());

			result = unzGoToNextFile(archive);
		}
		while (result == UNZ_OK);
	}
	return ret;
}

ZipArchive::ZipArchive(const boost::filesystem::path & from)
{
	CDefaultIOApi zipAPI;

#if MINIZIP_NEEDS_32BIT_FUNCS
	auto zipStructure = zipAPI.getApiStructure32();
	archive = unzOpen2(from.c_str(), &zipStructure);
#else
	auto zipStructure = zipAPI.getApiStructure();
	archive = unzOpen2_64(from.c_str(), &zipStructure);
#endif

	if (archive == nullptr)
		throw std::runtime_error("Failed to open file '" + from.string());
}

ZipArchive::~ZipArchive()
{
	unzClose(archive);
}

bool ZipArchive::extract(const boost::filesystem::path & where, const std::vector<std::string> & what)
{
	for (const std::string & file : what)
		if (!extract(where, file))
			return false;

	return true;
}

bool ZipArchive::extract(const boost::filesystem::path & where, const std::string & file)
{
	if (unzLocateFile(archive, file.c_str(), 1) != UNZ_OK)
		return false;

	const boost::filesystem::path fullName = where / file;
	const boost::filesystem::path fullPath = fullName.parent_path();

	boost::filesystem::create_directories(fullPath);
	// directory. No file to extract
	// TODO: better way to detect directory? Probably check return value of unzOpenCurrentFile?
	if (boost::algorithm::ends_with(file, "/"))
		return true;

	std::fstream destFile(fullName.c_str(), std::ios::out | std::ios::binary);
	if (!destFile.good())
	{
#ifdef VCMI_WINDOWS
		if (fullName.size() < 260)
			logGlobal->error("Failed to open file '%s'", fullName.string());
		else
			logGlobal->error("Failed to open file with long path '%s' (%d characters)", fullName.string(), fullName.size());
#else
		logGlobal->error("Failed to open file '%s'", fullName.c_str());
#endif

		return false;
	}

	if (!extractCurrent(archive, destFile))
		return false;
	return true;
}

VCMI_LIB_NAMESPACE_END
