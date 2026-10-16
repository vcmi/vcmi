/*
 * CLegacyLoadFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CLegacyLoadFile.h"

#include "CLoadFile.h"
#include "CSerializer.h"

#include "../StartInfo.h"
#include "../campaign/CampaignState.h"
#include "../mapping/CMapHeader.h"
#include "../modding/ActiveModsInSaveList.h"
#include "../rmg/CMapGenOptions.h"
#include "../texts/TextOperations.h"

CLegacyLoadFile::CLegacyLoadFile(const boost::filesystem::path & fname, IGameInfoCallback * cb)
	: serializer(this)
	, sfile(fname.c_str(), std::ios::in | std::ios::binary)
{
	serializer.cb = cb;
	serializer.loadingGamestate = true;
	serializer.version = CLoadFile::readVersion(fname);

	sfile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

	if(!sfile)
		throw std::runtime_error("Error: cannot open file '" + TextOperations::filesystemPathToUtf8(fname) + "' for reading!");

	sfile.seekg(SaveFileFormat::FILE_HEADER_SIZE);

	std::string loaded = SAVEGAME_MAGIC;
	sfile.read(loaded.data(), SAVEGAME_MAGIC.length());
	if(loaded != SAVEGAME_MAGIC)
		throw std::runtime_error("Magic bytes doesn't match!");
}

int CLegacyLoadFile::read(std::byte * data, unsigned size)
{
	sfile.read(reinterpret_cast<char *>(data), size);
	return size;
}

void CLegacyLoadFile::loadHeader(CMapHeader & mapHeader, StartInfo & startInfo)
{
	serializer & mapHeader;
	serializer & startInfo;
}

BinaryDeserializer & CLegacyLoadFile::skipToGameState()
{
	CMapHeader dummyHeader;
	StartInfo dummyStartInfo;
	ActiveModsInSaveList activeMods;

	loadHeader(dummyHeader, dummyStartInfo);
	serializer & activeMods;
	return serializer;
}
