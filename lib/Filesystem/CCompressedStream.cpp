#include "StdInc.h"
#include "CCompressedStream.h"

#include <zlib.h>

static const int inflateBlockSize = 10000;

CCompressedStream::CCompressedStream(std::unique_ptr<CInputStream> stream, bool gzip, size_t decompressedSize):
    gzipStream(std::move(stream)),
    buffer(decompressedSize),
    decompressedSize(0),
    compressedBuffer(inflateBlockSize),
    position(0)
{
	assert(gzipStream);

	// Allocate inflate state
	inflateState = new z_stream;
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
	if (inflateState)
	{
		inflateEnd(inflateState);
		delete inflateState;
	}
}

si64 CCompressedStream::read(ui8 * data, si64 size)
{
	decompressTill(position + size);

	auto start = buffer.begin() + position;
	si64 toRead = std::min<si64>(size, decompressedSize - position);

	std::copy(start, start + toRead, data);
	position += toRead;
	return size;
}

si64 CCompressedStream::seek(si64 position)
{
	decompressTill(position);
	this->position = std::min<si64>(position, decompressedSize);
	return this->position;
}

si64 CCompressedStream::tell()
{
	return position;
}

si64 CCompressedStream::skip(si64 delta)
{
	decompressTill(position + delta);

	si64 oldPosition = position;
	position = std::min<si64>(position + delta , decompressedSize);
	return position - oldPosition;
}

si64 CCompressedStream::getSize()
{
	decompressTill(-1);
	return decompressedSize;
}

void CCompressedStream::decompressTill(si64 newSize)
{
	assert(newSize < 100 * 1024 * 1024); //just in case

	if (inflateState == nullptr)
		return; //file already decompressed

	if (newSize >= 0 && newSize <= inflateState->total_out)
		return; //no need to decompress anything

	bool toEnd = newSize < 0; //if true - we've got request to read whole file

	if (toEnd && buffer.empty())
		buffer.resize(16 * 1024); //some space for initial decompression

	if (!toEnd && buffer.size() < newSize)
		buffer.resize(newSize);

	bool fileEnded = false; //end of file reached

	int endLoop = false;
	do
	{
		if (inflateState->avail_in == 0)
		{
			//inflate ran out of available data or was not initialized yet
			// get new input data and update state accordingly
			si64 size = gzipStream->read(compressedBuffer.data(), compressedBuffer.size());
			if (size != compressedBuffer.size())
				fileEnded = true; //end of file reached

			inflateState->avail_in = size;
			inflateState->next_in  = compressedBuffer.data();
		}

		inflateState->avail_out = buffer.size() - inflateState->total_out;
		inflateState->next_out = buffer.data() + inflateState->total_out;

		int ret = inflate(inflateState, Z_NO_FLUSH);

		switch (ret)
		{
		case Z_STREAM_END: //end decompression
			endLoop = true;
			break;
		case Z_OK: //decompress next chunk
			endLoop = false;
			break;
		case Z_BUF_ERROR:
			{
				if (toEnd)
				{
					//not enough memory. Allocate bigger buffer and try again
					buffer.resize(buffer.size() * 2);
					endLoop = false;
				}
				else
				{
					//specific amount of data was requested. inflate extracted all requested data
					// but returned "error". Ensure that enough data was extracted and return
					assert(inflateState->total_out == newSize);
					endLoop = true;
				}
			}
			break;
		default:
			throw std::runtime_error("Decompression error!\n");
		}

	}
	while (!endLoop);

	// Clean up and return
	if (fileEnded)
	{
		inflateEnd(inflateState);
		buffer.resize(inflateState->total_out);
		decompressedSize = inflateState->total_out;
		vstd::clear_pointer(inflateState);
	}
	else
		decompressedSize = inflateState->total_out;
}

bool CCompressedStream::getNextBlock()
{
	if (!inflateState)
		return false;

	if (inflateReset(inflateState) < 0)
		return false;

	decompressedSize = 0;
	position = 0;
	buffer.clear();
	return true;
}
