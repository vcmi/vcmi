/*
 * CCompressedStream.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCompressedStream.h"

#include <zlib.h>

VCMI_LIB_NAMESPACE_BEGIN

static const int inflateBlockSize = 10000;

CBufferedStream::CBufferedStream():
    position(0),
    endOfFileReached(false)
{
}

si64 CBufferedStream::read(ui8 * data, si64 size)
{
	ensureSize(position + size);

	auto start = buffer.data() + position;
	si64 toRead = std::min<si64>(size, buffer.size() - position);

	std::copy(start, start + toRead, data);
	position += toRead;
	return size;
}

si64 CBufferedStream::seek(si64 position)
{
	ensureSize(position);
	this->position = std::min<si64>(position, buffer.size());
	return this->position;
}

si64 CBufferedStream::tell()
{
	return position;
}

si64 CBufferedStream::skip(si64 delta)
{
	return seek(position + delta) - delta;
}

si64 CBufferedStream::getSize()
{
	si64 prevPos = tell();
	seek(std::numeric_limits<si64>::max());
	si64 size = tell();
	seek(prevPos);
	return size;
}

void CBufferedStream::ensureSize(si64 size)
{
	while ((si64)buffer.size() < size && !endOfFileReached)
	{
		si64 initialSize = buffer.size();
		si64 currentStep = std::min<si64>(size, buffer.size());
		vstd::amax(currentStep, 1024); // to avoid large number of calls at start

		buffer.resize(initialSize + currentStep);

		si64 readSize = readMore(buffer.data() + initialSize, currentStep);
		if (readSize != currentStep)
		{
			endOfFileReached = true;
			buffer.resize(initialSize + readSize);
			buffer.shrink_to_fit();
			return;
		}
	}
}

void CBufferedStream::reset()
{
	buffer.clear();
	position = 0;
	endOfFileReached = false;
}

CCompressedStream::CCompressedStream(std::unique_ptr<CInputStream> stream, bool gzip, size_t decompressedSize):
	gzipStream(std::move(stream)),
	compressedBuffer(inflateBlockSize)
{
	assert(gzipStream);

	// Allocate inflate state
	inflateState = new z_stream();
	inflateState->zalloc = Z_NULL;
	inflateState->zfree = Z_NULL;
	inflateState->opaque = Z_NULL;
	inflateState->avail_in = 0;
	inflateState->next_in = Z_NULL;

	int wbits = 15;
	if (gzip)
		wbits += 16;

	int ret = inflateInit2(inflateState, wbits);
	if (ret != Z_OK)
		throw std::runtime_error("Failed to initialize inflate!\n");
}

CCompressedStream::~CCompressedStream()
{
	inflateEnd(inflateState);
	vstd::clear_pointer(inflateState);
}

si64 CCompressedStream::readMore(ui8 *data, si64 size)
{
	if (inflateState == nullptr)
		return 0; //file already decompressed

	bool fileEnded = false; //end of file reached
	bool endLoop = false;

	int decompressed = inflateState->total_out;

	inflateState->avail_out = (uInt)size;
	inflateState->next_out = data;

	do
	{
		if (inflateState->avail_in == 0)
		{
			//inflate ran out of available data or was not initialized yet
			// get new input data and update state accordingly
			si64 availSize = gzipStream->read(compressedBuffer.data(), compressedBuffer.size());
			if (availSize != compressedBuffer.size())
				gzipStream.reset();

			inflateState->avail_in = (uInt)availSize;
			inflateState->next_in  = compressedBuffer.data();
		}

		int ret = inflate(inflateState, Z_NO_FLUSH);

		if (inflateState->avail_in == 0 && gzipStream == nullptr)
			fileEnded = true;

		switch (ret)
		{
		case Z_OK: //input data ended/ output buffer full
			endLoop = false;
			break;
		case Z_STREAM_END: // stream ended. Note that campaign files consist from multiple such streams
			endLoop = true;
			break;
		case Z_BUF_ERROR: // output buffer full. Can be triggered?
			endLoop = true;
			break;
		default:
			if (inflateState->msg == nullptr)
				throw std::runtime_error("Decompression error. Return code was " + std::to_string(ret));
			else
				throw std::runtime_error(std::string("Decompression error: ") + inflateState->msg);
		}
	}
	while (!endLoop && inflateState->avail_out != 0 );

	decompressed = inflateState->total_out - decompressed;

	// Clean up and return
	if (fileEnded)
	{
		inflateEnd(inflateState);
		vstd::clear_pointer(inflateState);
	}
	return decompressed;
}

bool CCompressedStream::getNextBlock()
{
	if (!inflateState)
		return false;

	if (inflateReset(inflateState) < 0)
		return false;

	reset();
	return true;
}

VCMI_LIB_NAMESPACE_END
