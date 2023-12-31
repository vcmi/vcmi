/*
 * ISimpleResourceLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CInputStream;
class ResourcePath;

/**
 * A class which knows the files containing in the archive or system and how to load them.
 */
class DLL_LINKAGE ISimpleResourceLoader
{
public:
	virtual ~ISimpleResourceLoader() { };

	/**
	 * Loads a resource with the given resource name.
	 *
	 * @param resourceName The unqiue resource name in space of the archive.
	 * @return a input stream object
	 */
	virtual std::unique_ptr<CInputStream> load(const ResourcePath & resourceName) const = 0;

	/**
	 * Checks if the entry exists.
	 *
	 * @return Returns true if the entry exists, false if not.
	 */
	virtual bool existsResource(const ResourcePath & resourceName) const = 0;

	/**
	 * Gets mount point to which this loader was attached
	 *
	 * @return mount point URI
	 */
	virtual std::string getMountPoint() const = 0;

	/**
	 * Gets full name of resource, e.g. name of file in filesystem.
	 *
	 * @return path or empty optional if file can't be accessed independently (e.g. file in archive)
	 */
	virtual std::optional<boost::filesystem::path> getResourceName(const ResourcePath & resourceName) const
	{
		return std::optional<boost::filesystem::path>();
	}

	/**
	 * Gets all full names of matching resources, e.g. names of files in filesystem.
	 *
	 * @return std::set with names.
	 */
	virtual std::set<boost::filesystem::path> getResourceNames(const ResourcePath & resourceName) const
	{
		std::set<boost::filesystem::path> result;
		auto rn = getResourceName(resourceName);
		if(rn)
		{
			result.insert(rn->string());
		}
		return result;
	}

	/**
	 * Update lists of files that match filter function
	 *
	 * @param filter Filter that returns true if specified mount point matches filter
	 */
	virtual void updateFilteredFiles(std::function<bool(const std::string &)> filter) const = 0;

	/**
	 * Get list of files that match filter function
	 *
	 * @param filter Filter that returns true if specified ID matches filter
	 * @return Returns list of flies
	 */
	virtual std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const = 0;

	/**
	 * Creates new resource with specified filename.
	 *
	 * @return true if new file was created, false on error or if file already exists
	 */
	virtual bool createResource(const std::string & filename, bool update = false)
	{
		return false;
	}

	/**
	 * @brief Returns all loaders that have resource with such name
	 *
	 * @return vector with all loaders
	 */
	virtual std::vector<const ISimpleResourceLoader *> getResourcesWithName(const ResourcePath & resourceName) const
	{
		if (existsResource(resourceName))
			return std::vector<const ISimpleResourceLoader *>(1, this);
		return std::vector<const ISimpleResourceLoader *>();
	}
};

VCMI_LIB_NAMESPACE_END
