#include "StdInc.h"
#include "ResourceID.h"

#include "CFileInfo.h"

// trivial to_upper that completely ignores localization and only work with ASCII
// Technically not a problem since
// 1) Right now VCMI does not supports unicode in filenames on Win
// 2) Filesystem case-sensivity is only problem for H3 data which uses ASCII-only symbols
// for me (Ivan) this define gives notable decrease in loading times
// #define ENABLE_TRIVIAL_TOUPPER

#ifdef ENABLE_TRIVIAL_TOUPPER
static inline void toUpper(char & symbol)
{
	static const int diff = 'a' - 'A';
	if (symbol >= 'a' && symbol <= 'z')
		symbol -= diff;
}

static inline void toUpper(std::string & string)
{
	for (char & symbol : string)
		toUpper(symbol);
}

#endif


ResourceID::ResourceID()
	:type(EResType::OTHER)
{
}

ResourceID::ResourceID(std::string name)
{
	CFileInfo info(std::move(name));
	setName(info.getStem());
	setType(info.getType());
}

ResourceID::ResourceID(std::string name, EResType::Type type)
{
	setName(std::move(name));
	setType(type);
}

std::string ResourceID::getName() const
{
	return name;
}

EResType::Type ResourceID::getType() const
{
	return type;
}

void ResourceID::setName(std::string name)
{
	this->name = std::move(name);

	size_t dotPos = this->name.find_last_of("/.");

	if(dotPos != std::string::npos && this->name[dotPos] == '.')
		this->name.erase(dotPos);

#ifdef ENABLE_TRIVIAL_TOUPPER
	toUpper(this->name);
#else
	// strangely enough but this line takes 40-50% of filesystem loading time
	boost::to_upper(this->name);
#endif
}

void ResourceID::setType(EResType::Type type)
{
	this->type = type;
}

EResType::Type EResTypeHelper::getTypeFromExtension(std::string extension)
{
#ifdef ENABLE_TRIVIAL_TOUPPER
	toUpper(extension);
#else
	boost::to_upper(extension);
#endif

	static const std::map<std::string, EResType::Type> stringToRes =
	{
		{".TXT",   EResType::TEXT},
		{".JSON",  EResType::TEXT},
		{".DEF",   EResType::ANIMATION},
		{".MSK",   EResType::MASK},
		{".MSG",   EResType::MASK},
		{".H3C",   EResType::CAMPAIGN},
		{".H3M",   EResType::MAP},
		{".FNT",   EResType::BMP_FONT},
		{".TTF",   EResType::TTF_FONT},
		{".BMP",   EResType::IMAGE},
		{".JPG",   EResType::IMAGE},
		{".PCX",   EResType::IMAGE},
		{".PNG",   EResType::IMAGE},
		{".TGA",   EResType::IMAGE},
		{".WAV",   EResType::SOUND},
		{".82M",   EResType::SOUND},
		{".SMK",   EResType::VIDEO},
		{".BIK",   EResType::VIDEO},
		{".MJPG",  EResType::VIDEO},
		{".MPG",   EResType::VIDEO},
		{".AVI",   EResType::VIDEO},
		{".MP3",   EResType::MUSIC},
		{".OGG",   EResType::MUSIC},
		{".FLAC",  EResType::MUSIC},
		{".ZIP",   EResType::ARCHIVE_ZIP},
		{".LOD",   EResType::ARCHIVE_LOD},
		{".PAC",   EResType::ARCHIVE_LOD},
		{".VID",   EResType::ARCHIVE_VID},
		{".SND",   EResType::ARCHIVE_SND},
		{".PAL",   EResType::PALETTE},
		{".VCGM1", EResType::CLIENT_SAVEGAME},
		{".VSGM1", EResType::SERVER_SAVEGAME},
		{".ERM",   EResType::ERM},
		{".ERT",   EResType::ERT},
		{".ERS",   EResType::ERS}
	};

	auto iter = stringToRes.find(extension);
	if (iter == stringToRes.end())
		return EResType::OTHER;
	return iter->second;
}

std::string EResTypeHelper::getEResTypeAsString(EResType::Type type)
{
#define MAP_ENUM(value) {EResType::value, #value},

	static const std::map<EResType::Type, std::string> stringToRes =
	{
		MAP_ENUM(TEXT)
		MAP_ENUM(ANIMATION)
		MAP_ENUM(MASK)
		MAP_ENUM(CAMPAIGN)
		MAP_ENUM(MAP)
		MAP_ENUM(BMP_FONT)
		MAP_ENUM(TTF_FONT)
		MAP_ENUM(IMAGE)
		MAP_ENUM(VIDEO)
		MAP_ENUM(SOUND)
		MAP_ENUM(MUSIC)
		MAP_ENUM(ARCHIVE_ZIP)
		MAP_ENUM(ARCHIVE_LOD)
		MAP_ENUM(ARCHIVE_SND)
		MAP_ENUM(ARCHIVE_VID)
		MAP_ENUM(PALETTE)
		MAP_ENUM(CLIENT_SAVEGAME)
		MAP_ENUM(SERVER_SAVEGAME)
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
