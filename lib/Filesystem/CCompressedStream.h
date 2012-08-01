
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

struct z_stream_s;

/**
 * A class which provides method definitions for reading a gzip-compressed file
 * This class implements lazy loading - data will be decompressed (and cached by this class) only by request
 */
class DLL_LINKAGE CCompressedStream : public CInputStream
{
public:
	/**
	 * C-tor.
	 *
	 * @param stream - stream with compresed data
	 * @param gzip - this is gzipp'ed file e.g. campaign or maps, false for files in lod
	 * @param decompressedSize - optional parameter to hint size of decompressed data
	 */
	CCompressedStream(std::unique_ptr<CInputStream> & stream, bool gzip, size_t decompressedSize=0);

	~CCompressedStream();

	/**
	 * Reads n bytes from the stream into the data buffer.
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
	 * This will cause decompressing data till this position is found
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
	 * Causes complete data decompression
	 *
	 * @return the length in bytes of the stream.
	 */
	si64 getSize();

private:
	/**
	 * Decompresses data to ensure that buffer has newSize bytes or end of stream was reached
	 */
	void decompressTill(si64 newSize);

	/** The file stream with compressed data. */
	std::unique_ptr<CInputStream> gzipStream;

	/** buffer with already decompressed data. Please note that size() != decompressed size */
	std::vector<ui8> buffer;

	/** size of already decompressed data */
	si64 decompressedSize;

	/** buffer with not yet decompressed data*/
	std::vector<ui8> compressedBuffer;

	/** struct with current zlib inflate state */
	z_stream_s * inflateState;

	/** Current read position */
	si64 position;
};
