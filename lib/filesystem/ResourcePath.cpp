/*
 * ResourcePath.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ResourcePath.h"
#include "FileInfo.h"

#include "../JsonNode.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

static inline void toUpper(std::string & string)
{
	boost::to_upper(string);
}

static inline EResType readType(const std::string& name)
{
	return EResTypeHelper::getTypeFromExtension(FileInfo::GetExtension(name).to_string());
}

static inline std::string readName(std::string name, bool uppercase)
{
	const auto dotPos = name.find_last_of('.');

	//do not cut "extension" of directory name
	auto delimPos = name.find_last_of('/');
	if(delimPos == std::string::npos)
		delimPos = name.find_last_of('\\');

	if((delimPos == std::string::npos || delimPos < dotPos) && dotPos != std::string::npos)
	{
		auto type = EResTypeHelper::getTypeFromExtension(name.substr(dotPos));
		if(type != EResType::OTHER)
			name.resize(dotPos);
	}

	if(uppercase)
		toUpper(name);

	return name;
}

ResourcePath::ResourcePath(const std::string & name_):
	type(readType(name_)),
	name(readName(name_, true)),
	originalName(readName(name_, false))
{}

ResourcePath::ResourcePath(const std::string & name_, EResType type_):
	type(type_),
	name(readName(name_, true)),
	originalName(readName(name_, false))
{}

ResourcePath::ResourcePath(const JsonNode & name, EResType type):
	type(type),
	name(readName(name.String(), true)),
	originalName(readName(name.String(), false))
{
}

void ResourcePath::serializeJson(JsonSerializeFormat & handler)
{
	if (!handler.saving)
	{
		JsonNode const & node = handler.getCurrent();

		if (node.isString())
		{
			name = readName(node.String(), true);
			originalName = readName(node.String(), false);
			return;
		}
	}

	handler.serializeInt("type", type);
	handler.serializeString("name", name);
	handler.serializeString("originalName", originalName);
}

EResType EResTypeHelper::getTypeFromExtension(std::string extension)
{
	toUpper(extension);

	static const std::map<std::string, EResType> stringToRes =
	{
		{".TXT",   EResType::TEXT},
		{".JSON",  EResType::JSON},
		{".DEF",   EResType::ANIMATION},
		{".MSK",   EResType::MASK},
		{".MSG",   EResType::MASK},
		{".H3C",   EResType::CAMPAIGN},
		{".H3M",   EResType::MAP},
		{".TUT",   EResType::MAP},
		{".FNT",   EResType::BMP_FONT},
		{".TTF",   EResType::TTF_FONT},
		{".BMP",   EResType::IMAGE},
		{".GIF",   EResType::IMAGE},
		{".JPG",   EResType::IMAGE},
		{".PCX",   EResType::IMAGE},
		{".PNG",   EResType::IMAGE},
		{".TGA",   EResType::IMAGE},
		{".WAV",   EResType::SOUND},
		{".82M",   EResType::SOUND},
		{".MP3",   EResType::SOUND},
		{".OGG",   EResType::SOUND},
		{".FLAC",  EResType::SOUND},
		{".SMK",   EResType::VIDEO},
		{".BIK",   EResType::VIDEO},
		{".MJPG",  EResType::VIDEO},
		{".MPG",   EResType::VIDEO},
		{".AVI",   EResType::VIDEO},
		{".ZIP",   EResType::ARCHIVE_ZIP},
		{".LOD",   EResType::ARCHIVE_LOD},
		{".PAC",   EResType::ARCHIVE_LOD},
		{".VID",   EResType::ARCHIVE_VID},
		{".SND",   EResType::ARCHIVE_SND},
		{".PAL",   EResType::PALETTE},
		{".VSGM1", EResType::SAVEGAME},
		{".ERM",   EResType::ERM},
		{".ERT",   EResType::ERT},
		{".ERS",   EResType::ERS},
		{".VMAP",  EResType::MAP},
		{".VCMP",  EResType::CAMPAIGN},
		{".VERM",  EResType::ERM},
		{".LUA",   EResType::LUA}
	};

	auto iter = stringToRes.find(extension);
	if (iter == stringToRes.end())
		return EResType::OTHER;
	return iter->second;
}

std::string EResTypeHelper::getEResTypeAsString(EResType type)
{
#define MAP_ENUM(value) {EResType::value, #value},

	static const std::map<EResType, std::string> stringToRes =
	{
		MAP_ENUM(TEXT)
		MAP_ENUM(JSON)
		MAP_ENUM(ANIMATION)
		MAP_ENUM(MASK)
		MAP_ENUM(CAMPAIGN)
		MAP_ENUM(MAP)
		MAP_ENUM(BMP_FONT)
		MAP_ENUM(TTF_FONT)
		MAP_ENUM(IMAGE)
		MAP_ENUM(VIDEO)
		MAP_ENUM(SOUND)
		MAP_ENUM(ARCHIVE_ZIP)
		MAP_ENUM(ARCHIVE_LOD)
		MAP_ENUM(ARCHIVE_SND)
		MAP_ENUM(ARCHIVE_VID)
		MAP_ENUM(PALETTE)
		MAP_ENUM(SAVEGAME)
		MAP_ENUM(DIRECTORY)
		MAP_ENUM(ERM)
		MAP_ENUM(ERT)
		MAP_ENUM(ERS)
		MAP_ENUM(OTHER)
	};

#undef MAP_ENUM

	auto iter = stringToRes.find(type);
	assert(iter != stringToRes.end());

	return iter->second;
}

VCMI_LIB_NAMESPACE_END
