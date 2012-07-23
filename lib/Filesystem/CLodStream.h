
/*
 * CLodStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CInputStream.h"
#include "CFileInputStream.h"
#include "CLodArchiveLoader.h"

/**
 * A class which provides method definitions for reading a file from a LOD archive.
 */
class DLL_LINKAGE CLodStream : public CInputStream
{
public:
	/**
	 * Default c-tor.
	 */
	CLodStream();

	/**
	 * C-tor.
	 *
	 * @param loader The archive loader object which knows about the structure of the archive format.
	 * @param resourceName The resource name which says what resource should be loaded.
	 *
	 * @throws std::runtime_error if the archive entry wasn't found
	 */
	CLodStream(const CLodArchiveLoader * loader, const std::string & resourceName);

	/**
	 * Opens a lod stream. It will close any currently opened file stream.
	 *
	 * @param loader The archive loader object which knows about the structure of the archive format.
	 * @param resourceName The resource name which says what resource should be loaded.
	 *
	 * @throws std::runtime_error if the archive entry wasn't found
	 */
	void open(const CLodArchiveLoader * loader, const std::string & resourceName);

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * Warning: You can't read just a part of the archive entry if it's a decompressed one. So reading the total size always is
	 * recommended unless you really know what you're doing e.g. reading from a video stream.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 *
	 * @throws std::runtime_error if the file decompression was not successful
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
	 * @return the read position.
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

	/** The file stream for reading from the LOD archive. */
	CFileInputStream fileStream;

	/** The archive entry which specifies the length, offset,... of the entry. */
	const ArchiveEntry * archiveEntry;
};
