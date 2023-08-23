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

VCMI_LIB_NAMESPACE_BEGIN

CFilesystemLoader::CFilesystemLoader(std::string _mountPoint, boost::filesystem::path baseDirectory, size_t depth, bool initial):
	baseDirectory(std::move(baseDirectory)),
	mountPoint(std::move(_mountPoint)),
	fileList(listFiles(mountPoint, depth, initial)),
	recursiveDepth(depth)
{
	logGlobal->trace("File system loaded, %d files found", fileList.size());
}

std::unique_ptr<CInputStream> CFilesystemLoader::load(const ResourcePath & resourceName) const
{
	assert(fileList.count(resourceName));
	boost::filesystem::path file = baseDirectory / fileList.at(resourceName);
	logGlobal->trace("loading %s", file.string());
	return std::make_unique<CFileInputStream>(file);
}

bool CFilesystemLoader::existsResource(const ResourcePath & resourceName) const
{
	return fileList.count(resourceName);
}

std::string CFilesystemLoader::getMountPoint() const
{
	return mountPoint;
}

std::optional<boost::filesystem::path> CFilesystemLoader::getResourceName(const ResourcePath & resourceName) const
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

std::unordered_set<ResourcePath> CFilesystemLoader::getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const
{
	std::unordered_set<ResourcePath> foundID;

	for (auto & file : fileList)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}

bool CFilesystemLoader::createResource(std::string filename, bool update)
{
	ResourcePath resID(filename);

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
		// create folders if not exists
		boost::filesystem::path p((baseDirectory / filename).c_str());
		boost::filesystem::create_directories(p.parent_path());

		// create file, if not exists
		std::ofstream file((baseDirectory / filename).c_str(), std::ofstream::binary);

		if (!file.is_open())
			return false;
	}
	fileList[resID] = filename;
	return true;
}

std::unordered_map<ResourcePath, boost::filesystem::path> CFilesystemLoader::listFiles(const std::string &mountPoint, size_t depth, bool initial) const
{
	static const EResType initArray[] = {
		EResType::DIRECTORY,
		EResType::TEXT,
		EResType::ARCHIVE_LOD,
		EResType::ARCHIVE_VID,
		EResType::ARCHIVE_SND,
		EResType::ARCHIVE_ZIP };
	static const std::set<EResType> initialTypes(initArray, initArray + std::size(initArray));

	assert(boost::filesystem::is_directory(baseDirectory));
	std::unordered_map<ResourcePath, boost::filesystem::path> fileList;

	std::vector<boost::filesystem::path> path; //vector holding relative path to our file

	boost::filesystem::recursive_directory_iterator enddir;
#if BOOST_VERSION >= 107200 // 1.72
	boost::filesystem::recursive_directory_iterator it(baseDirectory, boost::filesystem::directory_options::follow_directory_symlink);
#else
	boost::filesystem::recursive_directory_iterator it(baseDirectory, boost::filesystem::symlink_option::recurse);
#endif

	for(; it != enddir; ++it)
	{
		EResType type;
#if BOOST_VERSION >= 107200
		const auto currentDepth = it.depth();
#else
		const auto currentDepth = it.level();
#endif

		if (boost::filesystem::is_directory(it->status()))
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
			boost::filesystem::path filename;
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
			if (boost::filesystem::path::preferred_separator != '/')
			{
				// resource names are using UNIX slashes (/)
				resName.reserve(resName.size() + filename.native().size());
				resName = mountPoint;
				for (const char c : filename.string())
					if (c != boost::filesystem::path::preferred_separator)
						resName.push_back(c);
					else
						resName.push_back('/');
			}
			else
				resName = mountPoint + filename.string();

			fileList[ResourcePath(resName, type)] = std::move(filename);
		}
	}

	return fileList;
}

VCMI_LIB_NAMESPACE_END
