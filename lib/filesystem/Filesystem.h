/*
 * Filesystem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CInputStream.h"
#include "ISimpleResourceLoader.h"
#include "ResourcePath.h"

class CFilesystemList;
class JsonNode;

/// Helper class that allows generation of a ISimpleResourceLoader entry out of Json config(s)
class CFilesystemGenerator : boost::noncopyable
{
	using TLoadFunctor = std::function<void(const std::string &, const JsonNode &)>;
	using TLoadFunctorMap = std::map<std::string, TLoadFunctor>;

	std::vector<std::unique_ptr<ISimpleResourceLoader>> loaders;
	std::string prefix;

	template<EResType archiveType>
	void loadArchive(const std::string & mountPoint, const JsonNode & config);
	void loadDirectory(const std::string & mountPoint, const JsonNode & config);
	void loadZipArchive(const std::string & mountPoint, const JsonNode & config);
	void loadJsonMap(const std::string & mountPoint, const JsonNode & config);

	TLoadFunctorMap genFunctorMap();
public:
	/// prefix = prefix that will be given to file entries in all nodes of this filesystem
	/// extractArchives = Specifies if Original H3 archives should be extracted to a separate folder
	CFilesystemGenerator(std::string prefix, bool extractArchives = false);
	~CFilesystemGenerator();

	/// loads configuration from json
	/// config - configuration to load, using format of "filesystem" entry in config/filesystem.json
	void loadConfig(const JsonNode & config);

	/// returns generated filesystem: nullptr if none of data sources exist, data source itself if only one exists, or list of all data sources
	std::unique_ptr<ISimpleResourceLoader> acquireFilesystem();

	/** Specifies if Original H3 archives should be extracted to a separate folder **/
	bool extractArchives;
};

/**
 * This class has static methods for a global resource loader access.
 *
 * Class is not thread-safe.
 */
class DLL_LINKAGE CResourceHandler
{
	/**
	 * @brief createInitial - creates instance of initial loader
	 * that contains data necessary to load main FS
	 */
	static std::unique_ptr<ISimpleResourceLoader> createInitial();

public:
	/**
	 * Gets an instance of resource loader.
	 *
	 * Make sure that you've set an instance before using it. It'll throw an exception if no instance was set.
	 *
	 * @return Returns an instance of resource loader.
	 */
	static ISimpleResourceLoader * get();
	static ISimpleResourceLoader * get(const std::string & identifier);

	/**
	 * Creates instance of initial resource loader.
	 * Will not fill filesystem with data
	 *
	 */
	static void initialize();

	/**
	 * Destroys all existing data in filesystem, bringing it into uninitialized state
	 *
	 */
	static void destroy();

	/**
	 * Will load all filesystem data from Json data at this path (normally - config/filesystem.json)
	 * @param fsConfigURI - URI from which data will be loaded
	 */
	static void load(const std::string & fsConfigURI, bool extractArchives = false);

	/**
	 * @brief addFilesystem adds filesystem into global resource loader
	 * @param identifier name of this loader by which it can be retrieved later
	 * @param loader resource loader to add. If null, identifier will be accessible as empty filesystem
	 */
	static void addFilesystem(const std::string & parent, const std::string & identifier, std::unique_ptr<ISimpleResourceLoader> loader);
	
	/**
	 * @brief removeFilesystem removes previously added filesystem from global resource holder
	 * @param parent parent loader containing filesystem
	 * @param identifier name of this loader
	 * @return if filesystem was successfully removed
	 */
	static bool removeFilesystem(const std::string & parent, const std::string & identifier);

	/**
	 * @brief createModFileSystem - creates filesystem out of config file
	 * @param prefix - prefix for all paths in filesystem config
	 * @param fsConfig - configuration to load
	 * @return generated filesystem that contains all config entries, or nullptr if none of entries exist
	 */
	static std::unique_ptr<ISimpleResourceLoader> createFileSystem(const std::string &prefix, const JsonNode & fsConfig, bool extractArchives = false);

	~CResourceHandler() = default;
private:
	/** Instance of resource loader */
	static std::map<std::string, ISimpleResourceLoader*> knownLoaders;
	static CResourceHandler globalResourceHandler;

	CResourceHandler() {};
	std::unique_ptr<ISimpleResourceLoader> rootLoader;
	/// shared by all filesystems that have no data sources, not attached to any parent
	std::unique_ptr<ISimpleResourceLoader> emptyLoader;
};
