#include "StdInc.h"
#include "CFileInputStream.h"

#include "CFileInfo.h"

CFileInputStream::CFileInputStream(const boost::filesystem::path & file, si64 start, si64 size)
  : dataStart{start},
	dataSize{size},
	fileStream{file, std::ios::in | std::ios::binary}
{
	if (fileStream.fail())
		throw std::runtime_error("File " + file.string() + " isn't available.");

	if (dataSize == 0)
	{
		fileStream.seekg(0, std::ios::end);
		dataSize = tell();
	}

	fileStream.seekg(dataStart, std::ios::beg);
}

CFileInputStream::CFileInputStream(const CFileInfo & file, si64 start, si64 size)
	: CFileInputStream{file.getName(), start, size} {}

si64 CFileInputStream::read(ui8 * data, si64 size)
{
	si64 origin = tell();
	si64 toRead = std::min(dataSize - origin, size);
	fileStream.read(reinterpret_cast<char *>(data), toRead);
	return fileStream.gcount();
}

si64 CFileInputStream::seek(si64 position)
{
	fileStream.seekg(dataStart + std::min(position, dataSize));
	return tell();
}

si64 CFileInputStream::tell()
{
	return static_cast<si64>(fileStream.tellg()) - dataStart;
}

si64 CFileInputStream::skip(si64 delta)
{
	si64 origin = tell();
	//ensure that we're not seeking past the end of real data
	si64 toSeek = std::min(dataSize - origin, delta);
	fileStream.seekg(toSeek, std::ios::cur);

	return tell() - origin;
}

si64 CFileInputStream::getSize()
{
	return dataSize;
}
