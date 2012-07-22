#include "StdInc.h"
#include "CFilesystemLoader.h"

#include "CFileInfo.h"
#include "CFileInputStream.h"

CFilesystemLoader::CFilesystemLoader()
{

}

CFilesystemLoader::CFilesystemLoader(const std::string & baseDirectory)
{
	open(baseDirectory);
}

CFilesystemLoader::CFilesystemLoader(const CFileInfo & baseDirectory)
{
	open(baseDirectory);
}

void CFilesystemLoader::open(const std::string & baseDirectory)
{
	// Indexes all files in the directory and store them
	this->baseDirectory = baseDirectory;
	CFileInfo directory(baseDirectory);
	std::unique_ptr<std::list<CFileInfo> > fileList = directory.listFiles();
	if(fileList == nullptr)
	{
		throw std::runtime_error("Directory " + baseDirectory + " not available.");
	}

	this->fileList = std::move(*fileList);
}

void CFilesystemLoader::open(const CFileInfo & baseDirectory)
{
	open(baseDirectory.getName());
}

std::unique_ptr<CInputStream> CFilesystemLoader::load(const std::string & resourceName) const
{
	std::unique_ptr<CInputStream> stream(new CFileInputStream(resourceName));
	return stream;
}

bool CFilesystemLoader::existsEntry(const std::string & resourceName) const
{
	for(auto it = fileList.begin(); it != fileList.end(); ++it)
	{
		if((*it).getName() == resourceName)
		{
			return true;
		}
	}

	return false;
}

std::list<std::string> CFilesystemLoader::getEntries() const
{
	std::list<std::string> retList;

	for(auto it = fileList.begin(); it != fileList.end(); ++it)
	{
		retList.push_back(it->getName());
	}

	return std::move(retList);
}
