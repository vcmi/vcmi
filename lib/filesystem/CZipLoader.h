/*
 * CZipLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ISimpleResourceLoader.h"
#include "CInputStream.h"
#include "ResourcePath.h"
#include "CCompressedStream.h"

#include "MinizipExtensions.h"

#include <functional>

class CZipStream : public CBufferedStream
{
	unzFile file;
	uint64_t archiveId;

public:
	/// Takes ownership of an opened archive handle. On destruction the handle is kept for reuse, or closed if archiveId is 0
	CZipStream(unzFile file, uint64_t archiveId, unz64_file_pos filepos);
	~CZipStream();

	si64 getSize() override;
	ui32 calculateCRC32() override;

protected:
	si64 readMore(ui8 * data, si64 size) override;
};

class CZipLoader : public ISimpleResourceLoader
{
	std::shared_ptr<CIOApi> ioApi;
	zlib_filefunc64_def zlibApi;
	boost::filesystem::path archiveName;
	std::string mountPoint;
	uint64_t archiveId; ///< identifies handles of this archive kept for reuse, 0 if handles are not reused

	std::unordered_map<ResourcePath, unz64_file_pos> files;

	std::unordered_map<ResourcePath, unz64_file_pos> listFiles(const std::string & mountPoint, const boost::filesystem::path &archive);
public:
	CZipLoader(const std::string & mountPoint, const boost::filesystem::path & archive, std::shared_ptr<CIOApi> api = std::make_shared<CDefaultIOApi>());

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourcePath & resourceName) const override;
	bool existsResource(const ResourcePath & resourceName) const override;
	std::string getMountPoint() const override;
	bool updateFilteredFiles(std::function<bool(const std::string &)> filter) override { return false; }
	std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const override;
	std::string getFullFileURI(const ResourcePath& resourceName) const override;
	std::time_t getLastWriteTime(const ResourcePath& resourceName) const override;
};

class DLL_LINKAGE ZipArchive : boost::noncopyable
{
	unzFile archive;

public:
	struct Entry
	{
		std::string name;
		uint64_t uncompressedSize = 0;
		bool directory = false;
		bool isUtf8Filename = false;
	};

	/// Receives the number of extracted bytes and returns false to cancel extraction.
	using Progress = std::function<bool(uint64_t)>;

	ZipArchive(const boost::filesystem::path & from);
	~ZipArchive();

	/// Returns the cached central directory in archive order.
	const std::vector<Entry> & entries();
	std::vector<std::string> listFiles();
	/// Extracts a single entry below "where". relativeName is the path inside the archive.
	bool extractTo(const boost::filesystem::path & where, size_t entryIndex, const std::string & relativeName, const Progress & progress = {});
	bool extract(const boost::filesystem::path & where, const std::vector<std::string> & what);
	bool extract(const boost::filesystem::path & where, const std::string & what);

private:
	bool indexed = false;
	std::vector<Entry> indexedEntries;
	std::vector<unz64_file_pos> positions;
	std::unordered_map<std::string, size_t> names;
	void readIndex();
};
