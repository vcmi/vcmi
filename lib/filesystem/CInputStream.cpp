/*
 * CInputStream.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CInputStream.h"

#include <zlib.h>

std::pair<std::unique_ptr<ui8[]>, si64> CInputStream::readAll()
{
	std::unique_ptr<ui8[]> data(new ui8[getSize()]);

	seek(0);
	[[maybe_unused]] auto readSize = read(data.get(), getSize());
	assert(readSize == getSize());

	return std::make_pair(std::move(data), getSize());
}

ui32 CInputStream::calculateCRC32()
{
	si64 originalPos = tell();

	auto data = readAll();
	ui32 checksum = static_cast<ui32>(crc32_z(0, data.first.get(), data.second));

	seek(originalPos);

	return checksum;
}
