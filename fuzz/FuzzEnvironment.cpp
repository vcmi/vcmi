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
#include <mutex>

namespace fuzzing
{
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
		LIBRARY = new GameLibrary;
		LIBRARY->initializeFilesystem(false);
		LIBRARY->initializeLibrary();
	});
}
}
