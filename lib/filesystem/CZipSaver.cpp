/*
 * CZipSaver.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CZipSaver.h"

VCMI_LIB_NAMESPACE_BEGIN

///CZipOutputStream
CZipOutputStream::CZipOutputStream(CZipSaver * owner_, zipFile archive, const std::string & archiveFilename):
	handle(archive),
	owner(owner_)
{
	zip_fileinfo fileInfo;

	std::time_t t = time(nullptr);
	fileInfo.dosDate = 0;

	struct tm * localTime = std::localtime(&t);
	fileInfo.tmz_date.tm_hour = localTime->tm_hour;
	fileInfo.tmz_date.tm_mday = localTime->tm_mday;
	fileInfo.tmz_date.tm_min  = localTime->tm_min;
	fileInfo.tmz_date.tm_mon  = localTime->tm_mon;
	fileInfo.tmz_date.tm_sec  = localTime->tm_sec;
	fileInfo.tmz_date.tm_year = localTime->tm_year;

	fileInfo.external_fa = 0; //???
	fileInfo.internal_fa = 0;

	int status = zipOpenNewFileInZip4_64(
						handle,
						archiveFilename.c_str(),
						&fileInfo,
						nullptr,//extrafield_local
						0,
						nullptr,//extrafield_global
						0,
						nullptr,//comment
						Z_DEFLATED,
						Z_DEFAULT_COMPRESSION,
						0,//raw
						-15,//windowBits
						9,//memLevel
						Z_DEFAULT_STRATEGY,//strategy
						nullptr,//password
						0,//crcForCrypting
						20,//versionMadeBy
						0,//flagBase
						0//zip64
						);

    if(status != ZIP_OK)
		throw std::runtime_error("CZipOutputStream: zipOpenNewFileInZip failed");

	owner->activeStream = this;
}

CZipOutputStream::~CZipOutputStream()
{
	int status = zipCloseFileInZip(handle);
	if (status != ZIP_OK)
		logGlobal->error("CZipOutputStream: stream finalize failed: %d", static_cast<int>(status));
	owner->activeStream = nullptr;
}

si64 CZipOutputStream::write(const ui8 * data, si64 size)
{
	int ret = zipWriteInFileInZip(handle, (const void*)data, (unsigned)size);

	if (ret == ZIP_OK)
		return size;
	else
		return 0;
}

///CZipSaver
CZipSaver::CZipSaver(std::shared_ptr<CIOApi> api, const boost::filesystem::path & path):
	ioApi(api),
	zipApi(ioApi->getApiStructure()),
	handle(nullptr),
	activeStream(nullptr)
{
	handle = zipOpen2_64((const void *) & path, APPEND_STATUS_CREATE, nullptr, &zipApi);

	if (handle == nullptr)
		throw std::runtime_error("CZipSaver: Failed to create archive");
}

CZipSaver::~CZipSaver()
{
	if(activeStream != nullptr)
	{
		logGlobal->error("CZipSaver::~CZipSaver: active stream found");
		zipCloseFileInZip(handle);
	}


	if(handle != nullptr)
	{
		int status = zipClose(handle, nullptr);
		if (status != ZIP_OK)
			logGlobal->error("CZipSaver: archive finalize failed: %d", static_cast<int>(status));
	}

}

std::unique_ptr<COutputStream> CZipSaver::addFile(const std::string & archiveFilename)
{
	if(activeStream != nullptr)
		throw std::runtime_error("CZipSaver::addFile: stream already opened");

	std::unique_ptr<COutputStream> stream(new CZipOutputStream(this, handle, archiveFilename));
	return stream;
}


VCMI_LIB_NAMESPACE_END
