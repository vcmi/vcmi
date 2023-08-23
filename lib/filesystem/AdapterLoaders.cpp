/*
 * AdapterLoaders.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AdapterLoaders.h"

#include "../JsonNode.h"
#include "Filesystem.h"

VCMI_LIB_NAMESPACE_BEGIN

CMappedFileLoader::CMappedFileLoader(const std::string & mountPoint, const JsonNode &config)
{
	for(auto entry : config.Struct())
	{
		//fileList[ResourcePath(mountPoint + entry.first)] = ResourcePath(mountPoint + entry.second.String());
		fileList.emplace(ResourcePath(mountPoint + entry.first), ResourcePath(mountPoint + entry.second.String()));
	}
}

std::unique_ptr<CInputStream> CMappedFileLoader::load(const ResourcePath & resourceName) const
{
	return CResourceHandler::get()->load(fileList.at(resourceName));
}

bool CMappedFileLoader::existsResource(const ResourcePath & resourceName) const
{
	return fileList.count(resourceName) != 0;
}

std::string CMappedFileLoader::getMountPoint() const
{
	return ""; // does not have any meaning with this type of data source
}

std::optional<boost::filesystem::path> CMappedFileLoader::getResourceName(const ResourcePath & resourceName) const
{
	return CResourceHandler::get()->getResourceName(fileList.at(resourceName));
}

std::unordered_set<ResourcePath> CMappedFileLoader::getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const
{
	std::unordered_set<ResourcePath> foundID;

	for(const auto & file : fileList)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}

CFilesystemList::CFilesystemList()
{
}

CFilesystemList::~CFilesystemList()
{
}

std::unique_ptr<CInputStream> CFilesystemList::load(const ResourcePath & resourceName) const
{
	// load resource from last loader that have it (last overridden version)
	for(const auto & loader : boost::adaptors::reverse(loaders))
	{
		if (loader->existsResource(resourceName))
			return loader->load(resourceName);
	}

	throw std::runtime_error("Resource with name " + resourceName.getName() + " and type "
		+ EResTypeHelper::getEResTypeAsString(resourceName.getType()) + " wasn't found.");
}

bool CFilesystemList::existsResource(const ResourcePath & resourceName) const
{
	for(const auto & loader : loaders)
		if (loader->existsResource(resourceName))
			return true;
	return false;
}

std::string CFilesystemList::getMountPoint() const
{
	return "";
}

std::optional<boost::filesystem::path> CFilesystemList::getResourceName(const ResourcePath & resourceName) const
{
	if (existsResource(resourceName))
		return getResourcesWithName(resourceName).back()->getResourceName(resourceName);
	return std::optional<boost::filesystem::path>();
}

std::set<boost::filesystem::path> CFilesystemList::getResourceNames(const ResourcePath & resourceName) const
{
	std::set<boost::filesystem::path> paths;
	for(auto& loader : getResourcesWithName(resourceName))
	{
		auto rn = loader->getResourceName(resourceName);
		if(rn)
		{
			paths.insert(rn->string());
		}
	}
	return paths;
}

void CFilesystemList::updateFilteredFiles(std::function<bool(const std::string &)> filter) const
{
	for(const auto & loader : loaders)
		loader->updateFilteredFiles(filter);
}

std::unordered_set<ResourcePath> CFilesystemList::getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const
{
	std::unordered_set<ResourcePath> ret;

	for(const auto & loader : loaders)
		for(const auto & entry : loader->getFilteredFiles(filter))
			ret.insert(entry);

	return ret;
}

bool CFilesystemList::createResource(std::string filename, bool update)
{
	logGlobal->trace("Creating %s", filename);
	for (auto & loader : boost::adaptors::reverse(loaders))
	{
		if (writeableLoaders.count(loader.get()) != 0                       // writeable,
			&& loader->createResource(filename, update))          // successfully created
		{
			// Check if resource was created successfully. Possible reasons for this to fail
			// a) loader failed to create resource (e.g. read-only FS)
			// b) in update mode, call with filename that does not exists
			assert(load(ResourcePath(filename)));

			logGlobal->trace("Resource created successfully");
			return true;
		}
	}
	logGlobal->trace("Failed to create resource");
	return false;
}

std::vector<const ISimpleResourceLoader *> CFilesystemList::getResourcesWithName(const ResourcePath & resourceName) const
{
	std::vector<const ISimpleResourceLoader *> ret;

	for(const auto & loader : loaders)
		boost::range::copy(loader->getResourcesWithName(resourceName), std::back_inserter(ret));

	return ret;
}

void CFilesystemList::addLoader(ISimpleResourceLoader * loader, bool writeable)
{
	loaders.push_back(std::unique_ptr<ISimpleResourceLoader>(loader));
	if (writeable)
		writeableLoaders.insert(loader);
}

bool CFilesystemList::removeLoader(ISimpleResourceLoader * loader)
{
	for(auto loaderIterator = loaders.begin(); loaderIterator != loaders.end(); ++loaderIterator)
	{
		if(loaderIterator->get() == loader)
		{
			loaders.erase(loaderIterator);
			writeableLoaders.erase(loader);
			return true;
		}
	}
	return false;
}

VCMI_LIB_NAMESPACE_END
