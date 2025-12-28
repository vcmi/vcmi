/*
 * CSaveFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSaveFile.h"

VCMI_LIB_NAMESPACE_BEGIN

static const char * SAVE_HEADER = "VCMI";

SaveFile::SaveFile()
{
	saveData.reserve(128*1024);

	write(reinterpret_cast<const std::byte*>(SAVE_HEADER), 4); //write magic identifier
	serializer & ESerializationVersion::CURRENT; //write format version
	write(reinterpret_cast<const std::byte*>(SAVEGAME_MAGIC.c_str()), SAVEGAME_MAGIC.length());
}

void SaveFile::writeFile(const boost::filesystem::path & fileName)
{
	std::ofstream sfile(fileName.c_str(), std::ios::out | std::ios::binary);

	sfile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

	if(!sfile)
		throw std::runtime_error("Error: cannot open file '" + fileName.string() + "' for writing!");

	sfile.write(reinterpret_cast<const char *>(saveData.data()), saveData.size());
}

int SaveFile::write(const std::byte * data, unsigned size)
{
	saveData.insert(saveData.end(), data, data + size);
	return size;
}

int VerifyFile::write(const std::byte * data, unsigned size)
{
	if (std::equal(data, data + size, testSaveData.data() + testPosition))
	{
		testPosition += size;
	}
	else
	{
		logGlobal->error("GAMESTATE DESYNC FOUND! GAME WILL NOW TERMINATE");
		throw std::runtime_error("GAMESTATE DESYNC FOUND! GAME WILL NOW TERMINATE");
	}
	return size;
}

VerifyFile::VerifyFile(const std::vector<std::byte> & saveData)
	: testSaveData(saveData)
	, testPosition(0)
{
	write(reinterpret_cast<const std::byte*>(SAVE_HEADER), 4); //write magic identifier
	serializer & ESerializationVersion::CURRENT; //write format version
	write(reinterpret_cast<const std::byte*>(SAVEGAME_MAGIC.c_str()), SAVEGAME_MAGIC.length());
}


VCMI_LIB_NAMESPACE_END
