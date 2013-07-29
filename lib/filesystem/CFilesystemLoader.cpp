#include "StdInc.h"
#include "CFilesystemLoader.h"

#include "CFileInfo.h"
#include "CFileInputStream.h"

CFilesystemLoader::CFilesystemLoader(const std::string &mountPoint, const std::string & baseDirectory, size_t depth, bool initial):
    baseDirectory(baseDirectory),
    mountPoint(mountPoint),
    fileList(listFiles(mountPoint, depth, initial))
{
	logGlobal->traceStream() << "Filesystem loaded, " << fileList.size() << " files found";
}

std::unique_ptr<CInputStream> CFilesystemLoader::load(const ResourceID & resourceName) const
{
	assert(fileList.count(resourceName));

	std::unique_ptr<CInputStream> stream(new CFileInputStream(baseDirectory + '/' + fileList.at(resourceName)));
	return stream;
}

bool CFilesystemLoader::existsResource(const ResourceID & resourceName) const
{
	return fileList.count(resourceName);
}

std::string CFilesystemLoader::getMountPoint() const
{
	return mountPoint;
}

boost::optional<std::string> CFilesystemLoader::getResourceName(const ResourceID & resourceName) const
{
	assert(existsResource(resourceName));

	return baseDirectory + '/' + fileList.at(resourceName);
}

std::unordered_set<ResourceID> CFilesystemLoader::getFilteredFiles(std::function<bool(const ResourceID &)> filter) const
{
	std::unordered_set<ResourceID> foundID;

	for (auto & file : fileList)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}	return foundID;
}

bool CFilesystemLoader::createResource(std::string filename, bool update)
{
	ResourceID resID(filename);

	if (fileList.find(resID) != fileList.end())
		return true;

	if (!boost::iequals(mountPoint, filename.substr(0, mountPoint.size())))
	{
		logGlobal->traceStream() << "Can't create file: wrong mount point: " << mountPoint;
		return false;
	}

	filename = filename.substr(mountPoint.size());

	if (!update)
	{
		std::ofstream newfile (baseDirectory + "/" + filename);
		if (!newfile.good())
			return false;
	}
	fileList[resID] = filename;
	return true;
}

std::unordered_map<ResourceID, std::string> CFilesystemLoader::listFiles(const std::string &mountPoint, size_t depth, bool initial) const
{
	std::set<EResType::Type> initialTypes;
	initialTypes.insert(EResType::DIRECTORY);
	initialTypes.insert(EResType::TEXT);
	initialTypes.insert(EResType::ARCHIVE_LOD);
	initialTypes.insert(EResType::ARCHIVE_VID);
	initialTypes.insert(EResType::ARCHIVE_SND);

	assert(boost::filesystem::is_directory(baseDirectory));
	std::unordered_map<ResourceID, std::string> fileList;

	std::vector<std::string> path;//vector holding relative path to our file

	boost::filesystem::recursive_directory_iterator enddir;
	boost::filesystem::recursive_directory_iterator it(baseDirectory, boost::filesystem::symlink_option::recurse);

	for(; it != enddir; ++it)
	{
		EResType::Type type;

		if (boost::filesystem::is_directory(it->status()))
		{
			path.resize(it.level()+1);
			path.back() = it->path().leaf().string();
			// don't iterate into directory if depth limit reached
			it.no_push(depth <= it.level());

			type = EResType::DIRECTORY;
		}
		else
			type = EResTypeHelper::getTypeFromExtension(boost::filesystem::extension(*it));

		if (!initial || vstd::contains(initialTypes, type))
		{
			//reconstruct relative filename (not possible via boost AFAIK)
			std::string filename;
			for (size_t i=0; i<it.level() && i<path.size(); i++)
				filename += path[i] + '/';
			filename += it->path().leaf().string();

			fileList[ResourceID(mountPoint + filename, type)] = filename;
		}
	}

	return fileList;
}
