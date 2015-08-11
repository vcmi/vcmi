#pragma once

/*
 * CZipLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ISimpleResourceLoader.h"
#include "CInputStream.h"
#include "ResourceID.h"
#include "CCompressedStream.h"

#include "MinizipExtensions.h"

class DLL_LINKAGE CZipStream : public CBufferedStream
{
	unzFile file;

public:
	/**
	 * @brief constructs zip stream from already opened file
	 * @param api virtual filesystem interface
	 * @param archive path to archive to open
	 * @param filepos position of file to open
	 */
	CZipStream(std::shared_ptr<CIOApi> api, const std::string & archive, unz_file_pos filepos);
		
	~CZipStream();

	si64 getSize() override;
	ui32 calculateCRC32() override;

protected:
	si64 readMore(ui8 * data, si64 size) override;
};

class DLL_LINKAGE CZipLoader : public ISimpleResourceLoader
{
	std::shared_ptr<CIOApi> ioApi;
	zlib_filefunc64_def zlibApi;
	
	std::string archiveName;
	std::string mountPoint;

	std::unordered_map<ResourceID, unz_file_pos> files;

	std::unordered_map<ResourceID, unz_file_pos> listFiles(const std::string & mountPoint, const std::string &archive);
public:
	CZipLoader(const std::string & mountPoint, const std::string & archive, std::shared_ptr<CIOApi> api = std::shared_ptr<CIOApi>(new CDefaultIOApi()));
	
	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const override;
	bool existsResource(const ResourceID & resourceName) const override;
	std::string getMountPoint() const override;
	std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const override;
};


namespace ZipArchive
{
	/// List all files present in archive
	std::vector<std::string> DLL_LINKAGE listFiles(std::string filename);

	/// extracts all files from archive "from" into destination directory "where". Directory must exist
	bool DLL_LINKAGE extract(std::string from, std::string where);

	///same as above, but extracts only files mentioned in "what" list
	bool DLL_LINKAGE extract(std::string from, std::string where, std::vector<std::string> what);
}
