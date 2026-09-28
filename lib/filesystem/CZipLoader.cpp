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

namespace
{
/// Performance optimization: every unzOpen scans up to 64 KB at the end of the archive for its central directory.
/// Each thread keeps the handle of the last CZipStream destroyed on this thread and reuses it for the next load from the same archive.
/// Files are mostly loaded in runs from one archive, so this removes most reopens while keeping at most one idle handle per thread
struct IdleZipHandle
{
	uint64_t archiveId = 0;
	unzFile handle = nullptr;

	~IdleZipHandle()
	{
		if(handle)
			unzClose(handle);
	}
};

thread_local IdleZipHandle idleHandle;
std::atomic<uint64_t> nextArchiveId = 1;
}

CZipStream::CZipStream(unzFile file, uint64_t archiveId, unz64_file_pos filepos):
	file(file),
	archiveId(archiveId)
{
	unzGoToFilePos64(file, &filepos);
	unzOpenCurrentFile(file);
}

CZipStream::~CZipStream()
{
	unzCloseCurrentFile(file);

	if(archiveId == 0)
	{
		unzClose(file);
		return;
	}

	if(idleHandle.handle)
		unzClose(idleHandle.handle);
	idleHandle.archiveId = archiveId;
	idleHandle.handle = file;
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
	// handles of other I/O APIs may use streams that are destroyed together with the loader, so they are never kept for reuse
	archiveId(dynamic_cast<CDefaultIOApi *>(ioApi.get()) ? nextArchiveId++ : 0),
	files(listFiles(mountPoint, archive))
{
	logGlobal->trace("Zip archive loaded, %d files found", files.size());
}

std::unordered_map<ResourcePath, unz64_file_pos> CZipLoader::listFiles(const std::string & mountPoint, const boost::filesystem::path & archive)
{
	std::unordered_map<ResourcePath, unz64_file_pos> ret;

	unzFile file = unzOpen2_64(archive.c_str(), &zlibApi);

	if(file == nullptr)
		logGlobal->error("%s failed to open", TextOperations::filesystemPathToUtf8(archive));

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
	unz64_file_pos filepos = files.at(resourceName);
	unzFile handle = nullptr;

	if(archiveId != 0 && idleHandle.archiveId == archiveId)
		handle = std::exchange(idleHandle.handle, nullptr);

	if(handle == nullptr)
	{
		zlib_filefunc64_def api = zlibApi;
		handle = unzOpen2_64(archiveName.c_str(), &api);
	}

	return std::make_unique<CZipStream>(handle, archiveId, filepos);
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

boost::filesystem::path zipFilenameToFilesystemPath(const std::string & filename, bool isUtf8)
{
#ifdef VCMI_WINDOWS
	if (isUtf8)
		return TextOperations::Utf8TofilesystemPath(filename);

	return boost::filesystem::path(filename);
#else
	return boost::filesystem::path(filename);
#endif
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
		throw std::runtime_error("Failed to open file '" + TextOperations::filesystemPathToUtf8(from));
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

	unz_file_info64 info;
	unzGetCurrentFileInfo64(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

	constexpr uLong ZIP_UTF8_FILENAME_FLAG = 1 << 11;
	const bool isUtf8Filename = (info.flag & ZIP_UTF8_FILENAME_FLAG) != 0;

	const boost::filesystem::path relativeName = zipFilenameToFilesystemPath(file, isUtf8Filename);
	const boost::filesystem::path fullName = where / relativeName;
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
			logGlobal->error("Failed to open file '%s'", TextOperations::filesystemPathToUtf8(fullName));
		else
			logGlobal->error("Failed to open file with long path '%s' (%d characters)", TextOperations::filesystemPathToUtf8(fullName), fullName.size());
#else
		logGlobal->error("Failed to open file '%s'", fullName.c_str());
#endif

		return false;
	}

	if (!extractCurrent(archive, destFile))
		return false;
	return true;
}
