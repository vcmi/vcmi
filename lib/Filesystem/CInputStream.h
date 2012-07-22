
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

/**
 * Abstract class which provides method definitions for reading from a stream.
 */
class DLL_LINKAGE CInputStream : public boost::noncopyable
{
public:
	/**
	 * D-tor.
	 */
	virtual ~CInputStream() {}

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	virtual si64 read(ui8 * data, si64 size) = 0;

	/**
	 * Seeks the internal read pointer to the specified position.
	 *
	 * @param position The read position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	virtual si64 seek(si64 position) = 0;

	/**
	 * Gets the current read position in the stream.
	 *
	 * @return the read position.
	 */
	virtual si64 tell() = 0;

	/**
	 * Skips delta numbers of bytes.
	 *
	 * @param delta The count of bytes to skip.
	 * @return the count of bytes skipped actually.
	 */
	virtual si64 skip(si64 delta) = 0;

	/**
	 * Gets the length in bytes of the stream.
	 *
	 * @return the length in bytes of the stream.
	 */
	virtual si64 getSize() = 0;

	/**
	 * Closes the stream and releases any system resources associated with the stream explicitely.
	 */
	virtual void close() = 0;
};
