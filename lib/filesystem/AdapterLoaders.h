/*
 * AdapterLoaders.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ISimpleResourceLoader.h"
#include "ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

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
	std::unique_ptr<CInputStream> load(const ResourcePath & resourceName) const override;
	bool existsResource(const ResourcePath & resourceName) const override;
	std::string getMountPoint() const override;
	std::optional<boost::filesystem::path> getResourceName(const ResourcePath & resourceName) const override;
	void updateFilteredFiles(std::function<bool(const std::string &)> filter) const override {}
	std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const override;

private:
	/** A list of files in this map
	 * key = ResourcePath for resource loader
	 * value = ResourcePath to which file this request will be redirected
	*/
	std::unordered_map<ResourcePath, ResourcePath> fileList;
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
	std::unique_ptr<CInputStream> load(const ResourcePath & resourceName) const override;
	bool existsResource(const ResourcePath & resourceName) const override;
	std::string getMountPoint() const override;
	std::optional<boost::filesystem::path> getResourceName(const ResourcePath & resourceName) const override;
	std::set<boost::filesystem::path> getResourceNames(const ResourcePath & resourceName) const override;
	void updateFilteredFiles(std::function<bool(const std::string &)> filter) const override;
	std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const override;
	bool createResource(std::string filename, bool update = false) override;
	std::vector<const ISimpleResourceLoader *> getResourcesWithName(const ResourcePath & resourceName) const override;

	/**
	 * Adds a resource loader to the loaders list
	 * Passes loader ownership to this object
	 *
	 * @param loader The simple resource loader object to add
	 * @param writeable - resource shall be treated as writeable
	 */
	void addLoader(ISimpleResourceLoader * loader, bool writeable);
	
	/**
	 * Removes loader from the loader list
	 * Take care about memory deallocation
	 *
	 * @param loader The simple resource loader object to remove
	 *
	 * @return if loader was successfully removed
	 */
	bool removeLoader(ISimpleResourceLoader * loader);
};

VCMI_LIB_NAMESPACE_END
