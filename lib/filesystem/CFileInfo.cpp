#include "StdInc.h"
#include "CFileInfo.h"

CFileInfo::CFileInfo() : name("")
{

}

CFileInfo::CFileInfo(std::string name)
	: name(std::move(name))
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
	size_t dotPos = name.find_last_of('.');

	if(dotPos != std::string::npos)
		return name.substr(dotPos);

	return "";
}

std::string CFileInfo::getFilename() const
{
	const size_t found = name.find_last_of("/\\");
	return name.substr(found + 1);
}

std::string CFileInfo::getStem() const
{
	std::string rslt = name;

	// Remove file extension
	const size_t dotPos = name.find_last_of('.');

	if(dotPos != std::string::npos)
		rslt.erase(dotPos);

	return rslt;
}

std::string CFileInfo::getBaseName() const
{
	size_t begin = name.find_last_of("/\\");
	size_t end = name.find_last_of(".");

	if(begin == std::string::npos)
		begin = 0;
	else
		++begin;
	
	if (end < begin)
		end = std::string::npos;

	size_t len = (end == std::string::npos ? std::string::npos : end - begin);
	return name.substr(begin, len);
}

EResType::Type CFileInfo::getType() const
{
	return EResTypeHelper::getTypeFromExtension(getExtension());
}

std::time_t CFileInfo::getDate() const
{
	return boost::filesystem::last_write_time(name);
}
