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
#include "../JsonNode.h"
#include "../GameConstants.h"
#include "../VCMIDirs.h"
#include "../CStopWatch.h"

std::map<std::string, ISimpleResourceLoader *> CResourceHandler::knownLoaders = std::map<std::string, ISimpleResourceLoader *>();

CFilesystemGenerator::CFilesystemGenerator(std::string prefix)
	: filesystem(new CFilesystemList()), prefix(prefix)
{
}

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
	for(auto & mountPoint : config.Struct())
	{
		for(auto & entry : mountPoint.second.Vector())
		{
			CStopWatch timer;
			logGlobal->debugStream() << "\t\tLoading resource at " << prefix + entry["path"].String();

			auto map = genFunctorMap();
			auto typeName = entry["type"].String();
			auto functor = map.find(typeName);

			if(functor != map.end())
			{
				functor->second(mountPoint.first, entry);
				logGlobal->debugStream() << "Resource loaded in " << timer.getDiff() << " ms.";
			}
			else
			{
				logGlobal->error("Unknown filesystem format: %s", typeName);
			}
		}
	}
}

CFilesystemList * CFilesystemGenerator::getFilesystem()
{
	return filesystem;
}

void CFilesystemGenerator::loadDirectory(const std::string & mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	int depth = 16;
	if(!config["depth"].isNull())
		depth = (int)config["depth"].Float();

	ResourceID resID(URI, EResType::DIRECTORY);

	for(auto & loader : CResourceHandler::get("initial")->getResourcesWithName(resID))
	{
		auto filename = loader->getResourceName(resID);
		filesystem->addLoader(new CFilesystemLoader(mountPoint, *filename, depth), false);
	}
}

void CFilesystemGenerator::loadZipArchive(const std::string & mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(ResourceID(URI, EResType::ARCHIVE_ZIP));
	if(filename)
		filesystem->addLoader(new CZipLoader(mountPoint, *filename), false);
}

template<EResType::Type archiveType>
void CFilesystemGenerator::loadArchive(const std::string & mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(ResourceID(URI, archiveType));
	if(filename)
		filesystem->addLoader(new CArchiveLoader(mountPoint, *filename), false);
}

void CFilesystemGenerator::loadJsonMap(const std::string & mountPoint, const JsonNode & config)
{
	std::string URI = prefix + config["path"].String();
	auto filename = CResourceHandler::get("initial")->getResourceName(ResourceID(URI, EResType::TEXT));
	if(filename)
	{
		auto configData = CResourceHandler::get("initial")->load(ResourceID(URI, EResType::TEXT))->readAll();
		const JsonNode config((char *)configData.first.get(), configData.second);
		filesystem->addLoader(new CMappedFileLoader(mountPoint, config), false);
	}
}

void CResourceHandler::clear()
{
	delete knownLoaders["root"];
}

ISimpleResourceLoader * CResourceHandler::createInitial()
{
	//temporary filesystem that will be used to initialize main one.
	//used to solve several case-sensivity issues like Mp3 vs MP3
	auto initialLoader = new CFilesystemList();

	//recurse only into specific directories
	auto recurseInDir = [&](std::string URI, int depth)
		{
			ResourceID ID(URI, EResType::DIRECTORY);

			for(auto & loader : initialLoader->getResourcesWithName(ID))
			{
				auto filename = loader->getResourceName(ID);
				if(filename)
				{
					auto dir = new CFilesystemLoader(URI + '/', *filename, depth, true);
					initialLoader->addLoader(dir, false);
				}
			}
		};

	for(auto & path : VCMIDirs::get().dataPaths())
	{
		if(boost::filesystem::is_directory(path)) // some of system-provided paths may not exist
			initialLoader->addLoader(new CFilesystemLoader("", path, 0, true), false);
	}
	initialLoader->addLoader(new CFilesystemLoader("", VCMIDirs::get().userDataPath(), 0, true), false);

	recurseInDir("CONFIG", 0); // look for configs
	recurseInDir("DATA", 0); // look for archives
	recurseInDir("MODS", 64); // look for mods.

	return initialLoader;
}

void CResourceHandler::initialize()
{
	// Create tree-loke structure that looks like this:
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

	knownLoaders["root"] = new CFilesystemList();
	knownLoaders["saves"] = new CFilesystemLoader("SAVES/", VCMIDirs::get().userSavePath());
	knownLoaders["config"] = new CFilesystemLoader("CONFIG/", VCMIDirs::get().userConfigPath());

	auto localFS = new CFilesystemList();
	localFS->addLoader(knownLoaders["saves"], true);
	localFS->addLoader(knownLoaders["config"], true);

	addFilesystem("root", "initial", createInitial());
	addFilesystem("root", "data", new CFilesystemList());
	addFilesystem("root", "local", localFS);
}

ISimpleResourceLoader * CResourceHandler::get()
{
	return get("root");
}

ISimpleResourceLoader * CResourceHandler::get(std::string identifier)
{
	return knownLoaders.at(identifier);
}

void CResourceHandler::load(const std::string & fsConfigURI)
{
	auto fsConfigData = get("initial")->load(ResourceID(fsConfigURI, EResType::TEXT))->readAll();

	const JsonNode fsConfig((char *)fsConfigData.first.get(), fsConfigData.second);

	addFilesystem("data", "core", createFileSystem("", fsConfig["filesystem"]));
}

void CResourceHandler::addFilesystem(const std::string & parent, const std::string & identifier, ISimpleResourceLoader * loader)
{
	assert(knownLoaders.count(identifier) == 0);

	auto list = dynamic_cast<CFilesystemList *>(knownLoaders.at(parent));
	assert(list);
	list->addLoader(loader, false);
	knownLoaders[identifier] = loader;
}

ISimpleResourceLoader * CResourceHandler::createFileSystem(const std::string & prefix, const JsonNode & fsConfig)
{
	CFilesystemGenerator generator(prefix);
	generator.loadConfig(fsConfig);
	return generator.getFilesystem();
}
