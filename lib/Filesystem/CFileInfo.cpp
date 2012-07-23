#include "StdInc.h"
#include "CFileInfo.h"

CFileInfo::CFileInfo() : name("")
{

}

CFileInfo::CFileInfo(const std::string & name)
	: name(name)
{

}

bool CFileInfo::exists() const
{
	return boost::filesystem::exists(name);
}

bool CFileInfo::isDirectory() const
{
	return boost::filesystem::is_directory(name);
}

void CFileInfo::setName(const std::string & name)
{
	this->name = name;
}

std::string CFileInfo::getName() const
{
	return name;
}

std::string CFileInfo::getPath() const
{
	size_t found = name.find_last_of("/\\");
	return name.substr(0, found);
}

std::string CFileInfo::getExtension() const
{
	// Get position of file extension dot
	size_t dotPos = name.find_last_of("/.");

	if(dotPos != std::string::npos && name[dotPos] == '.')
		return name.substr(dotPos);
	else
		return "";
}

std::string CFileInfo::getFilename() const
{
	size_t found = name.find_last_of("/\\");
	return name.substr(found + 1);
}

std::string CFileInfo::getStem() const
{
	std::string rslt = name;

	// Remove file extension
	size_t dotPos = name.find_last_of("/.");

	if(dotPos != std::string::npos && name[dotPos] == '.')
		rslt.erase(dotPos);

	// Remove path
	size_t found = rslt.find_last_of("/\\");
	return rslt.substr(found + 1);
}

EResType CFileInfo::getType() const
{
	return EResTypeHelper::getTypeFromExtension(getExtension());
}

std::time_t CFileInfo::getDate() const
{
	return boost::filesystem::last_write_time(name);
}

std::unique_ptr<std::list<CFileInfo> > CFileInfo::listFiles(size_t depth, const std::string & extensionFilter /*= ""*/) const
{
	std::unique_ptr<std::list<CFileInfo> > fileListPtr;

	if(exists() && isDirectory())
	{
		std::list<CFileInfo> * fileList = new std::list<CFileInfo>;

		boost::filesystem::recursive_directory_iterator enddir;
		for(boost::filesystem::recursive_directory_iterator it(name); it != enddir; ++it)
		{
			if (boost::filesystem::is_directory(it->status()))
			{
				it.no_push(depth >= it.level());
			}
			if(boost::filesystem::is_regular(it->status()))
			{
				if(extensionFilter == "" || it->path().extension() == extensionFilter)
				{
					CFileInfo file(it->path().string());
					fileList->push_back(file);
				}
			}
		}

		fileListPtr.reset(fileList);
	}

	return fileListPtr;
}
