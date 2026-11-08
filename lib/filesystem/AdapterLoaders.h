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

class CInputStream;

class CFilesystemList : public ISimpleResourceLoader
{
	std::vector<std::shared_ptr<ISimpleResourceLoader>> loaders;

	std::set<ISimpleResourceLoader *> writeableLoaders;

	/// Resource hash -> position in `loaders` of the last child that has a resource with this hash.
	/// Kept only for lists with many children. Stays correct only if children gain resources through this list
	std::unordered_map<size_t, size_t> index;
	mutable std::shared_mutex indexGuard;

	void indexLoader(size_t position);
	void rebuildIndex();

	/// Loader that provides the resource, or nullptr. Every lookup goes through here,
	/// so that a resource is located exactly once per query
	const ISimpleResourceLoader * getLoader(const ResourcePath & resourceName) const;

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
	bool updateFilteredFiles(std::function<bool(const std::string &)> filter) override;
	std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const override;
	bool createResource(const std::string & filename, bool update = false) override;
	bool removeResource(const ResourcePath & resourceName) override;
	std::vector<const ISimpleResourceLoader *> getResourcesWithName(const ResourcePath & resourceName) const override;
	std::string getFullFileURI(const ResourcePath& resourceName) const override;
	std::time_t getLastWriteTime(const ResourcePath& resourceName) const override;

	/**
	 * Adds a resource loader to the loaders list
	 * Loader can be shared with other lists
	 *
	 * @param loader The simple resource loader object to add
	 * @param writeable - resource shall be treated as writeable
	 */
	void addLoader(std::shared_ptr<ISimpleResourceLoader> loader, bool writeable);

	/**
	 * Removes loader from the loader list
	 *
	 * @param loader The simple resource loader object to remove
	 *
	 * @return if loader was successfully removed
	 */
	bool removeLoader(ISimpleResourceLoader * loader);
};
