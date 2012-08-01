#include "StdInc.h"
#include "ISimpleResourceLoader.h"
#include "CMemoryStream.h"

#include <zlib.h>

std::pair<ui8*, size_t> ISimpleResourceLoader::decompress(ui8 * in, size_t size, size_t realSize)
{
	std::pair<ui8*, size_t> retError(nullptr, 0);

	//realSize was not set. Use compressed size as basisto gen something usable
	if (realSize == 0)
		realSize = size * 2;

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
			//TODO: check if  inflate have any special mode for inflating (e.g Z_BLOCK)
			//campaigns which consist from several blocks (campaign header + every map)
			//technically .h3c consists from multiple cancatenated gz streams
			//inflatings them separately can be useful for campaigns loading
			strm.avail_out = realSize - lastPosOut;
			strm.next_out = out.get() + lastPosOut;
			ret = inflate(&strm, Z_NO_FLUSH);

			bool breakLoop = false;
			switch (ret)
			{
			case Z_STREAM_END: //end decompression
				breakLoop = true;
			case Z_OK: //decompress next chunk
				break;
			case Z_MEM_ERROR:
			case Z_BUF_ERROR:
				{
					//not enough memory. Allocate bigger buffer and try again
					realSize *= 2;
					std::unique_ptr<ui8[]> newOut(new ui8[realSize]);
					std::copy(out.get(), out.get() + strm.total_out, newOut.get());
					out.reset(newOut.release());
				}
			default:
				ret = Z_DATA_ERROR;
				inflateEnd(&strm);
				return retError;

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

		//FIXMWE: duplicates previous  switch block
		switch (ret)
		{
		case Z_STREAM_END:
		case Z_OK:
			//TODO: trim buffer? may be too time consuming
			return std::make_pair(out.release(), realSize - strm.avail_out);
		case Z_MEM_ERROR:
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