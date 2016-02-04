#pragma once

/*
 * AdapterLoaders.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ISimpleResourceLoader.h"
#include "ResourceID.h"

class CFileInfo;
class CInputStream;
class JsonNode;

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
	explicit CMappedFileLoader(const std::string &mountPoint, const JsonNode & config);

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const override;
	bool existsResource(const ResourceID & resourceName) const override;
	std::string getMountPoint() const override;
	boost::optional<boost::filesystem::path> getResourceName(const ResourceID & resourceName) const override;
	std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const override;

private:
	/** A list of files in this map
	 * key = ResourceID for resource loader
	 * value = ResourceID to which file this request will be redirected
	*/
	std::unordered_map<ResourceID, ResourceID> fileList;
};

class DLL_LINKAGE CFilesystemList : public ISimpleResourceLoader
{
	std::vector<std::unique_ptr<ISimpleResourceLoader> > loaders;

	std::set<ISimpleResourceLoader *> writeableLoaders;

	//FIXME: this is only compile fix, should be removed in the end
	CFilesystemList(CFilesystemList &) = delete;
	CFilesystemList &operator=(CFilesystemList &) = delete;

public:
	CFilesystemList();
	~CFilesystemList();
	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const override;
	bool existsResource(const ResourceID & resourceName) const override;
	std::string getMountPoint() const override;
	boost::optional<boost::filesystem::path> getResourceName(const ResourceID & resourceName) const override;
	std::set<boost::filesystem::path> getResourceNames(const ResourceID & resourceName) const override;
	std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const override;
	bool createResource(std::string filename, bool update = false) override;
	std::vector<const ISimpleResourceLoader *> getResourcesWithName(const ResourceID & resourceName) const override;

	/**
	 * Adds a resource loader to the loaders list
	 * Passes loader ownership to this object
	 *
	 * @param loader The simple resource loader object to add
	 * @param writeable - resource shall be treated as writeable
	 */
	void addLoader(ISimpleResourceLoader * loader, bool writeable);
};
