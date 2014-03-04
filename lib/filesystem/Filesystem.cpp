#include "StdInc.h"
#include "Filesystem.h"

#include "CFileInfo.h"

#include "CArchiveLoader.h"
#include "CFilesystemLoader.h"
#include "AdapterLoaders.h"
#include "CZipLoader.h"

//For filesystem initialization
#include "../JsonNode.h"
#include "../GameConstants.h"
#include "../VCMIDirs.h"
#include "../CStopWatch.h"

CFilesystemList * CResourceHandler::resourceLoader = nullptr;
CFilesystemList * CResourceHandler::initialLoader = nullptr;
std::map<std::string, ISimpleResourceLoader*> CResourceHandler::knownLoaders = std::map<std::string, ISimpleResourceLoader*>();

CFilesystemGenerator::CFilesystemGenerator(std::string prefix):
	filesystem(new CFilesystemList()),
	prefix(prefix)
{
}

CFilesystemGenerator::TLoadFunctorMap CFilesystemGenerator::genFunctorMap()
{
	TLoadFunctorMap map;
	map["map"] = boost::bind(&CFilesystemGenerator::loadJsonMap, this, _1, _2);
	map["dir"] = boost::bind(&CFilesystemGenerator::loadDirectory, this, _1, _2);
	map["lod"] = boost::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_LOD>, this, _1, _2);
	map["snd"] = boost::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_SND>, this, _1, _2);
	map["vid"] = boost::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_VID>, this, _1, _2);
	map["zip"] = boost::bind(&CFilesystemGenerator::loadZipArchive, this, _1, _2);
	return map;
}

void CFilesystemGenerator::loadConfig(const JsonNode & config)
{
	for(auto & mountPoint : config.Struct())
	{
		for(auto & entry : mountPoint.second.Vector())
		{
			CStopWatch timer;
			logGlobal->debugStream() << "\t\tLoading resource at " << prefix + entry["path"].String();

			auto map = genFunctorMap();
			auto functor = map.find(entry["type"].String());

			if (functor != map.end())
			{
				functor->second(mountPoint.first, entry);
				logGlobal->debugStream() << "Resource loaded in " << timer.getDiff() << " ms.";
			}
			else
			{
				logGlobal->errorStream() << "Unknown filesystem format: " << functor->first;
			}
		}
	}
}

CFilesystemList * CFilesystemGenerator::getFilesystem()
{
	return filesystem;
}

void CFilesystemGenerator::loadDirectory(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	int depth = 16;
	if (!config["depth"].isNull())
		depth = config["depth"].Float();

	ResourceID resID(URI, EResType::DIRECTORY);

	for(auto & loader : CResourceHandler::getInitial()->getResourcesWithName(resID))
	{
		auto filename = loader->getResourceName(resID);
		filesystem->addLoader(new CFilesystemLoader(mountPoint, *filename, depth), false);
	}
}

void CFilesystemGenerator::loadZipArchive(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::getInitial()->getResourceName(ResourceID(URI, EResType::ARCHIVE_ZIP));
	if (filename)
		filesystem->addLoader(new CZipLoader(mountPoint, *filename), false);
}

template<EResType::Type archiveType>
void CFilesystemGenerator::loadArchive(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::getInitial()->getResourceName(ResourceID(URI, archiveType));
	if (filename)
		filesystem->addLoader(new CArchiveLoader(mountPoint, *filename), false);
}

void CFilesystemGenerator::loadJsonMap(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::getInitial()->getResourceName(ResourceID(URI, EResType::TEXT));
	if (filename)
	{
		auto configData = CResourceHandler::getInitial()->load(ResourceID(URI, EResType::TEXT))->readAll();
		const JsonNode config((char*)configData.first.get(), configData.second);
		filesystem->addLoader(new CMappedFileLoader(mountPoint, config), false);
	}
}

void CResourceHandler::clear()
{
	delete resourceLoader;
	delete initialLoader;
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

	for (auto & path : VCMIDirs::get().dataPaths())
	{
		if (boost::filesystem::is_directory(path)) // some of system-provided paths may not exist
			initialLoader->addLoader(new CFilesystemLoader("", path, 0, true), false);
	}
	initialLoader->addLoader(new CFilesystemLoader("", VCMIDirs::get().userDataPath(), 0, true), false);

	recurseInDir("CONFIG", 0);// look for configs
	recurseInDir("DATA", 0); // look for archives
	//TODO: improve mod loading process so depth 2 will no longer be needed
	recurseInDir("MODS", 2); // look for mods. Depth 2 is required for now but won't cause speed issues if no mods present
}

ISimpleResourceLoader * CResourceHandler::get()
{
	return get("");
}

ISimpleResourceLoader * CResourceHandler::get(std::string identifier)
{
	return knownLoaders.at(identifier);
}

ISimpleResourceLoader * CResourceHandler::getInitial()
{
	assert(initialLoader);
	return initialLoader;
}

void CResourceHandler::load(const std::string &fsConfigURI)
{
	auto fsConfigData = initialLoader->load(ResourceID(fsConfigURI, EResType::TEXT))->readAll();

	const JsonNode fsConfig((char*)fsConfigData.first.get(), fsConfigData.second);

	resourceLoader = new CFilesystemList();
	knownLoaders[""] = resourceLoader;
	addFilesystem("core", createFileSystem("", fsConfig["filesystem"]));

	// hardcoded system-specific path, may not be inside any of data directories
	resourceLoader->addLoader(new CFilesystemLoader("SAVES/", VCMIDirs::get().userSavePath()), true);
	resourceLoader->addLoader(new CFilesystemLoader("CONFIG/", VCMIDirs::get().userConfigPath()), true);
}

void CResourceHandler::addFilesystem(const std::string & identifier, ISimpleResourceLoader * loader)
{
	assert(knownLoaders.count(identifier) == 0);
	resourceLoader->addLoader(loader, false);
	knownLoaders[identifier] = loader;
}

ISimpleResourceLoader * CResourceHandler::createFileSystem(const std::string & prefix, const JsonNode &fsConfig)
{
	CFilesystemGenerator generator(prefix);
	generator.loadConfig(fsConfig);
	return generator.getFilesystem();
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

		if (!name.empty())
			foundMods.push_back(name);
	}
	return foundMods;
}
