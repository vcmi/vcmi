#include "StdInc.h"
#include "CFilesystemLoader.h"

#include "CFileInfo.h"
#include "CFileInputStream.h"
#include "FileStream.h"

namespace bfs = boost::filesystem;

CFilesystemLoader::CFilesystemLoader(std::string _mountPoint, bfs::path baseDirectory, size_t depth, bool initial):
    baseDirectory(std::move(baseDirectory)),
	mountPoint(std::move(_mountPoint)),
    fileList(listFiles(mountPoint, depth, initial))
{
	logGlobal->traceStream() << "Filesystem loaded, " << fileList.size() << " files found";
}

std::unique_ptr<CInputStream> CFilesystemLoader::load(const ResourceID & resourceName) const
{
	assert(fileList.count(resourceName));

	std::unique_ptr<CInputStream> stream(new CFileInputStream(baseDirectory / fileList.at(resourceName)));
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

boost::optional<boost::filesystem::path> CFilesystemLoader::getResourceName(const ResourceID & resourceName) const
{
	assert(existsResource(resourceName));

	return baseDirectory / fileList.at(resourceName);
}

std::unordered_set<ResourceID> CFilesystemLoader::getFilteredFiles(std::function<bool(const ResourceID &)> filter) const
{
	std::unordered_set<ResourceID> foundID;

	for (auto & file : fileList)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
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
		if (!FileStream::CreateFile(baseDirectory / filename))
			return false;
	}
	fileList[resID] = filename;
	return true;
}

std::unordered_map<ResourceID, bfs::path> CFilesystemLoader::listFiles(const std::string &mountPoint, size_t depth, bool initial) const
{
	static const EResType::Type initArray[] = {
		EResType::DIRECTORY,
		EResType::TEXT,
		EResType::ARCHIVE_LOD,
		EResType::ARCHIVE_VID,
		EResType::ARCHIVE_SND,
		EResType::ARCHIVE_ZIP };
	static const std::set<EResType::Type> initialTypes(initArray, initArray + ARRAY_COUNT(initArray));

	assert(bfs::is_directory(baseDirectory));
	std::unordered_map<ResourceID, bfs::path> fileList;

	std::vector<bfs::path> path; //vector holding relative path to our file

	bfs::recursive_directory_iterator enddir;
	bfs::recursive_directory_iterator it(baseDirectory, bfs::symlink_option::recurse);

	for(; it != enddir; ++it)
	{
		EResType::Type type;

		if (bfs::is_directory(it->status()))
		{
			path.resize(it.level() + 1);
			path.back() = it->path().filename();
			// don't iterate into directory if depth limit reached
			it.no_push(depth <= it.level());

			type = EResType::DIRECTORY;
		}
		else
			type = EResTypeHelper::getTypeFromExtension(it->path().extension().string());

		if (!initial || vstd::contains(initialTypes, type))
		{
			//reconstruct relative filename (not possible via boost AFAIK)
			bfs::path filename;
			const size_t iterations = std::min((size_t)it.level(), path.size());
			if (iterations)
			{
				filename = path.front();
				for (size_t i = 1; i < iterations; ++i)
					filename /= path[i];
				filename /= it->path().filename();
			}
			else
				filename = it->path().filename();

			std::string resName;
			if (bfs::path::preferred_separator != '/')
			{
				// resource names are using UNIX slashes (/)
				resName.reserve(resName.size() + filename.native().size());
				resName = mountPoint;
				for (const char c : filename.string())
					if (c != bfs::path::preferred_separator)
						resName.push_back(c);
					else
						resName.push_back('/');
			}
			else
				resName = mountPoint + filename.string();

			fileList[ResourceID(resName, type)] = std::move(filename);
		}
	}

	return fileList;
}
