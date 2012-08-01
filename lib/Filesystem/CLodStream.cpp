#include "StdInc.h"
#include "CLodStream.h"
/*
#include "CLodArchiveLoader.h"
#include "CFileInputStream.h"
#include "CMemoryStream.h"

CLodStream::CLodStream()
{

}

CLodStream::CLodStream(const CLodArchiveLoader * loader, const std::string & resourceName)
{
	open(loader, resourceName);
}

void CLodStream::open(const CLodArchiveLoader * loader, const std::string & resourceName)
{
	assert(!fileStream);

	const ArchiveEntry * archiveEntry = loader->getArchiveEntry(resourceName);
	if(archiveEntry == nullptr)
	{
		throw std::runtime_error("Archive entry " + resourceName + " wasn't found in the archive " + loader->getOrigin());
	}
	size = archiveEntry->size;
	offset = archiveEntry->offset;

	// Open the archive and set the read pointer to the correct position
	fileStream.reset(new CFileInputStream(loader->getOrigin()));
	fileStream->seek(archiveEntry->offset);

	// Decompress file
	if(archiveEntry->size != 0)
	{
		// replace original buffer with decompressed one
		fileStream.reset(CLodArchiveLoader::decompress(*this, archiveEntry->realSize));
		assert(fileStream->getSize() == archiveEntry->realSize);

		//in memory stream we no longer need offset
		offset = 0;
	}
	size = archiveEntry->realSize;
}

si64 CLodStream::read(ui8 * data, si64 size)
{
	return fileStream->read(data, size);
}

si64 CLodStream::seek(si64 position)
{
	return fileStream->seek(offset + position);
}

si64 CLodStream::tell()
{
	return fileStream->tell() - offset;
}

si64 CLodStream::skip(si64 delta)
{
	return fileStream->skip(delta);
}

si64 CLodStream::getSize()
{
	return size;
}

void CLodStream::close()
{
	fileStream->close();
}
*/