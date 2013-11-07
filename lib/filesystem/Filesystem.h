#pragma once

/*
 * Filesystem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CInputStream.h"
#include "ISimpleResourceLoader.h"

class CFilesystemList;
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
 * Archive: .lod, .snd, .vid .pac .zip
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
		BMP_FONT,
		TTF_FONT,
		IMAGE,
		VIDEO,
		SOUND,
		MUSIC,
		ARCHIVE_VID,
		ARCHIVE_ZIP,
		ARCHIVE_SND,
		ARCHIVE_LOD,
		PALETTE,
		CLIENT_SAVEGAME,
		SERVER_SAVEGAME,
		DIRECTORY,
		ERM,
		ERT,
		ERS,
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
	 * Ctor. Can be used to create indentifier for resource loading using one parameter
	 *
	 * @param fullName The resource name including extension.
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

	std::string getName() const;
	EResType::Type getType() const;
	void setName(std::string name);
	void setType(EResType::Type type);

private:
	/** Specifies the resource name. No extension so .pcx and .png can override each other, always in upper case. **/
	std::string name;

	/**
	 * Specifies the resource type. EResType::OTHER if not initialized.
	 * Required to prevent conflicts if files with different types (e.g. text and image) have the same name.
	 */
	EResType::Type type;
};

namespace std
{
	template <> struct hash<ResourceID>
	{
		size_t operator()(const ResourceID & resourceIdent) const
		{
			std::hash<int> intHasher;
			std::hash<std::string> stringHasher;
			return stringHasher(resourceIdent.getName()) ^ intHasher(static_cast<int>(resourceIdent.getType()));
		}
	};
}

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
	static ISimpleResourceLoader * get();

	/**
	 * Creates instance of resource loader.
	 * Will not fill filesystem with data
	 *
	 */
	static void initialize();

	/**
	 * Semi-debug method to track all possible cases of memory leaks
	 * Used before exiting application
	 *
	 */
	static void clear();

	/**
	 * Will load all filesystem data from Json data at this path (config/filesystem.json)
	 * @param prefix - prefix for all paths in filesystem config
	 */
	static void loadModFileSystem(const std::string &prefix, const JsonNode & fsConfig);
	static void loadMainFileSystem(const std::string & fsConfigURI);
	static void loadDirectory(const std::string &prefix, const std::string & mountPoint, const JsonNode & config);
	static void loadZipArchive(const std::string &prefix, const std::string & mountPoint, const JsonNode & config);
	static void loadArchive(const std::string &prefix, const std::string & mountPoint, const JsonNode & config, EResType::Type archiveType);
	static void loadJsonMap(const std::string &prefix, const std::string & mountPoint, const JsonNode & config);

	/**
	 * Checks all subfolders of MODS directory for presence of mods
	 * If this directory has mod.json file it will be added to resources
	 */
	static std::vector<std::string> getAvailableMods();
	static void setActiveMods(std::vector<std::string> enabledMods); //WARNING: not reentrable. Do not call it twice!!!

private:
	/** Instance of resource loader */
	static CFilesystemList * resourceLoader;
	static CFilesystemList * initialLoader;
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
