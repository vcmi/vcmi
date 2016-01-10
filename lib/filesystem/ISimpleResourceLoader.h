#pragma once

/*
 * ISimpleResourceLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CInputStream;
class ResourceID;

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
	virtual std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const = 0;

	/**
	 * Checks if the entry exists.
	 *
	 * @return Returns true if the entry exists, false if not.
	 */
	virtual bool existsResource(const ResourceID & resourceName) const = 0;

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
	virtual boost::optional<boost::filesystem::path> getResourceName(const ResourceID & resourceName) const
	{
		return boost::optional<boost::filesystem::path>();
	}

	/**
	 * Get list of files that matches filter function
	 *
	 * @param filter Filter that returns true if specified ID matches filter
	 * @return Returns list of flies
	 */
	virtual std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const = 0;

	/**
	 * Creates new resource with specified filename.
	 *
	 * @return true if new file was created, false on error or if file already exists
	 */
	virtual bool createResource(std::string filename, bool update = false)
	{
		return false;
	}

	/**
	 * @brief Returns all loaders that have resource with such name
	 *
	 * @return vector with all loaders
	 */
	virtual std::vector<const ISimpleResourceLoader *> getResourcesWithName(const ResourceID & resourceName) const
	{
		if (existsResource(resourceName))
			return std::vector<const ISimpleResourceLoader *>(1, this);
		return std::vector<const ISimpleResourceLoader *>();
	}
};
