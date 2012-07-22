
/*
 * CFileInputStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CInputStream.h"

class CFileInfo;

/**
 * A class which provides method definitions for reading a file from the filesystem.
 */
class DLL_LINKAGE CFileInputStream : public CInputStream
{
public:
	/**
	 * Standard c-tor.
	 */
	CFileInputStream();

	/**
	 * C-tor. Opens the specified file.
	 *
	 * @param file Path to the file.
	 *
	 * @throws std::runtime_error if file wasn't found
	 */
	CFileInputStream(const std::string & file);

	/**
	 * C-tor. Opens the specified file.
	 *
	 * @param file A file info object, pointing to a location in the file system
	 *
	 * @throws std::runtime_error if file wasn't found
	 */
	CFileInputStream(const CFileInfo & file);

	/**
	 * D-tor. Calls the close method implicitely, if the file is still opened.
	 */
	~CFileInputStream();

	/**
	 * Opens a file. If a file is currently opened, it will be closed.
	 *
	 * @param file Path to the file.
	 *
	 * @throws std::runtime_error if file wasn't found
	 */
	void open(const std::string & file);

	/**
	 * Opens a file.
	 *
	 * @param file A file info object, pointing to a location in the file system
	 *
	 * @throws std::runtime_error if file wasn't found
	 */
	void open(const CFileInfo & file);

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	si64 read(ui8 * data, si64 size);

	/**
	 * Seeks the internal read pointer to the specified position.
	 *
	 * @param position The read position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	si64 seek(si64 position);

	/**
	 * Gets the current read position in the stream.
	 *
	 * @return the read position. -1 on failure or if the read pointer isn't in the valid range.
	 */
	si64 tell();

	/**
	 * Skips delta numbers of bytes.
	 *
	 * @param delta The count of bytes to skip.
	 * @return the count of bytes skipped actually.
	 */
	si64 skip(si64 delta);

	/**
	 * Gets the length in bytes of the stream.
	 *
	 * @return the length in bytes of the stream.
	 */
	si64 getSize();

	/**
	 * Closes the stream and releases any system resources associated with the stream explicitely.
	 */
	void close();

private:
	/** Native c++ input file stream object. */
	std::ifstream fileStream;
};
