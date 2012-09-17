#include "StdInc.h"
#include "CResourceLoader.h"
#include "CFileInfo.h"
#include "CLodArchiveLoader.h"
#include "CFilesystemLoader.h"

//For filesystem initialization
#include "../JsonNode.h"
#include "../GameConstants.h"
#include "../VCMIDirs.h"
#include "../CStopWatch.h"

CResourceLoader * CResourceHandler::resourceLoader = nullptr;
CResourceLoader * CResourceHandler::initialLoader = nullptr;

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

ResourceID::ResourceID(const std::string & prefix, const std::string & name, EResType::Type type)
{
	this->name = name;

	size_t dotPos = this->name.find_last_of("/.");

	if(dotPos != std::string::npos && this->name[dotPos] == '.')
		this->name.erase(dotPos);

	this->name = prefix + this->name;
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

CResourceLoader::CResourceLoader()
{
}

std::unique_ptr<CInputStream> CResourceLoader::load(const ResourceID & resourceIdent) const
{
	auto resource = resources.find(resourceIdent);

	if(resource == resources.end())
	{
		throw std::runtime_error("Resource with name " + resourceIdent.getName() + " and type "
			+ EResTypeHelper::getEResTypeAsString(resourceIdent.getType()) + " wasn't found.");
	}

	// get the last added resource(most overriden)
	const ResourceLocator & locator = resource->second.back();

	// load the resource and return it
	return locator.getLoader()->load(locator.getResourceName());
}

std::pair<std::unique_ptr<ui8[]>, ui64> CResourceLoader::loadData(const ResourceID & resourceIdent) const
{
	auto stream = load(resourceIdent);
	std::unique_ptr<ui8[]> data(new ui8[stream->getSize()]);
	size_t readSize = stream->read(data.get(), stream->getSize());

	assert(readSize == stream->getSize());
	return std::make_pair(std::move(data), stream->getSize());
}

ResourceLocator CResourceLoader::getResource(const ResourceID & resourceIdent) const
{
	auto resource = resources.find(resourceIdent);

	if (resource == resources.end())
		return ResourceLocator(nullptr, "");
	return resource->second.back();
}

const std::vector<ResourceLocator> & CResourceLoader::getResourcesWithName(const ResourceID & resourceIdent) const
{
	static const std::vector<ResourceLocator> emptyList;
	auto resource = resources.find(resourceIdent);

	if (resource == resources.end())
		return emptyList;
	return resource->second;
}


std::string CResourceLoader::getResourceName(const ResourceID & resourceIdent) const
{
	auto locator = getResource(resourceIdent);
	if (locator.getLoader())
		return locator.getLoader()->getOrigin() + '/' + locator.getResourceName();
	return "";
}

bool CResourceLoader::existsResource(const ResourceID & resourceIdent) const
{
	return resources.find(resourceIdent) != resources.end();
}

bool CResourceLoader::createResource(std::string URI)
{
	std::string filename = URI;
	boost::to_upper(URI);
	BOOST_REVERSE_FOREACH (const LoaderEntry & entry, loaders)
	{
		if (entry.writeable && boost::algorithm::starts_with(URI, entry.prefix))
		{
			// remove loader prefix from filename
			filename = filename.substr(entry.prefix.size());
			if (!entry.loader->createEntry(filename))
				return false; //or continue loop?

			resources[ResourceID(URI)].push_back(ResourceLocator(entry.loader.get(), filename));
		}
	}
	return false;
}

void CResourceLoader::addLoader(std::string mountPoint, shared_ptr<ISimpleResourceLoader> loader, bool writeable)
{
	LoaderEntry loaderEntry;
	loaderEntry.loader = loader;
	loaderEntry.prefix = mountPoint;
	loaderEntry.writeable = writeable;
	loaders.push_back(loaderEntry);

	// Get entries and add them to the resources list
	const boost::unordered_map<ResourceID, std::string> & entries = loader->getEntries();

	boost::to_upper(mountPoint);

	BOOST_FOREACH (auto & entry, entries)
	{
		// Create identifier and locator and add them to the resources list
		ResourceID ident(mountPoint, entry.first.getName(), entry.first.getType());
		ResourceLocator locator(loader.get(), entry.second);

		if (ident.getType() == EResType::OTHER)
			tlog5 << "Warning: unknown file type: " << entry.second << "\n";

		resources[ident].push_back(locator);
	}
}

CResourceLoader * CResourceHandler::get()
{
	if(resourceLoader != nullptr)
	{
		return resourceLoader;
	}
	else
	{
		std::stringstream string;
		string << "Error: Resource loader wasn't initialized. "
			   << "Make sure that you set one via CResourceLoaderFactory::initialize";
		throw std::runtime_error(string.str());
	}
}

//void CResourceLoaderFactory::setInstance(CResourceLoader * resourceLoader)
//{
//	CResourceLoaderFactory::resourceLoader = resourceLoader;
//}

ResourceLocator::ResourceLocator(ISimpleResourceLoader * loader, const std::string & resourceName)
			: loader(loader), resourceName(resourceName)
{

}

ISimpleResourceLoader * ResourceLocator::getLoader() const
{
	return loader;
}

std::string ResourceLocator::getResourceName() const
{
	return resourceName;
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
	        (".FNT",   EResType::FONT)
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
	        (".MP3",   EResType::MUSIC)
	        (".OGG",   EResType::MUSIC)
	        (".LOD",   EResType::ARCHIVE_LOD)
	        (".PAC",   EResType::ARCHIVE_LOD)
	        (".VID",   EResType::ARCHIVE_VID)
	        (".SND",   EResType::ARCHIVE_SND)
	        (".PAL",   EResType::PALETTE)
	        (".VCGM1", EResType::CLIENT_SAVEGAME)
	        (".VLGM1", EResType::LIB_SAVEGAME)
	        (".VSGM1", EResType::SERVER_SAVEGAME);

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
		MAP_ENUM(FONT)
		MAP_ENUM(IMAGE)
		MAP_ENUM(VIDEO)
		MAP_ENUM(SOUND)
		MAP_ENUM(MUSIC)
		MAP_ENUM(ARCHIVE_LOD)
		MAP_ENUM(ARCHIVE_SND)
		MAP_ENUM(ARCHIVE_VID)
		MAP_ENUM(PALETTE)
		MAP_ENUM(CLIENT_SAVEGAME)
		MAP_ENUM(LIB_SAVEGAME)
		MAP_ENUM(SERVER_SAVEGAME)
		MAP_ENUM(DIRECTORY)
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
		auto resources = initialLoader->getResourcesWithName(ResourceID(URI, EResType::DIRECTORY));
		BOOST_FOREACH(const ResourceLocator & entry, resources)
		{
			std::string filename = entry.getLoader()->getOrigin() + '/' + entry.getResourceName();
			if (!filename.empty())
			{
				shared_ptr<ISimpleResourceLoader> dir(new CFilesystemLoader(filename, depth, true));
				initialLoader->addLoader(URI + '/', dir, false);
			}
		}
	};

	//temporary filesystem that will be used to initialize main one.
	//used to solve several case-sensivity issues like Mp3 vs MP3
	initialLoader = new CResourceLoader;
	resourceLoader = new CResourceLoader;

	shared_ptr<ISimpleResourceLoader> rootDir(new CFilesystemLoader(GameConstants::DATA_DIR, 0, true));
	initialLoader->addLoader("GLOBAL/", rootDir, false);
	initialLoader->addLoader("ALL/", rootDir, false);

	auto userDir = rootDir;

	//add local directory to "ALL" but only if it differs from root dir (true for linux)
	if (GameConstants::DATA_DIR != GVCMIDirs.UserPath)
	{
		userDir = shared_ptr<ISimpleResourceLoader>(new CFilesystemLoader(GVCMIDirs.UserPath, 0, true));
		initialLoader->addLoader("ALL/", userDir, false);
	}

	//create "LOCAL" dir with current userDir (may be same as rootDir)
	initialLoader->addLoader("LOCAL/", userDir, false);

	recurseInDir("ALL/CONFIG", 0);// look for configs
	recurseInDir("ALL/DATA", 0); // look for archives
	recurseInDir("ALL/MODS", 2); // look for mods. Depth 2 is required for now but won't cause issues if no mods present
}

void CResourceHandler::loadDirectory(const std::string mountPoint, const JsonNode & config)
{
	std::string URI = config["path"].String();
	bool writeable = config["writeable"].Bool();
	int depth = 16;
	if (!config["depth"].isNull())
		depth = config["depth"].Float();

	auto resources = initialLoader->getResourcesWithName(ResourceID(URI, EResType::DIRECTORY));

	BOOST_FOREACH(const ResourceLocator & entry, resources)
	{
		std::string filename = entry.getLoader()->getOrigin() + '/' + entry.getResourceName();
		resourceLoader->addLoader(mountPoint,
		    shared_ptr<ISimpleResourceLoader>(new CFilesystemLoader(filename, depth)), writeable);
	}
}

void CResourceHandler::loadArchive(const std::string mountPoint, const JsonNode & config, EResType::Type archiveType)
{
	std::string URI = config["path"].String();
	std::string filename = initialLoader->getResourceName(ResourceID(URI, archiveType));
	if (!filename.empty())
		resourceLoader->addLoader(mountPoint,
		    shared_ptr<ISimpleResourceLoader>(new CLodArchiveLoader(filename)), false);
}

void CResourceHandler::loadFileSystem(const std::string fsConfigURI)
{
	auto fsConfigData = initialLoader->loadData(ResourceID(fsConfigURI, EResType::TEXT));

	const JsonNode fsConfig((char*)fsConfigData.first.get(), fsConfigData.second);

	BOOST_FOREACH(auto & mountPoint, fsConfig["filesystem"].Struct())
	{
		BOOST_FOREACH(auto & entry, mountPoint.second.Vector())
		{
			CStopWatch timer;
			tlog5 << "\t\tLoading resource at " << entry["path"].String();

			if (entry["type"].String() == "dir")
				loadDirectory(mountPoint.first, entry);
			if (entry["type"].String() == "lod")
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_LOD);
			if (entry["type"].String() == "snd")
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_SND);
			if (entry["type"].String() == "vid")
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_VID);

			if (entry["type"].String() == "file") // for some compatibility, will be removed for 0.90
			{
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_LOD);
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_SND);
				loadArchive(mountPoint.first, entry, EResType::ARCHIVE_VID);
			}

			tlog5 << " took " << timer.getDiff() << " ms.\n";
		}
	}
}

void CResourceHandler::loadModsFilesystems()
{
	auto iterator = initialLoader->getIterator([](const ResourceID & ident) ->  bool
	{
		std::string name = ident.getName();

		return ident.getType() == EResType::TEXT
		    && std::count(name.begin(), name.end(), '/') == 3
		    && boost::algorithm::starts_with(name, "ALL/MODS/")
		    && boost::algorithm::ends_with(name, "FILESYSTEM");
	});

	//sorted storage for found mods
	//implements basic load order (entries in hashtable are basically random)
	std::set<std::string> foundMods;
	while (iterator.hasNext())
	{
		foundMods.insert(iterator->getName());
		++iterator;
	}

	BOOST_FOREACH(const std::string & entry, foundMods)
	{
		tlog1 << "\t\tFound mod filesystem: " << entry << "\n";
		loadFileSystem(entry);
	}
}
