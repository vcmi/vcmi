#include "StdInc.h"
#include "Filesystem.h"

#include "CFileInfo.h"

#include "CArchiveLoader.h"
#include "CFilesystemLoader.h"
#include "AdapterLoaders.h"

//For filesystem initialization
#include "../JsonNode.h"
#include "../GameConstants.h"
#include "../VCMIDirs.h"
#include "../CStopWatch.h"

CFilesystemList * CResourceHandler::resourceLoader = nullptr;
CFilesystemList * CResourceHandler::initialLoader = nullptr;

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

	// strangely enough but this line takes 40-50% of filesystem loading time
	boost::to_upper(this->name);
}

void ResourceID::setType(EResType::Type type)
{
	this->type = type;
}

void CResourceHandler::clear()
{
	delete resourceLoader;
	delete initialLoader;
}

EResType::Type EResTypeHelper::getTypeFromExtension(std::string extension)
{
	boost::to_upper(extension);

	static const std::map<std::string, EResType::Type> stringToRes =
			boost::assign::map_list_of
			(".TXT",   EResType::TEXT)
			(".JSON",  EResType::TEXT)
			(".DEF",   EResType::ANIMATION)
			(".MSK",   EResType::MASK)
			(".MSG",   EResType::MASK)
			(".H3C",   EResType::CAMPAIGN)
			(".H3M",   EResType::MAP)
			(".FNT",   EResType::BMP_FONT)
			(".TTF",   EResType::TTF_FONT)
			(".BMP",   EResType::IMAGE)
			(".JPG",   EResType::IMAGE)
			(".PCX",   EResType::IMAGE)
			(".PNG",   EResType::IMAGE)
			(".TGA",   EResType::IMAGE)
			(".WAV",   EResType::SOUND)
			(".82M",   EResType::SOUND)
			(".SMK",   EResType::VIDEO)
			(".BIK",   EResType::VIDEO)
			(".MJPG",  EResType::VIDEO)
			(".MPG",   EResType::VIDEO)
			(".AVI",   EResType::VIDEO)
			(".MP3",   EResType::MUSIC)
			(".OGG",   EResType::MUSIC)
			(".LOD",   EResType::ARCHIVE_LOD)
			(".PAC",   EResType::ARCHIVE_LOD)
			(".VID",   EResType::ARCHIVE_VID)
			(".SND",   EResType::ARCHIVE_SND)
			(".PAL",   EResType::PALETTE)
			(".VCGM1", EResType::CLIENT_SAVEGAME)
			(".VSGM1", EResType::SERVER_SAVEGAME)
			(".ERM",   EResType::ERM)
			(".ERT",   EResType::ERT)
			(".ERS",   EResType::ERS);

	auto iter = stringToRes.find(extension);
	if (iter == stringToRes.end())
		return EResType::OTHER;
	return iter->second;
}

std::string EResTypeHelper::getEResTypeAsString(EResType::Type type)
{
#define MAP_ENUM(value) (EResType::value, #value)

	static const std::map<EResType::Type, std::string> stringToRes = boost::assign::map_list_of
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
		MAP_ENUM(OTHER);

#undef MAP_ENUM

	auto iter = stringToRes.find(type);
	assert(iter != stringToRes.end());

	return iter->second;
}

void CResourceHandler::initialize()
{
	//recurse only into specific directories
	auto recurseInDir = [](std::string URI, int depth)
	{
		ResourceID ID(URI, EResType::DIRECTORY);

		for(auto & loader : initialLoader->getResourcesWithName(ID))
		{
			auto filename = loader->getResourceName(ID);
			if (filename)
			{
				auto dir = new CFilesystemLoader(URI + "/", *filename, depth, true);
				initialLoader->addLoader(dir, false);
			}
		}
	};

	//temporary filesystem that will be used to initialize main one.
	//used to solve several case-sensivity issues like Mp3 vs MP3
	initialLoader = new CFilesystemList;
	resourceLoader = new CFilesystemList;

	for (auto & path : VCMIDirs::get().dataPaths())
		initialLoader->addLoader(new CFilesystemLoader("", path, 0, true), false);

	if (VCMIDirs::get().dataPaths().back() != VCMIDirs::get().userDataPath())
		initialLoader->addLoader(new CFilesystemLoader("", VCMIDirs::get().userDataPath(), 0, true), false);

	recurseInDir("CONFIG", 0);// look for configs
	recurseInDir("DATA", 0); // look for archives
	//TODO: improve mod loading process so depth 2 will no longer be needed
	recurseInDir("MODS", 2); // look for mods. Depth 2 is required for now but won't cause speed issues if no mods present
}

void CResourceHandler::loadDirectory(const std::string &prefix, const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	bool writeable = config["writeable"].Bool();
	int depth = 16;
	if (!config["depth"].isNull())
		depth = config["depth"].Float();

	ResourceID resID(URI, EResType::DIRECTORY);

	for(auto & loader : initialLoader->getResourcesWithName(resID))
	{
		auto filename = loader->getResourceName(resID);
		resourceLoader->addLoader(new CFilesystemLoader(mountPoint, *filename, depth), writeable);
	}
}

void CResourceHandler::loadArchive(const std::string &prefix, const std::string &mountPoint, const JsonNode & config, EResType::Type archiveType)
{
	std::string URI = prefix + config["path"].String();
	auto filename = initialLoader->getResourceName(ResourceID(URI, archiveType));
	if (filename)
		resourceLoader->addLoader(new CArchiveLoader(mountPoint, *filename), false);
}

void CResourceHandler::loadJsonMap(const std::string &prefix, const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = initialLoader->getResourceName(ResourceID(URI, EResType::TEXT));
	if (filename)
	{
		auto configData = initialLoader->load(ResourceID(URI, EResType::TEXT))->readAll();
		const JsonNode config((char*)configData.first.get(), configData.second);
		resourceLoader->addLoader(new CMappedFileLoader(mountPoint, config), false);
	}
}

void CResourceHandler::loadMainFileSystem(const std::string &fsConfigURI)
{
	auto fsConfigData = initialLoader->load(ResourceID(fsConfigURI, EResType::TEXT))->readAll();

	const JsonNode fsConfig((char*)fsConfigData.first.get(), fsConfigData.second);

	loadModFileSystem("", fsConfig["filesystem"]);

	// hardcoded system-specific path, may not be inside any of data directories
	resourceLoader->addLoader(new CFilesystemLoader("SAVES/", VCMIDirs::get().userSavePath()), true);
	resourceLoader->addLoader(new CFilesystemLoader("CONFIG/", VCMIDirs::get().userConfigPath()), true);
}

void CResourceHandler::loadModFileSystem(const std::string & prefix, const JsonNode &fsConfig)
{
	for(auto & mountPoint : fsConfig.Struct())
	{
		for(auto & entry : mountPoint.second.Vector())
		{
			CStopWatch timer;
			logGlobal->debugStream() << "\t\tLoading resource at " << prefix + entry["path"].String();

			//TODO: replace if's with string->functor map?
			if (entry["type"].String() == "map")
				loadJsonMap(prefix, mountPoint.first, entry);
			if (entry["type"].String() == "dir")
				loadDirectory(prefix, mountPoint.first, entry);
			if (entry["type"].String() == "lod")
				loadArchive(prefix, mountPoint.first, entry, EResType::ARCHIVE_LOD);
			if (entry["type"].String() == "snd")
				loadArchive(prefix, mountPoint.first, entry, EResType::ARCHIVE_SND);
			if (entry["type"].String() == "vid")
				loadArchive(prefix, mountPoint.first, entry, EResType::ARCHIVE_VID);

			logGlobal->debugStream() << "Resource loaded in " << timer.getDiff() << " ms.";
		}
	}
}

std::vector<std::string> CResourceHandler::getAvailableMods()
{
	static const std::string modDir = "MODS/";

	auto list = initialLoader->getFilteredFiles([](const ResourceID & id) ->  bool
	{
		return id.getType() == EResType::DIRECTORY
			&& boost::range::count(id.getName(), '/') == 1
			&& boost::algorithm::starts_with(id.getName(), modDir);
	});

	//storage for found mods
	std::vector<std::string> foundMods;
	for (auto & entry : list)
	{
		std::string name = entry.getName();

		name.erase(0, modDir.size()); //Remove path prefix

		if (name == "WOG") // check if wog is actually present. Hack-ish but better than crash
		{
			if (!initialLoader->existsResource(ResourceID("DATA/ZVS", EResType::DIRECTORY)) &&
				!initialLoader->existsResource(ResourceID("MODS/WOG/DATA/ZVS", EResType::DIRECTORY)))
			{
				continue;
			}
		}
		foundMods.push_back(name);
	}
	return foundMods;
}

void CResourceHandler::setActiveMods(std::vector<std::string> enabledMods)
{
	// default FS config for mods: directory "Content" that acts as H3 root directory
	JsonNode defaultFS;

	defaultFS[""].Vector().resize(1);
	defaultFS[""].Vector()[0]["type"].String() = "dir";
	defaultFS[""].Vector()[0]["path"].String() = "/Content";

	for(std::string & modName : enabledMods)
	{
		ResourceID modConfFile("mods/" + modName + "/mod", EResType::TEXT);
		auto fsConfigData = initialLoader->load(modConfFile)->readAll();
		const JsonNode fsConfig((char*)fsConfigData.first.get(), fsConfigData.second);

		if (!fsConfig["filesystem"].isNull())
			loadModFileSystem("mods/" + modName, fsConfig["filesystem"]);
		else
			loadModFileSystem("mods/" + modName, defaultFS);
	}
}
