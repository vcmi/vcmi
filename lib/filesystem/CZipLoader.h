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
#include "Filesystem.h"
#include "CCompressedStream.h"

// Necessary here in order to get all types
#include "../minizip/unzip.h"

class DLL_LINKAGE CZipStream : public CBufferedStream
{
	unzFile file;

public:
	/**
	 * @brief constructs zip stream from already opened file
	 * @param archive path to archive to open
	 * @param filepos position of file to open
	 */
	CZipStream(const std::string & archive, unz_file_pos filepos);
	~CZipStream();

	si64 getSize();

protected:
	si64 readMore(ui8 * data, si64 size) override;
};

class DLL_LINKAGE CZipLoader : public ISimpleResourceLoader
{
	std::string archiveName;
	std::string mountPoint;

	std::unordered_map<ResourceID, unz_file_pos> files;

	std::unordered_map<ResourceID, unz_file_pos> listFiles(const std::string & mountPoint, const std::string &archive);
public:
	CZipLoader(const std::string & mountPoint, const std::string & archive);

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const override;
	bool existsResource(const ResourceID & resourceName) const override;
	std::string getMountPoint() const override;
	std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const override;
};