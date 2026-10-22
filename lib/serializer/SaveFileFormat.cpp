/*
 * SaveFileFormat.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SaveFileFormat.h"

SaveTooOldException::~SaveTooOldException() = default;

std::string SaveFileFormat::toString(const ChunkID & id)
{
	return std::string(id.begin(), id.end());
}

void SaveFileFormat::writeUInt32(std::vector<std::byte> & target, uint32_t value)
{
	for(int shift = 0; shift < 32; shift += 8)
		target.push_back(static_cast<std::byte>((value >> shift) & 0xff));
}

uint32_t SaveFileFormat::readUInt32(const std::byte * source)
{
	uint32_t result = 0;
	for(int index = 0; index < 4; ++index)
		result |= std::to_integer<uint32_t>(source[index]) << (index * 8);
	return result;
}
