/*
 * CFilesystemLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CFilesystemLoader.h"

#include "CFileInputStream.h"
#include "FileStream.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace bfs = boost::filesystem;

CFilesystemLoader::CFilesystemLoader(std::string _mountPoint, bfs::path baseDirectory, size_t depth, bool initial):
	baseDirectory(std::move(baseDirectory)),
	mountPoint(std::move(_mountPoint)),
	fileList(listFiles(mountPoint, depth, initial)),
	recursiveDepth(depth)
{
	logGlobal->trace("File system loaded, %d files found", fileList.size());
}

std::unique_ptr<CInputStream> CFilesystemLoader::load(const ResourceID & resourceName) const
{
	assert(fileList.count(resourceName));
	bfs::path file = baseDirectory / fileList.at(resourceName);
	logGlobal->trace("loading %s", file.string());
	return std::make_unique<CFileInputStream>(file);
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

void CFilesystemLoader::updateFilteredFiles(std::function<bool(const std::string &)> filter) const
{
	if (filter(mountPoint))
	{
		fileList = listFiles(mountPoint, recursiveDepth, false);
	}
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
		logGlobal->trace("Can't create file: wrong mount point: %s", mountPoint);
		return false;
	}

	filename = filename.substr(mountPoint.size());

	if (!update)
	{
		if (!FileStream::createFile(baseDirectory / filename))
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
#if BOOST_VERSION >= 107200 // 1.72
	bfs::recursive_directory_iterator it(baseDirectory, bfs::directory_options::follow_directory_symlink);
#else
	bfs::recursive_directory_iterator it(baseDirectory, bfs::symlink_option::recurse);
#endif

	for(; it != enddir; ++it)
	{
		EResType::Type type;
#if BOOST_VERSION >= 107200
		const auto currentDepth = it.depth();
#else
		const auto currentDepth = it.level();
#endif

		if (bfs::is_directory(it->status()))
		{
			path.resize(currentDepth + 1);
			path.back() = it->path().filename();
			// don't iterate into directory if depth limit reached
#if BOOST_VERSION >= 107200
			it.disable_recursion_pending(depth <= currentDepth);
#else
			it.no_push(depth <= currentDepth);
#endif

			type = EResType::DIRECTORY;
		}
		else
			type = EResTypeHelper::getTypeFromExtension(it->path().extension().string());

		if (!initial || vstd::contains(initialTypes, type))
		{
			//reconstruct relative filename (not possible via boost AFAIK)
			bfs::path filename;
			const size_t iterations = std::min(static_cast<size_t>(currentDepth), path.size());
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

VCMI_LIB_NAMESPACE_END
