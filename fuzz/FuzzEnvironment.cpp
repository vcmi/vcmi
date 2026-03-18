/*
 * FuzzEnvironment.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "FuzzEnvironment.h"

#include "../lib/GameLibrary.h"

#include <algorithm>
#include <fstream>
#include <mutex>

#include <boost/dll/runtime_symbol_info.hpp>

namespace fuzzing
{
namespace
{
void ensureDevelopmentModeMarker()
{
	const bool hasDataDirectories = boost::filesystem::exists("config") && boost::filesystem::exists("Mods");
	const bool hasAnyBinary = boost::filesystem::exists("vcmiclient")
		|| boost::filesystem::exists("vcmiserver")
		|| boost::filesystem::exists("vcmilobby");

	if(hasDataDirectories && !hasAnyBinary)
	{
		std::ofstream marker("vcmiserver", std::ios::app);
	}
}

void ensureResourceWorkingDirectory()
{
	const auto executableDirectory = boost::dll::program_location().parent_path();
	const auto initialDirectory = boost::filesystem::current_path();

	for(const auto & candidate : {executableDirectory, executableDirectory.parent_path(), initialDirectory})
	{
		if(boost::filesystem::exists(candidate / "config/filesystem.json") && boost::filesystem::exists(candidate / "Mods"))
		{
			boost::filesystem::current_path(candidate);
			break;
		}
	}

	ensureDevelopmentModeMarker();
}
}

ByteReader::ByteReader(const uint8_t * data_, size_t size_)
	: data(data_)
	, size(size_)
	, offset(0)
{
}

bool ByteReader::empty() const
{
	return remaining() == 0;
}

size_t ByteReader::remaining() const
{
	return size - std::min(offset, size);
}

uint8_t ByteReader::readByte()
{
	if(offset >= size)
		return 0;

	return data[offset++];
}

bool ByteReader::readBool()
{
	return (readByte() & 1U) != 0;
}

uint32_t ByteReader::readU32()
{
	uint32_t value = 0;
	for(int i = 0; i < 4; ++i)
		value |= static_cast<uint32_t>(readByte()) << (i * 8);
	return value;
}

int ByteReader::readInt()
{
	const uint32_t value = readU32() & 0x7fffffffU;
	return static_cast<int>(value);
}

int ByteReader::readInRange(int minValue, int maxValue)
{
	if(minValue >= maxValue)
		return minValue;

	const uint32_t range = static_cast<uint32_t>(maxValue - minValue + 1);
	return minValue + static_cast<int>(readU32() % range);
}

void initializeEngine()
{
	static std::once_flag once;

	std::call_once(once, []()
	{
		ensureResourceWorkingDirectory();
		LIBRARY = new GameLibrary;
		LIBRARY->initializeFilesystem(false);
		LIBRARY->initializeLibrary();
	});
}
}
