/*
 * ResourcePath.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class JsonSerializeFormat;

/**
 * Specifies the resource type.
 *
 * Supported file extensions:
 *
 * Text: .txt
 * Json: .json
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
enum class EResType
{
	TEXT,
	JSON,
	ANIMATION,
	MASK,
	CAMPAIGN,
	MAP,
	BMP_FONT,
	TTF_FONT,
	IMAGE,
	VIDEO,
	SOUND,
	ARCHIVE_VID,
	ARCHIVE_ZIP,
	ARCHIVE_SND,
	ARCHIVE_LOD,
	PALETTE,
	SAVEGAME,
	DIRECTORY,
	ERM,
	ERT,
	ERS,
	LUA,
	OTHER,
	UNDEFINED,
};

/**
 * A struct which identifies a resource clearly.
 */
class DLL_LINKAGE ResourcePath
{
protected:
	/// Constructs resource path based on JsonNode and selected type. File extension is ignored
	ResourcePath(const JsonNode & name, EResType type);

public:
	/// Constructs resource path based on full name including extension
	explicit ResourcePath(const std::string & fullName);

	/// Constructs resource path based on filename and selected type. File extension is ignored
	ResourcePath(const std::string & name, EResType type);

	inline bool operator==(const ResourcePath & other) const
	{
		return name == other.name && type == other.type;
	}

	inline bool operator!=(const ResourcePath & other) const
	{
		return name != other.name || type != other.type;
	}

	inline bool operator<(const ResourcePath & other) const
	{
		if (type != other.type)
			return type < other.type;
		return name < other.name;
	}

	bool empty() const {return name.empty();}
	std::string getName() const {return name;}
	std::string getOriginalName() const {return originalName;}
	EResType getType() const {return type;}

	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & type;
		h & name;
		h & originalName;
	}

protected:
	 /// Specifies the resource type. EResType::OTHER if not initialized.
	 /// Required to prevent conflicts if files with different types (e.g. text and image) have the same name.
	EResType type;

	/// Specifies the resource name. No extension so .pcx and .png can override each other, always in upper case.
	std::string name;

	/// name in original case
	std::string originalName;
};

template<EResType Type>
class ResourcePathTempl : public ResourcePath
{
	template <EResType>
	friend class ResourcePathTempl;

	ResourcePathTempl(const ResourcePath & copy)
		:ResourcePath(copy)
	{
		type = Type;
	}

	ResourcePathTempl(const std::string & path)
		:ResourcePath(path, Type)
	{}

	ResourcePathTempl(const JsonNode & name)
		:ResourcePath(name, Type)
	{}

public:
	ResourcePathTempl()
		:ResourcePath("", Type)
	{}

	static ResourcePathTempl fromResource(const ResourcePath & resource)
	{
		assert(Type == resource.getType());
		return ResourcePathTempl(resource);
	}

	static ResourcePathTempl builtin(const std::string & filename)
	{
		return ResourcePathTempl(filename);
	}

	static ResourcePathTempl builtinTODO(const std::string & filename)
	{
		return ResourcePathTempl(filename);
	}

	static ResourcePathTempl fromJson(const JsonNode & path)
	{
		return ResourcePathTempl(path);
	}

	template<EResType Type2>
	ResourcePathTempl<Type2> toType() const
	{
		ResourcePathTempl<Type2> result(static_cast<ResourcePath>(*this));
		return result;
	}

	ResourcePathTempl addPrefix(const std::string & prefix) const
	{
		ResourcePathTempl result;
		result.name = prefix + this->getName();
		result.originalName = prefix + this->getOriginalName();

		return result;
	}
};

using AnimationPath = ResourcePathTempl<EResType::ANIMATION>;
using ImagePath = ResourcePathTempl<EResType::IMAGE>;
using TextPath = ResourcePathTempl<EResType::TEXT>;
using JsonPath = ResourcePathTempl<EResType::JSON>;
using VideoPath = ResourcePathTempl<EResType::VIDEO>;
using AudioPath = ResourcePathTempl<EResType::SOUND>;

namespace EResTypeHelper
{
	/**
	 * Converts a extension string to a EResType enum object.
	 *
	 * @param extension The extension string e.g. .BMP, .PNG
	 * @return Returns a EResType enum object
	 */
	EResType getTypeFromExtension(std::string extension);

	/**
	 * Gets the EResType as a string representation.
	 *
	 * @param type the EResType
	 * @return the type as a string representation
	 */
	std::string getEResTypeAsString(EResType type);
};

VCMI_LIB_NAMESPACE_END

namespace std
{
template <> struct hash<VCMI_LIB_WRAP_NAMESPACE(ResourcePath)>
{
	size_t operator()(const VCMI_LIB_WRAP_NAMESPACE(ResourcePath) & resourceIdent) const
	{
		std::hash<int> intHasher;
		std::hash<std::string> stringHasher;
		return stringHasher(resourceIdent.getName()) ^ intHasher(static_cast<int>(resourceIdent.getType()));
	}
};
}
