
/*
 * CResourceLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CInputStream.h"

class CResourceLoader;
class ResourceLocator;
class ISimpleResourceLoader;
class JsonNode;

/**
 * Specifies the resource type.
 *
 * Supported file extensions:
 *
 * Text: .txt .json
 * Animation: .def
 * Mask: .msk .msg
 * Campaign: .h3c
 * Map: .h3m
 * Font: .fnt
 * Image: .bmp, .jpg, .pcx, .png, .tga
 * Sound: .wav .82m
 * Video: .smk, .bik .mjpg .mpg
 * Music: .mp3, .ogg
 * Archive: .lod, .snd, .vid .pac
 * Palette: .pal
 * Savegame: .v*gm1
 */
namespace EResType
{
	enum Type
	{
		TEXT,
		ANIMATION,
		MASK,
		CAMPAIGN,
		MAP,
		FONT,
		IMAGE,
		VIDEO,
		SOUND,
		MUSIC,
		ARCHIVE_VID,
		ARCHIVE_SND,
		ARCHIVE_LOD,
		PALETTE,
		CLIENT_SAVEGAME,
		LIB_SAVEGAME,
		SERVER_SAVEGAME,
		DIRECTORY,
		OTHER
	};
}

/**
 * A struct which identifies a resource clearly.
 */
class DLL_LINKAGE ResourceID
{
public:
	/**
	 * Default c-tor.
	 */
	ResourceID();

	/**
	 * Move Ctor.
	 */
	ResourceID(ResourceID && other)
		: name(std::move(other.name)), type(other.getType())
	{

	}

	/**
	 * Copy Ctor. Required by clang (or this is standard?) if move constructor is present
	 */
	ResourceID(const ResourceID & other)
	    : name(other.getName()), type(other.getType())
	{
	}

	/**
	 * Ctor. Can be used to create indentifier for resource loading using one parameter
	 *
	 * @param name The resource name including extension.
	 */
	explicit ResourceID(std::string fullName);

	/**
	 * Ctor.
	 *
	 * @param name The resource name.
	 * @param type The resource type. A constant from the enumeration EResType.
	 */
	ResourceID(std::string name, EResType::Type type);

	/**
	 * Compares this object with a another resource identifier.
	 *
	 * @param other The other resource identifier.
	 * @return Returns true if both are equally, false if not.
	 */
	inline bool operator==(ResourceID const & other) const
	{
		return name == other.name && type == other.type;
	}

	/*
	 * Move-assignment operator.
	 */
	inline ResourceID& operator=(ResourceID && other)
	{
		name = std::move(other.name);
		type = other.getType();
		return *this;
	}

	/**
	 * Gets the name of the identifier.
	 *
	 * @return the name of the identifier
	 */
	std::string getName() const;

	/**
	 * Gets the type of the identifier.
	 *
	 * @return the type of the identifier
	 */
	EResType::Type getType() const;

	/**
	 * Sets the name of the identifier.
	 *
	 * @param name the name of the identifier. No extension, will be converted to uppercase.
	 */
	void setName(std::string name);

	/**
	 * Sets the type of the identifier.
	 *
	 * @param type the type of the identifier.
	 */
	void setType(EResType::Type type);

protected:
	/**
	 * Ctor for usage strictly in resourceLoader for some speedup
	 *
	 * @param prefix Prefix of ths filename, already in upper case
	 * @param name The resource name, upper case
	 * @param type The resource type. A constant from the enumeration EResType.
	 */
	ResourceID(const std::string & prefix, const std::string & name, EResType::Type type);

	friend class CResourceLoader;
private:
	/** Specifies the resource name. No extension so .pcx and .png can override each other, always in upper case. **/
	std::string name;

	/**
	 * Specifies the resource type. EResType::OTHER if not initialized.
	 * Required to prevent conflicts if files with different types (e.g. text and image) have the same name.
	 */
	EResType::Type type;
};

/**
 	* Generates a hash value for the resource identifier object.
 	*
 	* @param resourceIdent The object from which a hash value should be generated.
 	* @return the generated hash value
 	*/
inline size_t hash_value(const ResourceID & resourceIdent)
{
	boost::hash<int> intHasher;
	boost::hash<std::string> stringHasher;
	return stringHasher(resourceIdent.getName()) ^ intHasher(static_cast<int>(resourceIdent.getType()));
}

/**
 * This class manages the loading of resources whether standard
 * or derived from several container formats and the file system.
 */
class DLL_LINKAGE CResourceLoader
{
	typedef boost::unordered_map<ResourceID, std::vector<ResourceLocator> > ResourcesMap;

public:
	/// class for iterating over all available files/Identifiers
	/// can be created via CResourceLoader::getIterator
	template <typename Comparator, typename Iter>
	class Iterator
	{
	public:
		/// find next available item.
		Iterator& operator++()
		{
			assert(begin != end);
			begin++;
			findNext();
			return *this;
		}
		bool hasNext()
		{
			return begin != end;
		}

		/// get identifier of current item
		const ResourceID & operator* () const
		{
			assert(begin != end);
			return begin->first;
		}

		/// get identifier of current item
		const ResourceID * operator -> () const
		{
			assert(begin != end);
			return &begin->first;
		}

	protected:
		Iterator(Iter begin, Iter end, Comparator comparator):
		    begin(begin),
		    end(end),
		    comparator(comparator)
		{
			//find first applicable item
			findNext();
		}

		friend class CResourceLoader;

	private:
		Iter begin;
		Iter end;
		Comparator comparator;

		void findNext()
		{
			while (begin != end && !comparator(begin->first))
				begin++;
		}

	};

	CResourceLoader();

	/**
	 * Loads the resource specified by the resource identifier.
	 *
	 * @param resourceIdent This parameter identifies the resource to load.
	 * @return a pointer to the input stream, not null
	 *
	 * @throws std::runtime_error if the resource doesn't exists
	 */
	std::unique_ptr<CInputStream> load(const ResourceID & resourceIdent) const;
	/// temporary member to ease transition to new filesystem classes
	std::pair<std::unique_ptr<ui8[]>, ui64> loadData(const ResourceID & resourceIdent) const;

	/**
	 * Get resource locator for this identifier
	 *
	 * @param resourceIdent This parameter identifies the resource to load.
	 * @return resource locator for this resource or empty one if resource was not found
	 */
	ResourceLocator getResource(const ResourceID & resourceIdent) const;

	/// returns ALL overriden resources with same name, including last one acessible via getResource
	const std::vector<ResourceLocator> & getResourcesWithName(const ResourceID & resourceIdent) const;

	/// returns real name of file in filesystem. Not usable for archives
	std::string getResourceName(const ResourceID & resourceIdent) const;

	/**
	 * Get iterator for looping all files matching filter
	 * Notes:
	 * - iterating over all files may be slow. Use with caution
	 * - all filenames are in upper case
	 *
	 * @param filter functor with signature bool(ResourceIdentifier) used to check if this file is required
	 * @return resource locator for this resource or empty one if resource was not found
	 */
	template<typename Comparator>
	Iterator<Comparator, ResourcesMap::const_iterator> getIterator(Comparator filter) const
	{
		return Iterator<Comparator, ResourcesMap::const_iterator>(resources.begin(), resources.end(), filter);
	}

	/**
	 * Tests whether the specified resource exists.
	 *
	 * @param resourceIdent the resource which should be checked
	 * @return true if the resource exists, false if not
	 */
	bool existsResource(const ResourceID & resourceIdent) const;

	/**
	 * Creates new resource (if not exists) with specified URI.
	 * Type will be determined from extension
	 * File case will be same as in URI
	 *
	 * @param URI file to create
	 * @return true on success, false if resource exists or on error
	 */
	bool createResource(std::string URI);

	/**
	 * Adds a simple resource loader to the loaders list and its entries to the resources list.
	 *
	 * The loader object will be destructed when this resource loader is destructed.
	 * Don't delete it manually.
	 * Same loader can be added multiple times (with different mount point)
	 *
	 * @param mountPoint prefix that will be added to all files in this loader
	 * @param loader The simple resource loader object to add
	 */
	void addLoader(std::string mountPoint, shared_ptr<ISimpleResourceLoader> loader, bool writeable);

private:
	/**
	 * Contains lists of same resources which can be accessed uniquely by an
	 * resource identifier.
	 */
	ResourcesMap resources;

	struct LoaderEntry
	{
		std::string prefix;
		shared_ptr<ISimpleResourceLoader> loader;
		bool writeable;
	};

	/** A list of resource loader objects */
	std::vector<LoaderEntry > loaders;
};

/**
 * This class has static methods for a global resource loader access.
 *
 * Class is not thread-safe. Make sure nobody is calling getInstance while somebody else is calling initialize.
 */
class DLL_LINKAGE CResourceHandler
{
public:
	/**
	 * Gets an instance of resource loader.
	 *
	 * Make sure that you've set an instance before using it. It'll throw an exception if no instance was set.
	 *
	 * @return Returns an instance of resource loader.
	 */
	static CResourceLoader * get();

	/**
	 * Creates instance of resource loader.
	 * Will not fill filesystem with data
	 *
	 */
	static void initialize();

	/**
	 * Will load all filesystem data from Json data at this path (config/filesystem.json)
	 */
	static void loadFileSystem(const std::string & fsConfigURI);
	static void loadDirectory(const std::string & mountPoint, const JsonNode & config);
	static void loadArchive(const std::string & mountPoint, const JsonNode & config, EResType::Type archiveType);

	/**
	 * Checks all subfolders of MODS directory for presence of mods
	 * If this directory has mod.json file it will be added to resources
	 */
	static std::vector<std::string> getAvailableMods();
	static void setActiveMods(std::vector<std::string> enabledMods); //WARNING: not reentrable. Do not call it twice!!!

private:
	/** Instance of resource loader */
	static CResourceLoader * resourceLoader;
	static CResourceLoader * initialLoader;
};

/**
 * A struct which describes the exact position of a resource.
 */
class DLL_LINKAGE ResourceLocator
{
public:
	/**
	 * Ctor.
	 *
	 * @param archive A pointer to the resource archive object.
	 * @param resourceName Unique resource name in the space of the given resource archive.
	 */
	ResourceLocator(ISimpleResourceLoader * loader, const std::string & resourceName);

	/**
	 * Gets a pointer to the resource loader object.
	 *
	 * @return a pointer to the resource loader object
	 */
	ISimpleResourceLoader * getLoader() const;

	/**
	 * Gets the resource name.
	 *
	 * @return the resource name.
	 */
	std::string getResourceName() const;

private:
	/**
	 * A pointer to the loader which knows where and how to construct a stream object
	 * which does the loading process actually.
	 */
	ISimpleResourceLoader * loader;

	/** A unique name of the resource in space of the loader. */
	std::string resourceName;
};

/**
 * A helper class which provides a functionality to convert extension strings to EResTypes.
 */
class DLL_LINKAGE EResTypeHelper
{
public:
	/**
	 * Converts a extension string to a EResType enum object.
	 *
	 * @param extension The extension string e.g. .BMP, .PNG
	 * @return Returns a EResType enum object
	 */
	static EResType::Type getTypeFromExtension(std::string extension);

	/**
	 * Gets the EResType as a string representation.
	 *
	 * @param type the EResType
	 * @return the type as a string representation
	 */
	static std::string getEResTypeAsString(EResType::Type type);
};
