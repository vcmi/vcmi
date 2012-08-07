
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

#include "CInputStream.h"
#include "CResourceLoader.h" //FIXME: move ResourceID + EResType in separate file?

/**
 * A class which knows the files containing in the archive or system and how to load them.
 */
class DLL_LINKAGE ISimpleResourceLoader
{
public:
	/**
	 * Dtor.
	 */
	virtual ~ISimpleResourceLoader() { };

	/**
	 * Loads a resource with the given resource name.
	 *
	 * @param resourceName The unqiue resource name in space of the archive.
	 * @return a input stream object
	 */
	virtual std::unique_ptr<CInputStream> load(const std::string & resourceName) const =0;

	/**
	 * Checks if the entry exists.
	 *
	 * @return Returns true if the entry exists, false if not.
	 */
	virtual bool existsEntry(const std::string & resourceName) const =0;

	/**
	 * Gets all entries in the archive or (file) system.
	 *
	 * @return Returns a list of all entries in the archive or (file) system.
	 */
	virtual std::unordered_map<ResourceID, std::string> getEntries() const =0;

	/**
	 * Gets the origin of the loader.
	 *
	 * @return the file path to source of this loader
	 */
	virtual std::string getOrigin() const =0;

	/**
	 * Creates new resource with specified filename.
	 *
	 * @returns true if new file was created, false on error or if file already exists
	 */
	virtual bool createEntry(std::string filename)
	{
		return false;
	}
};
