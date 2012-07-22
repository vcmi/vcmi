#include "StdInc.h"
#include "CResourceLoader.h"
#include "CFileInfo.h"
#include "CLodArchiveLoader.h"

CResourceLoader * CResourceLoaderFactory::resourceLoader = nullptr;

ResourceIdentifier::ResourceIdentifier() : type(EResType::OTHER)
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
	for(auto it = loaders.begin(); it != loaders.end(); ++it)
	{
		delete *it;
	}
}

std::unique_ptr<CInputStream> CResourceLoader::load(const ResourceIdentifier & resourceIdent) const
{
	if(!existsResource(resourceIdent))
	{
		throw std::runtime_error("Resource with name " + resourceIdent.getName() + " and type "
			+ EResTypeHelper::getEResTypeAsString(resourceIdent.getType()) + " wasn't found.");
	}

	// get the last added resource(most overriden)
	const ResourceLocator & locator = resources.at(resourceIdent).back();

	// load the resource and return it
	return locator.getLoader()->load(locator.getResourceName());
}

bool CResourceLoader::existsResource(const ResourceIdentifier & resourceIdent) const
{
	// Check if resource is registered
	if(resources.find(resourceIdent) != resources.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CResourceLoader::addLoader(ISimpleResourceLoader * loader)
{
	loaders.push_back(loader);

	// Get entries and add them to the resources list
	const std::list<std::string> & entries = loader->getEntries();

	for(auto it = entries.begin(); it != entries.end(); ++it)
	{
		std::string entry = *it;
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

	if(extension == ".TXT" || extension == ".JSON")
	{
		return EResType::TEXT;
	}
	else if(extension == ".DEF" || extension == ".JSON")
	{
		return EResType::ANIMATION;
	}
	else if(extension == ".MSK" || extension == ".MSG")
	{
		return EResType::MASK;
	}
	else if(extension == ".H3C")
	{
		return EResType::CAMPAIGN;
	}
	else if(extension == ".H3M")
	{
		return EResType::MAP;
	}
	else if(extension == ".FNT")
	{
		return EResType::FONT;
	}
	else if(extension == ".BMP" || extension == ".JPG" || extension == ".PCX" || extension == ".PNG" || extension == ".TGA")
	{
		return EResType::IMAGE;
	}
	else if(extension == ".WAV")
	{
		return EResType::SOUND;
	}
	else if(extension == ".SMK" || extension == ".BIK")
	{
		return EResType::VIDEO;
	}
	else if(extension == ".MP3" || extension == ".OGG")
	{
		return EResType::MUSIC;
	}
	else if(extension == ".ZIP" || extension == ".TAR.GZ" || extension == ".LOD" || extension == ".VID" || extension == ".SND")
	{
		return EResType::ARCHIVE;
	}
	else if(extension == ".VLGM1")
	{
		return EResType::SAVEGAME;
	}
	else
	{
		return EResType::OTHER;
	}
}

std::string EResTypeHelper::getEResTypeAsString(EResType type)
{
	if(type == EResType::ANIMATION)
	{
		return "ANIMATION";
	}
	else if(type == EResType::ANY)
	{
		return "ANY";
	}
	else if(type == EResType::ARCHIVE)
	{
		return "ARCHIVE";
	}
	else if(type == EResType::CAMPAIGN)
	{
		return "CAMPAIGN";
	}
	else if(type == EResType::FONT)
	{
		return "FONT";
	}
	else if(type == EResType::IMAGE)
	{
		return "IMAGE";
	}
	else if(type == EResType::MAP)
	{
		return "MAP";
	}
	else if(type == EResType::MASK)
	{
		return "MASK";
	}
	else if(type == EResType::MUSIC)
	{
		return "MUSIC";
	}
	else if(type == EResType::OTHER)
	{
		return "OTHER";
	}
	else if(type == EResType::SAVEGAME)
	{
		return "SAVEGAME";
	}
	else if(type == EResType::SOUND)
	{
		return "SOUND";
	}
	else if(type == EResType::TEXT)
	{
		return "TEXT";
	}
	else if(type == EResType::VIDEO)
	{
		return "VIDEO";
	}

	return "";
}
