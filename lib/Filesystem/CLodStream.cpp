#include "StdInc.h"
#include "CLodStream.h"

#include "CLodArchiveLoader.h"
#include "CFileInputStream.h"
#include <zlib.h>

CLodStream::CLodStream()
{

}

CLodStream::CLodStream(const CLodArchiveLoader * loader, const std::string & resourceName)
{
	open(loader, resourceName);
}

void CLodStream::open(const CLodArchiveLoader * loader, const std::string & resourceName)
{
	close();

	const ArchiveEntry * archiveEntry = loader->getArchiveEntry(resourceName);
	if(archiveEntry == nullptr)
	{
		throw std::runtime_error("Archive entry " + resourceName + " wasn't found in the archive " + loader->getOrigin());
	}
	this->archiveEntry = archiveEntry;

	// Open the archive and set the read pointer to the correct position
	fileStream.open(loader->getOrigin());
	fileStream.seek(this->archiveEntry->offset);
}

si64 CLodStream::read(ui8 * data, si64 size)
{
	// Test whether the file has to be decompressed
	if(archiveEntry->size == 0)
	{
		// No decompression
		return fileStream.read(data, size);
	}
	else
	{
		// Decompress file

		// We can't decompress partially, so the size of the output buffer has to be minimum the
		// size of the decompressed lod entry. If this isn't the case, it is a programming fault.
		assert(size >= archiveEntry->realSize);

		// Read the compressed data into a buffer
		ui8 * comp = new ui8[archiveEntry->size];
		fileStream.read(comp, archiveEntry->size);

		// Decompress the file
		if(!decompressFile(comp, archiveEntry->size, archiveEntry->realSize, data))
		{
			throw std::runtime_error("File decompression wasn't successful. Resource name: " + archiveEntry->name);
		}
		delete[] comp;

		// We're reading the total size always
		return archiveEntry->realSize;
	}
}

bool CLodStream::decompressFile(ui8 * in, int size, int realSize, ui8 * out)
{
	const int WBITS = 15;
	const int FCHUNK = 50000;

	int ret;
	unsigned have;
	z_stream strm;
	int latPosOut = 0;

	// Allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, WBITS);
	if (ret != Z_OK)
		return false;

	int chunkNumber = 0;
	do
	{
		if(size < chunkNumber * FCHUNK)
			break;
		strm.avail_in = std::min(FCHUNK, size - chunkNumber * FCHUNK);
		if (strm.avail_in == 0)
			break;
		strm.next_in = in + chunkNumber * FCHUNK;

		// Run inflate() on input until output buffer not full
		do
		{
			strm.avail_out = realSize - latPosOut;
			strm.next_out = out + latPosOut;
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
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return false;
			}

			if(breakLoop)
				break;

			have = realSize - latPosOut - strm.avail_out;
			latPosOut += have;
		} while (strm.avail_out == 0);

		++chunkNumber;
	} while (ret != Z_STREAM_END);

	// Clean up and return
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? true : false;
}

si64 CLodStream::seek(si64 position)
{
	return fileStream.seek(archiveEntry->offset + position);
}

si64 CLodStream::tell()
{
	return fileStream.tell() - archiveEntry->offset;
}

si64 CLodStream::skip(si64 delta)
{
	return fileStream.skip(delta);
}

si64 CLodStream::getSize()
{
	return archiveEntry->realSize;
}

void CLodStream::close()
{
	fileStream.close();
}
