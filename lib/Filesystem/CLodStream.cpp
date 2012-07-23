#include "StdInc.h"
#include "CLodStream.h"

#include "CLodArchiveLoader.h"
#include "CFileInputStream.h"

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
		auto comp = std::unique_ptr<ui8[]>(new ui8[archiveEntry->size]);
		fileStream.read(comp.get(), archiveEntry->size);

		// Decompress the file
		data = CLodArchiveLoader::decompressFile(comp.get(), archiveEntry->size, archiveEntry->realSize).first;

		if (!data)
		{
			throw std::runtime_error("File decompression wasn't successful. Resource name: " + archiveEntry->name);
		}

		// We're reading the total size always
		return archiveEntry->realSize;
	}
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
