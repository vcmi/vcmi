/*
 * ResourceID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN


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
		OTHER,
		UNDEFINED,
		LUA
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
	//ResourceID();

	/**
	 * Ctor. Can be used to create identifier for resource loading using one parameter
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

	std::string		getName() const {return name;}
	EResType::Type	getType() const {return type;}
	//void setName(std::string name);
	//void setType(EResType::Type type);

private:
	/**
	 * Specifies the resource type. EResType::OTHER if not initialized.
	 * Required to prevent conflicts if files with different types (e.g. text and image) have the same name.
	 */
	EResType::Type type;

	/** Specifies the resource name. No extension so .pcx and .png can override each other, always in upper case. **/
	std::string name;
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

VCMI_LIB_NAMESPACE_END


namespace std
{
template <> struct hash<VCMI_LIB_WRAP_NAMESPACE(ResourceID)>
{
	size_t operator()(const VCMI_LIB_WRAP_NAMESPACE(ResourceID) & resourceIdent) const
	{
		std::hash<int> intHasher;
		std::hash<std::string> stringHasher;
		return stringHasher(resourceIdent.getName()) ^ intHasher(static_cast<int>(resourceIdent.getType()));
	}
};
}
