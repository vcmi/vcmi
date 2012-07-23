#include "StdInc.h"
#include "CResourceLoader.h"
#include "CFileInfo.h"
#include "CLodArchiveLoader.h"

CResourceLoader * CResourceLoaderFactory::resourceLoader = nullptr;

ResourceIdentifier::ResourceIdentifier()
    :type(EResType::OTHER)
{
}

ResourceIdentifier::ResourceIdentifier(const std::string & name, EResType type) : name(name), type(type)
{
	boost::to_upper(this->name);
}

std::string ResourceIdentifier::getName() const
{
	return name;
}

EResType ResourceIdentifier::getType() const
{
	return type;
}

void ResourceIdentifier::setName(const std::string & name)
{
	this->name = name;
	boost::to_upper(this->name);
}

void ResourceIdentifier::setType(EResType type)
{
	this->type = type;
}

CResourceLoader::CResourceLoader()
{
}

CResourceLoader::~CResourceLoader()
{
	// Delete all loader objects
	BOOST_FOREACH ( ISimpleResourceLoader* it, loaders)
	{
		delete it;
	}
}

std::unique_ptr<CInputStream> CResourceLoader::load(const ResourceIdentifier & resourceIdent) const
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

bool CResourceLoader::existsResource(const ResourceIdentifier & resourceIdent) const
{
	// Check if resource is registered
	return resources.find(resourceIdent) != resources.end();
}

void CResourceLoader::addLoader(ISimpleResourceLoader * loader)
{
	loaders.push_back(loader);

	// Get entries and add them to the resources list
	const std::list<std::string> & entries = loader->getEntries();

	BOOST_FOREACH (const std::string & entry, entries)
	{
		CFileInfo file(entry);

		// Create identifier and locator and add them to the resources list
		ResourceIdentifier ident(file.getStem(), file.getType());
		ResourceLocator locator(loader, entry);
		resources[ident].push_back(locator);
	}
}

CResourceLoader * CResourceLoaderFactory::getInstance()
{
	if(resourceLoader != nullptr)
	{
		return resourceLoader;
	}
	else
	{
		std::stringstream string;
		string << "Error: Resource loader wasn't initialized. "
			   << "Make sure that you set one via CResourceLoaderFactory::setInstance";
		throw std::runtime_error(string.str());
	}
}

void CResourceLoaderFactory::setInstance(CResourceLoader * resourceLoader)
{
	CResourceLoaderFactory::resourceLoader = resourceLoader;
}

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

EResType EResTypeHelper::getTypeFromExtension(std::string extension)
{
	boost::to_upper(extension);

	static const std::map<std::string, EResType> stringToRes =
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
	        (".SMK",   EResType::VIDEO)
	        (".BIK",   EResType::VIDEO)
	        (".MJPG",  EResType::VIDEO)
	        (".MP3",   EResType::MUSIC)
	        (".OGG",   EResType::MUSIC)
	        (".LOD",   EResType::ARCHIVE)
	        (".VID",   EResType::ARCHIVE)
	        (".SND",   EResType::ARCHIVE)
	        (".VCGM1", EResType::CLIENT_SAVEGAME)
	        (".VLGM1", EResType::LIB_SAVEGAME)
	        (".VSGM1", EResType::SERVER_SAVEGAME);

	auto iter = stringToRes.find(extension);
	if (iter == stringToRes.end())
		return EResType::OTHER;
	return iter->second;
}

std::string EResTypeHelper::getEResTypeAsString(EResType type)
{

#define MAP_ENUM(value) (EResType::value, "value")

	static const std::map<EResType, std::string> stringToRes = boost::assign::map_list_of
		MAP_ENUM(ANY)
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
		MAP_ENUM(ARCHIVE)
		MAP_ENUM(CLIENT_SAVEGAME)
		MAP_ENUM(LIB_SAVEGAME)
		MAP_ENUM(SERVER_SAVEGAME)
		MAP_ENUM(OTHER);

#undef MAP_ENUM

	auto iter = stringToRes.find(type);
	assert(iter != stringToRes.end());

	return iter->second;

}
