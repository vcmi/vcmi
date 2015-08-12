#pragma once

/*
 * CZipSaver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "COutputStream.h"

#include "MinizipExtensions.h"

class CZipSaver;

class DLL_LINKAGE CZipOutputStream: public COutputStream
{
public:
	/**
	 * @brief constructs zip stream from already opened file
	 * @param archive archive handle, must be opened
	 * @param archiveFilename name of file to write
	 */	
	explicit CZipOutputStream(CZipSaver * owner_, zipFile archive, const std::string & archiveFilename);
	~CZipOutputStream();
	
	si64 write(const ui8 * data, si64 size) override;

	si64 seek(si64 position) override {return -1;};
	si64 tell() override {return 0;};
	si64 skip(si64 delta) override {return 0;};
	si64 getSize() override {return 0;};	
private:
	zipFile handle;
	CZipSaver * owner;
};

class DLL_LINKAGE CZipSaver
{
public:	
	explicit CZipSaver(std::shared_ptr<CIOApi> api, const std::string & path);
	virtual ~CZipSaver();
	
	std::unique_ptr<COutputStream> addFile(const std::string & archiveFilename);
private:
	std::shared_ptr<CIOApi> ioApi;
	zlib_filefunc64_def zipApi;	
	
	zipFile handle;
	
	///due to minizip design only one file stream may opened at a time
	COutputStream * activeStream;
	friend class CZipOutputStream;
};
