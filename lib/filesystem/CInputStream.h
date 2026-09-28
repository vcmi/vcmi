/*
 * CInputStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CStream.h"

/**
 * Abstract class which provides method definitions for reading from a stream.
 */
class DLL_LINKAGE CInputStream : public virtual CStream
{
public:
	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	virtual si64 read(ui8 * data, si64 size) = 0;

	/**
	 * @brief for convenience, reads whole stream at once
	 *
	 * @return pair, first = raw data, second = size of data
	 */
	virtual std::pair<std::unique_ptr<ui8[]>, si64> readAll();

	/**
	 * @brief calculateCRC32 calculates CRC32 checksum for the whole file
	 * @return calculated checksum
	 */
	virtual ui32 calculateCRC32();
};
