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

	return rslt;
}

std::string CFileInfo::getBaseName() const
{
	size_t end = name.find_last_of("/.");
	size_t begin = name.find_last_of("/");

	if(end != std::string::npos && name[end] == '.')
		end = std::string::npos;

	if(begin == std::string::npos)
		begin = 0;

	return name.substr(begin, end);
}


EResType::Type CFileInfo::getType() const
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

		std::vector<std::string> path;

		boost::filesystem::recursive_directory_iterator enddir;
		boost::filesystem::recursive_directory_iterator it(name, boost::filesystem::symlink_option::recurse);

		for(; it != enddir; ++it)
		{
			if (boost::filesystem::is_directory(it->status()))
			{
				path.resize(it.level()+1);
				path.back() = it->path().leaf().string();
				it.no_push(depth <= it.level());
			}

			if(extensionFilter.empty() || it->path().extension() == extensionFilter)
			{
				std::string filename;
				for (size_t i=0; i<it.level() && i<path.size(); i++)
					filename += path[i] + '/';
				filename += it->path().leaf().string();

				//tlog1 << "Found file: " << filename << "\n";
				CFileInfo file(filename);
				fileList->push_back(file);
			}
		}

		fileListPtr.reset(fileList);
	}

	return fileListPtr;
}
