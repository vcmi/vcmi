#include "StdInc.h"
#include "CLoadIntegrityValidator.h"

#include "../registerTypes/RegisterTypes.h"

/*
 * CLoadIntegrityValidator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CLoadIntegrityValidator::CLoadIntegrityValidator(const boost::filesystem::path &primaryFileName, const boost::filesystem::path &controlFileName, int minimalVersion /*= version*/)
	: serializer(this), foundDesync(false)
{
	registerTypes(serializer);
	primaryFile = make_unique<CLoadFile>(primaryFileName, minimalVersion);
	controlFile = make_unique<CLoadFile>(controlFileName, minimalVersion);

	assert(primaryFile->serializer.fileVersion == controlFile->serializer.fileVersion);
	serializer.fileVersion = primaryFile->serializer.fileVersion;
}

int CLoadIntegrityValidator::read( void * data, unsigned size )
{
	assert(primaryFile);
	assert(controlFile);

	if(!size)
		return size;

	std::vector<ui8> controlData(size);
	auto ret = primaryFile->read(data, size);

	if(!foundDesync)
	{
		controlFile->read(controlData.data(), size);
		if(std::memcmp(data, controlData.data(), size))
		{
			logGlobal->errorStream() << "Desync found! Position: " << primaryFile->sfile->tellg();
			foundDesync = true;
			//throw std::runtime_error("Savegame dsynchronized!");
		}
	}
	return ret;
}

std::unique_ptr<CLoadFile> CLoadIntegrityValidator::decay()
{
	primaryFile->serializer.loadedPointers = this->serializer.loadedPointers;
	primaryFile->serializer.loadedPointersTypes = this->serializer.loadedPointersTypes;
	return std::move(primaryFile);
}

void CLoadIntegrityValidator::checkMagicBytes( const std::string &text )
{
	assert(primaryFile);
	assert(controlFile);

	primaryFile->checkMagicBytes(text);
	controlFile->checkMagicBytes(text);
}
