
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

class ResourceLocator;
class ISimpleResourceLoader;

/**
 * Specifies the resource type.
 *
 * Supported file extensions:
 *
 * Text: .txt .json
 * Animation: .def
 * Mask: .msk
 * Campaign: .h3c
 * Map: .h3m
 * Font: .fnt
 * Image: .bmp, .jpg, .pcx, .png, .tga
 * Sound: .wav
 * Video: .smk, .bik .mjpg
 * Music: .mp3, .ogg
 * Archive: .lod, .snd, .vid
 * Savegame: .v*gm1
 */
enum EResType
{
	ANY,
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
	ARCHIVE,
	CLIENT_SAVEGAME,
	LIB_SAVEGAME,
	SERVER_SAVEGAME,
	OTHER
};

/**
 * A struct which identifies a resource clearly.
 */
class DLL_LINKAGE ResourceIdentifier
{
public:
	/**
	 * Default c-tor.
	 */
	ResourceIdentifier();

	/**
	 * Ctor.
	 *
	 * @param name The resource name.
	 * @param type The resource type. A constant from the enumeration EResType.
	 */
	ResourceIdentifier(const std::string & name, EResType type);

	/**
	 * Compares this object with a another resource identifier.
	 *
	 * @param other The other resource identifier.
	 * @return Returns true if both are equally, false if not.
	 */
	inline bool operator==(ResourceIdentifier const & other) const
	{
		return name == other.name && type == other.type;
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
	EResType getType() const;

	/**
	 * Sets the name of the identifier.
	 *
	 * @param name the name of the identifier. No extension, will be converted to uppercase.
	 */
	void setName(const std::string & name);

	/**
	 * Sets the type of the identifier.
	 *
	 * @param type the type of the identifier.
	 */
	void setType(EResType type);

private:
	/** Specifies the resource name. No extension so .pcx and .png can override each other, always in upper case. **/
	std::string name;

	/**
	 * Specifies the resource type. EResType::OTHER if not initialized.
	 * Required to prevent conflicts if files with different types (e.g. text and image) have the same name.
	 */
	EResType type;
};

namespace std
{
	/**
	 * Template specialization for std::hash.
	 */
    template <>
    class hash<ResourceIdentifier>
    {
public:
    	/**
    	 * Generates a hash value for the resource identifier object.
    	 *
    	 * @param resourceIdent The object from which a hash value should be generated.
    	 * @return the generated hash value
    	 */
		size_t operator()(const ResourceIdentifier & resourceIdent) const
		{
			return hash<string>()(resourceIdent.getName()) ^ hash<int>()(static_cast<int>(resourceIdent.getType()));
		}
    };
};

/**
 * This class manages the loading of resources whether standard
 * or derived from several container formats and the file system.
 */
class DLL_LINKAGE CResourceLoader : public boost::noncopyable
{
public:
	CResourceLoader();

	/**
	 * D-tor.
	 */
	virtual ~CResourceLoader();

	/**
	 * Loads the resource specified by the resource identifier.
	 *
	 * @param resourceIdent This parameter identifies the resource to load.
	 * @return a pointer to the input stream, not null
	 *
	 * @throws std::runtime_error if the resource doesn't exists
	 */
	std::unique_ptr<CInputStream> load(const ResourceIdentifier & resourceIdent) const;

	/**
	 * Tests whether the specified resource exists.
	 *
	 * @param resourceIdent the resource which should be checked
	 * @return true if the resource exists, false if not
	 */
	bool existsResource(const ResourceIdentifier & resourceIdent) const;

	/**
	 * Adds a simple resource loader to the loaders list and its entries to the resources list.
	 *
	 * The loader object will be destructed when this resource loader is destructed.
	 * Don't delete it manually.
	 *
	 * @param loader The simple resource loader object to add
	 */
	void addLoader(ISimpleResourceLoader * loader);

private:

	/**
	 * Contains lists of same resources which can be accessed uniquely by an
	 * resource identifier.
	 */
	std::unordered_map<ResourceIdentifier, std::list<ResourceLocator> > resources;

	/** A list of resource loader objects */
	std::list<ISimpleResourceLoader *> loaders;
};

/**
 * This class has static methods for a global resource loader access.
 *
 * Note: Compared to the singleton pattern it has the advantage that the class CResourceLoader
 * and classes which use it can be tested separately. CResourceLoader can be sub-classes as well.
 * Compared to a global variable the factory pattern can throw an exception if the resource loader wasn't
 * initialized.
 *
 * This class is not thread-safe. Make sure nobody is calling getInstance while somebody else is calling setInstance.
 */
class DLL_LINKAGE CResourceLoaderFactory
{
public:
	/**
	 * Gets an instance of resource loader.
	 *
	 * Make sure that you've set an instance before using it. It'll throw an exception if no instance was set.
	 *
	 * @return Returns an instance of resource loader.
	 */
	static CResourceLoader * getInstance();

	/**
	 * Sets an instance of resource loader.
	 *
	 * @param resourceLoader An instance of resource loader.
	 */
	static void setInstance(CResourceLoader * resourceLoader);

private:
	/** Instance of resource loader */
	static CResourceLoader * resourceLoader;
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
	static EResType getTypeFromExtension(std::string extension);

	/**
	 * Gets the EResType as a string representation.
	 *
	 * @param type the EResType
	 * @return the type as a string representation
	 */
	static std::string getEResTypeAsString(EResType type);
};
