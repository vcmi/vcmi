#include "StdInc.h"
#include "CFileInputStream.h"
#include "CFileInfo.h"

CFileInputStream::CFileInputStream()
{

}

CFileInputStream::CFileInputStream(const std::string & file)
{
	open(file);
}

CFileInputStream::CFileInputStream(const CFileInfo & file)
{
	open(file);
}

CFileInputStream::~CFileInputStream()
{
	close();
}

void CFileInputStream::open(const std::string & file)
{
	close();

	fileStream.open(file.c_str(), std::ios::in | std::ios::binary);
	if (fileStream.fail())
	{
		throw std::runtime_error("File " + file + " isn't available.");
	}
}

void CFileInputStream::open(const CFileInfo & file)
{
	open(file.getName());
}

si64 CFileInputStream::read(ui8 * data, si64 size)
{
	fileStream.read(reinterpret_cast<char *>(data), size);
	return fileStream.gcount();
}

si64 CFileInputStream::seek(si64 position)
{
	si64 diff = tell();
	fileStream.seekg(position);

	return tell() - diff;
}

si64 CFileInputStream::tell()
{
	return fileStream.tellg();
}

si64 CFileInputStream::skip(si64 delta)
{
	si64 diff = tell();
	fileStream.seekg(delta, std::ios::cur);

	return tell() - diff;
}

si64 CFileInputStream::getSize()
{
	fileStream.seekg(0, std::ios::end);
	return fileStream.tellg();
}

void CFileInputStream::close()
{
	if (fileStream.is_open())
	{
		fileStream.close();
	}
}
