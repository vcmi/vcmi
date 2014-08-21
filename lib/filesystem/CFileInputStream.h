#pragma once

/*
 * CFileInputStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CInputStream.h"

class CFileInfo;

/**
 * A class which provides method definitions for reading a file from the filesystem.
 */
class DLL_LINKAGE CFileInputStream : public CInputStream
{
public:
	/**
	 * C-tor. Opens the specified file.
	 *
	 * @see CFileInputStream::open
	 */
	CFileInputStream(const boost::filesystem::path & file, si64 start = 0, si64 size = 0);

	/**
	 * C-tor. Opens the specified file.
	 *
	 * @see CFileInputStream::open
	 */
	CFileInputStream(const CFileInfo & file, si64 start=0, si64 size=0);

	/**
	 * D-tor. Calls the close method implicitely, if the file is still opened.
	 */
	~CFileInputStream();

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	si64 read(ui8 * data, si64 size) override;

	/**
	 * Seeks the internal read pointer to the specified position.
	 *
	 * @param position The read position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	si64 seek(si64 position) override;

	/**
	 * Gets the current read position in the stream.
	 *
	 * @return the read position. -1 on failure or if the read pointer isn't in the valid range.
	 */
	si64 tell() override;

	/**
	 * Skips delta numbers of bytes.
	 *
	 * @param delta The count of bytes to skip.
	 * @return the count of bytes skipped actually.
	 */
	si64 skip(si64 delta) override;

	/**
	 * Gets the length in bytes of the stream.
	 *
	 * @return the length in bytes of the stream.
	 */
	si64 getSize() override;

private:
	/**
	 * Opens a file. If a file is currently opened, it will be closed.
	 *
	 * @param file Path to the file.
	 * @param start - offset from file start where real data starts (e.g file on archive)
	 * @param size - size of real data in file (e.g file on archive) or 0 to use whole file
	 *
	 * @throws std::runtime_error if file wasn't found
	 */
	void open(const boost::filesystem::path & file, si64 start, si64 size);

	si64 dataStart;
	si64 dataSize;

	/** Native c++ input file stream object. */
	boost::filesystem::ifstream fileStream;
};
