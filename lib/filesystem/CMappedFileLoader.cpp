#include "StdInc.h"
#include "CMappedFileLoader.h"
#include "CResourceLoader.h"
#include "../JsonNode.h"

CMappedFileLoader::CMappedFileLoader(const JsonNode &config)
{
	for(auto entry : config.Struct())
	{
		fileList[ResourceID(entry.first)] = entry.second.String();
	}
}

std::unique_ptr<CInputStream> CMappedFileLoader::load(const std::string & resourceName) const
{
	return CResourceHandler::get()->load(ResourceID(resourceName));
}

bool CMappedFileLoader::existsEntry(const std::string & resourceName) const
{
	for(auto & elem : fileList)
	{
		if(elem.second == resourceName)
		{
			return true;
		}
	}

	return false;
}

std::unordered_map<ResourceID, std::string> CMappedFileLoader::getEntries() const
{
	return fileList;
}

std::string CMappedFileLoader::getOrigin() const
{
	return ""; // does not have any meaning with this type of data source
}

std::string CMappedFileLoader::getFullName(const std::string & resourceName) const
{
	return CResourceHandler::get()->getResourceName(ResourceID(resourceName));
}
