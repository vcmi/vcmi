/*
 * Filesystem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Filesystem.h"

#include "CArchiveLoader.h"
#include "CFilesystemLoader.h"
#include "AdapterLoaders.h"
#include "CZipLoader.h"

//For filesystem initialization
#include "../GameConstants.h"
#include "../VCMIDirs.h"
#include "../CStopWatch.h"
#include "../json/JsonNode.h"
#include "../modding/ModScope.h"

VCMI_LIB_NAMESPACE_BEGIN

std::map<std::string, ISimpleResourceLoader*> CResourceHandler::knownLoaders = std::map<std::string, ISimpleResourceLoader*>();
CResourceHandler CResourceHandler::globalResourceHandler;

CFilesystemGenerator::CFilesystemGenerator(std::string prefix, bool extractArchives):
	filesystem(std::make_unique<CFilesystemList>()),
	prefix(std::move(prefix)),
	extractArchives(extractArchives)
{
}

CFilesystemGenerator::~CFilesystemGenerator() = default;

CFilesystemGenerator::TLoadFunctorMap CFilesystemGenerator::genFunctorMap()
{
	TLoadFunctorMap map;
	map["map"] = std::bind(&CFilesystemGenerator::loadJsonMap, this, _1, _2);
	map["dir"] = std::bind(&CFilesystemGenerator::loadDirectory, this, _1, _2);
	map["lod"] = std::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_LOD>, this, _1, _2);
	map["snd"] = std::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_SND>, this, _1, _2);
	map["vid"] = std::bind(&CFilesystemGenerator::loadArchive<EResType::ARCHIVE_VID>, this, _1, _2);
	map["zip"] = std::bind(&CFilesystemGenerator::loadZipArchive, this, _1, _2);
	return map;
}

void CFilesystemGenerator::loadConfig(const JsonNode & config)
{
	for(const auto & mountPoint : config.Struct())
	{
		for(const auto & entry : mountPoint.second.Vector())
		{
			CStopWatch timer;
			logGlobal->trace("\t\tLoading resource at %s%s", prefix, entry["path"].String());

			auto map = genFunctorMap();
			auto typeName = entry["type"].String();
			auto functor = map.find(typeName);

			if (functor != map.end())
			{
				functor->second(mountPoint.first, entry);
				logGlobal->trace("Resource loaded in %d ms", timer.getDiff());
			}
			else
			{
				logGlobal->error("Unknown filesystem format: %s", typeName);
			}
		}
	}
}

std::unique_ptr<CFilesystemList> CFilesystemGenerator::acquireFilesystem()
{
	return std::move(filesystem);
}

void CFilesystemGenerator::loadDirectory(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	int depth = 16;
	if (!config["depth"].isNull())
		depth = static_cast<int>(config["depth"].Float());

	ResourcePath resID(URI, EResType::DIRECTORY);

	for(auto & loader : CResourceHandler::get("initial")->getResourcesWithName(resID))
	{
		auto filename = loader->getResourceName(resID);
		filesystem->addLoader(std::make_unique<CFilesystemLoader>(mountPoint, *filename, depth), false);
	}
}

void CFilesystemGenerator::loadZipArchive(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(ResourcePath(URI, EResType::ARCHIVE_ZIP));
	if (filename)
		filesystem->addLoader(std::make_unique<CZipLoader>(mountPoint, *filename), false);
}

template<EResType archiveType>
void CFilesystemGenerator::loadArchive(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(ResourcePath(URI, archiveType));
	if (filename)
		filesystem->addLoader(std::make_unique<CArchiveLoader>(mountPoint, *filename, extractArchives), false);
}

void CFilesystemGenerator::loadJsonMap(const std::string &mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(JsonPath::builtin(URI));
	if (filename)
	{
		auto configData = CResourceHandler::get("initial")->load(JsonPath::builtin(URI))->readAll();
		const JsonNode configInitial(reinterpret_cast<std::byte *>(configData.first.get()), configData.second, URI);
		filesystem->addLoader(std::make_unique<CMappedFileLoader>(mountPoint, configInitial), false);
	}
}

std::unique_ptr<ISimpleResourceLoader> CResourceHandler::createInitial()
{
	//temporary filesystem that will be used to initialize main one.
	//used to solve several case-sensivity issues like Mp3 vs MP3
	auto initialLoader = std::make_unique<CFilesystemList>();

	//recurse only into specific directories
	auto recurseInDir = [&](const std::string & URI, int depth)
	{
		ResourcePath ID(URI, EResType::DIRECTORY);

		for(auto & loader : initialLoader->getResourcesWithName(ID))
		{
			auto filename = loader->getResourceName(ID);
			if (filename)
			{
				auto dir = std::make_unique<CFilesystemLoader>(URI + '/', *filename, depth, true);
				initialLoader->addLoader(std::move(dir), false);
			}
		}
	};

	for (auto & path : VCMIDirs::get().dataPaths())
	{
		if (boost::filesystem::is_directory(path)) // some of system-provided paths may not exist
			initialLoader->addLoader(std::make_unique<CFilesystemLoader>("", path, 1, true), false);
	}
	initialLoader->addLoader(std::make_unique<CFilesystemLoader>("", VCMIDirs::get().userDataPath(), 0, true), false);

	recurseInDir("CONFIG", 0);// look for configs
	recurseInDir("DATA", 0); // look for archives
	recurseInDir("MODS", 64); // look for mods.

	return initialLoader;
}

void CResourceHandler::initialize()
{
	// Create tree-like structure that looks like this:
	// root
	// |
	// |- initial
	// |
	// |- data
	// |  |-core
	// |  |-mod1
	// |  |-modN
	// |
	// |- local
	//    |-saves
	//    |-config

	// when built as single process, server can be started multiple times
	if (globalResourceHandler.rootLoader)
		return;

	auto savesLoader = std::make_unique<CFilesystemLoader>("SAVES/", VCMIDirs::get().userSavePath());
	auto configLoader = std::make_unique<CFilesystemLoader>("CONFIG/", VCMIDirs::get().userConfigPath());

	globalResourceHandler.rootLoader = std::make_unique<CFilesystemList>();
	knownLoaders["root"] = globalResourceHandler.rootLoader.get();
	knownLoaders["saves"] = savesLoader.get();
	knownLoaders["config"] = configLoader.get();

	auto localFS = std::make_unique<CFilesystemList>();
	localFS->addLoader(std::move(savesLoader), true);
	localFS->addLoader(std::move(configLoader), true);

	addFilesystem("root", "initial", createInitial());
	addFilesystem("root", "data", std::make_unique<CFilesystemList>());
	addFilesystem("root", "local", std::move(localFS));
}

void CResourceHandler::destroy()
{
	knownLoaders.clear();
	globalResourceHandler.rootLoader.reset();
}

ISimpleResourceLoader * CResourceHandler::get()
{
	return get("root");
}

ISimpleResourceLoader * CResourceHandler::get(const std::string & identifier)
{
	assert(knownLoaders.count(identifier));
	return knownLoaders.at(identifier);
}

void CResourceHandler::load(const std::string &fsConfigURI, bool extractArchives)
{
	auto fsConfigData = get("initial")->load(JsonPath::builtin(fsConfigURI))->readAll();

	const JsonNode fsConfig(reinterpret_cast<std::byte *>(fsConfigData.first.get()), fsConfigData.second, fsConfigURI);

	addFilesystem("data", ModScope::scopeBuiltin(), createFileSystem("", fsConfig["filesystem"], extractArchives));
}

void CResourceHandler::addFilesystem(const std::string & parent, const std::string & identifier, std::unique_ptr<ISimpleResourceLoader> loader)
{
	if(knownLoaders.count(identifier) != 0)
	{
		logMod->error("[CRITICAL] Virtual filesystem %s already loaded!", identifier);
		return;
	}

	if(knownLoaders.count(parent) == 0)
	{
		logMod->error("[CRITICAL] Parent virtual filesystem %s for %s not found!", parent, identifier);
		return;
	}

	auto * list = dynamic_cast<CFilesystemList *>(knownLoaders.at(parent));
	assert(list);
	knownLoaders[identifier] = loader.get();
	list->addLoader(std::move(loader), false);
}

bool CResourceHandler::removeFilesystem(const std::string & parent, const std::string & identifier)
{
	if(knownLoaders.count(identifier) == 0)
		return false;
	
	if(knownLoaders.count(parent) == 0)
		return false;

	auto * list = dynamic_cast<CFilesystemList *>(knownLoaders.at(parent));
	assert(list);
	list->removeLoader(knownLoaders[identifier]);
	knownLoaders.erase(identifier);
	return true;
}

std::unique_ptr<ISimpleResourceLoader> CResourceHandler::createFileSystem(const std::string & prefix, const JsonNode &fsConfig, bool extractArchives)
{
	CFilesystemGenerator generator(prefix, extractArchives);
	generator.loadConfig(fsConfig);
	return generator.acquireFilesystem();
}

VCMI_LIB_NAMESPACE_END
