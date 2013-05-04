#include "StdInc.h"
#include "CMappedFileLoader.h"
#include "CResourceLoader.h"
#include "../JsonNode.h"

CMappedFileLoader::CMappedFileLoader(const JsonNode &config)
{
	BOOST_FOREACH(auto entry, config.Struct())
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
	for(auto it = fileList.begin(); it != fileList.end(); ++it)
	{
		if(it->second == resourceName)
		{
			return true;
		}
	}

	return false;
}

boost::unordered_map<ResourceID, std::string> CMappedFileLoader::getEntries() const
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
