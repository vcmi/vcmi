/*
 * CCompressedStream.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

/// Abstract class that provides buffer for one-directional input streams (e.g. compressed data)
/// Used for zip archives support and in .lod deflate compression
class CBufferedStream : public CInputStream
{
public:
	CBufferedStream();

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 *
	 * @throws std::runtime_error if the file decompression was not successful
	 */
	si64 read(ui8 * data, si64 size) override;

	/**
	 * Seeks the internal read pointer to the specified position.
	 * This will cause decompressing data till this position is found
	 *
	 * @param position The read position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	si64 seek(si64 position) override;

	/**
	 * Gets the current read position in the stream.
	 *
	 * @return the read position.
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
	 * Causes complete data decompression
	 *
	 * @return the length in bytes of the stream.
	 */
	si64 getSize() override;

protected:
	/**
	 * @brief virtual method to get more data into the buffer
	 * @return amount of bytes actually read, non-positive values are considered to be end-of-file mark
	 */
	virtual si64 readMore(ui8 * data, si64 size) = 0;

	/// resets all internal state
	void reset();
private:
	/// ensures that buffer contains at lest size of bytes. Calls readMore() to fill remaining part
	void ensureSize(si64 size);

	/** buffer with already decompressed data */
	std::vector<ui8> buffer;

	/** Current read position */
	si64 position;

	bool endOfFileReached;
};

/**
 * A class which provides method definitions for reading a gzip-compressed file
 * This class implements lazy loading - data will be decompressed (and cached) only by request
 */
class DLL_LINKAGE CCompressedStream : public CBufferedStream
{
public:
	/**
	 * C-tor.
	 *
	 * @param stream - stream with compresed data
	 * @param gzip - this is gzipp'ed file e.g. campaign or maps, false for files in lod
	 * @param decompressedSize - optional parameter to hint size of decompressed data
	 */
	CCompressedStream(std::unique_ptr<CInputStream> stream, bool gzip, size_t decompressedSize=0);

	~CCompressedStream();

	/**
	 * Prepare stream for decompression of next block (e.g. next part of h3c)
	 * Applicable only for streams that contain multiple concatenated compressed data
	 *
	 * @return false if next block was not found, true othervice
	 */
	bool getNextBlock();

private:
	/**
	 * Decompresses data to ensure that buffer has newSize bytes or end of stream was reached
	 */
	si64 readMore(ui8 * data, si64 size) override;

	/** The file stream with compressed data. */
	std::unique_ptr<CInputStream> gzipStream;

	/** buffer with not yet decompressed data*/
	std::vector<ui8> compressedBuffer;

	/** struct with current zlib inflate state */
	z_stream_s * inflateState;

	enum EState
	{
		ERROR_OCCURED,
		INITIALIZED,
		IN_PROGRESS,
		STREAM_END,
		FINISHED
	};
};

VCMI_LIB_NAMESPACE_END
