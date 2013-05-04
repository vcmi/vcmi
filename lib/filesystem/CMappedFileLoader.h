
/*
 * CMappedFileLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISimpleResourceLoader.h"
#include "CResourceLoader.h"

class CFileInfo;
class CInputStream;

/**
 * Class that implements file mapping (aka *nix symbolic links)
 * Uses json file as input, content is map:
 * "fileA.txt" : "fileB.txt"
 * Note that extension is necessary, but used only to determine type
 *
 * fileA - file which will be replaced
 * fileB - file which will be used as replacement
 */
class DLL_LINKAGE CMappedFileLoader : public ISimpleResourceLoader
{
public:
	/**
	 * Ctor.
	 *
	 * @param config Specifies filesystem configuration
	 */
	explicit CMappedFileLoader(const JsonNode & config);

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const std::string & resourceName) const override;
	bool existsEntry(const std::string & resourceName) const override;
	boost::unordered_map<ResourceID, std::string> getEntries() const override;
	std::string getOrigin() const override;
	std::string getFullName(const std::string & resourceName) const override;

private:
	/** A list of files in this map
	 * key = ResourceID for resource loader
	 * value = ResourceID to which file this request will be redirected
	*/
	boost::unordered_map<ResourceID, std::string> fileList;
};
