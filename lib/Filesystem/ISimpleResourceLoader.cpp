#include "StdInc.h"
#include "ISimpleResourceLoader.h"

#include <zlib.h>

std::pair<ui8*, size_t> ISimpleResourceLoader::decompressFile(ui8 * in, size_t size, size_t realSize)
{
	std::pair<ui8*, size_t> retError(nullptr, 0);

	if (realSize == 0)
		realSize = 16 * 1024;

	std::unique_ptr<ui8[]> out(new ui8[realSize]);

	const int WBITS = 15;
	const int FCHUNK = 50000;

	int ret;
	z_stream strm;
	int lastPosOut = 0;

	// Allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, WBITS);
	if (ret != Z_OK)
		return retError;

	int chunkNumber = 0;
	do
	{
		if(size < chunkNumber * FCHUNK)
			break;
		strm.avail_in = std::min<size_t>(FCHUNK, size - chunkNumber * FCHUNK);
		if (strm.avail_in == 0)
			break;
		strm.next_in = in + chunkNumber * FCHUNK;

		// Run inflate() on input until output buffer not full
		do
		{
			strm.avail_out = realSize - lastPosOut;
			strm.next_out = out.get() + lastPosOut;
			ret = inflate(&strm, Z_NO_FLUSH);

			bool breakLoop = false;
			switch (ret)
			{
			case Z_STREAM_END:
				breakLoop = true;
				break;
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
				inflateEnd(&strm);
				return retError;
			case Z_MEM_ERROR:
				{
					//not enough memory. Allocate bigger buffer and try again
					realSize *= 2;
					std::unique_ptr<ui8[]> newOut(new ui8[realSize]);
					std::copy(out.get(), out.get() + strm.total_out, newOut.get());
					out.reset(newOut.release());
				}
			}

			if(breakLoop)
				break;

			lastPosOut = realSize - strm.avail_out;
		}
		while (strm.avail_out == 0);

		++chunkNumber;
	}
	while (ret != Z_STREAM_END);

	// Clean up and return
	while (1)
	{
		ret = inflateEnd(&strm);

		switch (ret)
		{
		case Z_STREAM_END:
			//TODO: trim buffer? may be too time consuming
			return std::make_pair(out.release(), realSize - strm.avail_out);
		case Z_BUF_ERROR:
			{
				//not enough memory. Allocate bigger buffer and try again
				realSize *= 2;
				std::unique_ptr<ui8[]> newOut(new ui8[realSize]);
				std::copy(out.get(), out.get() + strm.total_out, newOut.get());
				out.reset(newOut.release());
			}
		default:
			return retError;
		}
	}
}