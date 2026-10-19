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

#include <boost/filesystem/fstream.hpp>

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

void ZipArchive::readIndex()
{
	if(indexed)
		return;

	std::vector<Entry> newEntries;
	std::vector<unz64_file_pos> newPositions;
	std::unordered_map<std::string, size_t> newNames;
	unz_global_info64 globalInfo{};
	if(unzGetGlobalInfo64(archive, &globalInfo) != UNZ_OK)
		throw std::runtime_error("Failed to read zip central directory");

	int result = unzGoToFirstFile(archive);
	while(result == UNZ_OK)
	{
		unz_file_info64 info{};
		unz64_file_pos position{};
		if(unzGetCurrentFileInfo64(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK)
			throw std::runtime_error("Failed to read zip entry metadata");

		std::vector<char> filename(info.size_filename + 1, '\0');
		if(unzGetCurrentFileInfo64(archive, &info, filename.data(), static_cast<uLong>(filename.size()),
			nullptr, 0, nullptr, 0) != UNZ_OK || unzGetFilePos64(archive, &position) != UNZ_OK)
			throw std::runtime_error("Failed to index zip entry");

		std::string name(filename.data(), info.size_filename);
		if(name.empty() || name.find('\0') != std::string::npos)
			throw std::runtime_error("Invalid zip entry name");

		constexpr uLong ZIP_UTF8_FILENAME_FLAG = 1 << 11;
		const auto unixType = (info.external_fa >> 16) & 0170000;
		const bool directory = name.back() == '/' || unixType == 0040000 || (info.external_fa & 0x10) != 0;
		newNames.emplace(name, newEntries.size());
		newEntries.push_back({std::move(name), info.compressed_size, info.uncompressed_size,
			static_cast<uint32_t>(info.crc), directory, (info.flag & 1) != 0, unixType == 0120000,
			(info.flag & ZIP_UTF8_FILENAME_FLAG) != 0});
		newPositions.push_back(position);
		result = unzGoToNextFile(archive);
	}
	if(result != UNZ_END_OF_LIST_OF_FILE || newEntries.size() != globalInfo.number_entry)
		throw std::runtime_error("Invalid or truncated zip central directory");

	indexedEntries = std::move(newEntries);
	positions = std::move(newPositions);
	names = std::move(newNames);
	indexed = true;
}

const std::vector<ZipArchive::Entry> & ZipArchive::entries()
{
	readIndex();
	return indexedEntries;
}

std::vector<std::string> ZipArchive::listFiles()
{
	std::vector<std::string> result;
	for(const auto & entry : entries())
		result.push_back(entry.name);
	return result;
}

ZipArchive::ZipArchive(const boost::filesystem::path & from)
{
	CDefaultIOApi zipAPI;
	auto zipStructure = zipAPI.getApiStructure();
	archive = unzOpen2_64(from.c_str(), &zipStructure);

	if (archive == nullptr)
		throw std::runtime_error("Failed to open file '" + TextOperations::filesystemPathToUtf8(from) + "'");
}

ZipArchive::~ZipArchive()
{
	unzClose(archive);
}

namespace
{
bool prepareZipDestination(const boost::filesystem::path & root, const boost::filesystem::path & relative,
	bool directory)
{
	if(root.empty() || relative.has_root_path() || (!directory && relative.empty()))
		return false;

	const auto name = TextOperations::filesystemPathToUtf8(relative);
	if(name.find_first_of("\\:\0", 0, 3) != std::string::npos)
		return false;

	for(auto it = relative.begin(); it != relative.end(); ++it)
	{
		const bool trailingSeparator = directory && !name.empty() && name.back() == '/' && std::next(it) == relative.end();
		if(*it == ".." || (*it == "." && !trailingSeparator))
			return false;
	}

	boost::system::error_code error;
	if(boost::filesystem::is_symlink(root, error))
		return false;
	error.clear();
	boost::filesystem::create_directories(root, error);
	if(error)
		return false;

	auto current = root;
	for(auto it = relative.begin(); it != relative.end(); ++it)
	{
		current /= *it;
		error.clear();
		const auto status = boost::filesystem::symlink_status(current, error);
		if(error && error != boost::system::errc::no_such_file_or_directory)
			return false;
		if(boost::filesystem::is_symlink(status))
			return false;

		const bool isDirectory = directory || std::next(it) != relative.end();
		if(isDirectory)
		{
			error.clear();
			boost::filesystem::create_directory(current, error);
			if(error || !boost::filesystem::is_directory(current))
				return false;
		}
		else if(boost::filesystem::exists(status) && !boost::filesystem::is_regular_file(status))
			return false;
	}
	return true;
}
}

bool ZipArchive::extractTo(const boost::filesystem::path & root, size_t entryIndex,
	const boost::filesystem::path & relativePath, const Progress & progress)
{
	const auto & allEntries = entries();
	if(entryIndex >= allEntries.size())
		return false;
	const auto & entry = allEntries.at(entryIndex);
	if(entry.encrypted || entry.symlink || (progress && !progress(0)))
		return false;
	if(!prepareZipDestination(root, relativePath, entry.directory))
		return false;
	if(entry.directory)
		return true;
	if(unzGoToFilePos64(archive, &positions.at(entryIndex)) != UNZ_OK || unzOpenCurrentFile(archive) != UNZ_OK)
		return false;

	bool entryOpen = true;
	bool success = false;
	const auto destination = root / relativePath;
	boost::filesystem::ofstream output(destination, std::ios::out | std::ios::binary | std::ios::trunc);
	auto cleanup = vstd::makeScopeGuard([&]()
	{
		if(entryOpen)
			unzCloseCurrentFile(archive);
		if(output.is_open())
			output.close();
		if(!success)
		{
			boost::system::error_code error;
			boost::filesystem::remove(destination, error);
			if(error)
				logGlobal->error("Failed to remove incomplete zip entry '%s': %s", TextOperations::filesystemPathToUtf8(destination), error.message());
		}
	});
	if(!output.good())
		return false;

	std::array<char, 64 * 1024> buffer{};
	uint64_t written = 0;
	while(true)
	{
		const int count = unzReadCurrentFile(archive, buffer.data(), static_cast<unsigned int>(buffer.size()));
		if(count < 0)
			return false;
		if(count == 0)
			break;
		if(static_cast<uint64_t>(count) > entry.uncompressedSize - written)
			return false;
		output.write(buffer.data(), count);
		if(!output.good())
			return false;
		written += count;
		if(progress && !progress(count))
			return false;
	}
	const int closeResult = unzCloseCurrentFile(archive);
	entryOpen = false;
	output.flush();
	if(closeResult != UNZ_OK || written != entry.uncompressedSize || !output.good())
		return false;
	output.close();
	if(output.fail())
		return false;
	success = true;
	return true;
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
	readIndex();
	const auto found = names.find(file);
	if(found == names.end())
		return false;
	return extractTo(where, found->second,
		zipFilenameToFilesystemPath(file, indexedEntries.at(found->second).isUtf8Filename));
}
