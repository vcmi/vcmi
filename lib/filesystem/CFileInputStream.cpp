#include "StdInc.h"
#include "CFileInputStream.h"

#include "CFileInfo.h"

CFileInputStream::CFileInputStream(const std::string & file, si64 start, si64 size)
{
	open(file, start, size);
}

CFileInputStream::CFileInputStream(const CFileInfo & file, si64 start, si64 size)
{
	open(file.getName(), start, size);
}

CFileInputStream::~CFileInputStream()
{
	fileStream.close();
}

void CFileInputStream::open(const std::string & file, si64 start, si64 size)
{
	fileStream.open(file.c_str(), std::ios::in | std::ios::binary);

	if (fileStream.fail())
	{
		throw std::runtime_error("File " + file + " isn't available.");
	}

	dataStart = start;
	dataSize = size;

	if (dataSize == 0)
	{
		fileStream.seekg(0, std::ios::end);
		dataSize = tell();
	}

	fileStream.seekg(start, std::ios::beg);
}

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
	return fileStream.tellg() - dataStart;
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
