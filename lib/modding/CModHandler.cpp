/*
 * CModHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CModHandler.h"

#include "ContentTypeHandler.h"
#include "IdentifierStorage.h"
#include "ModDescription.h"
#include "ModManager.h"
#include "ModScope.h"

#include "../CConfigHandler.h"
#include "../CCreatureHandler.h"
#include "../GameSettings.h"
#include "../ScriptHandler.h"
#include "../VCMI_Lib.h"
#include "../filesystem/Filesystem.h"
#include "../json/JsonUtils.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"

VCMI_LIB_NAMESPACE_BEGIN

CModHandler::CModHandler()
	: content(std::make_shared<CContentHandler>())
	, modManager(std::make_unique<ModManager>())
{
}

CModHandler::~CModHandler() = default;

std::vector<std::string> CModHandler::getAllMods() const
{
	return modManager->getActiveMods();// TODO: currently identical to active
}

std::vector<std::string> CModHandler::getActiveMods() const
{
	return modManager->getActiveMods();
}

std::string CModHandler::getModLoadErrors() const
{
	return ""; // TODO: modLoadErrors->toString();
}

const ModDescription & CModHandler::getModInfo(const TModID & modId) const
{
	return modManager->getModDescription(modId);
}

static JsonNode genDefaultFS()
{
	// default FS config for mods: directory "Content" that acts as H3 root directory
	JsonNode defaultFS;
	defaultFS[""].Vector().resize(2);
	defaultFS[""].Vector()[0]["type"].String() = "zip";
	defaultFS[""].Vector()[0]["path"].String() = "/Content.zip";
	defaultFS[""].Vector()[1]["type"].String() = "dir";
	defaultFS[""].Vector()[1]["path"].String() = "/Content";
	return defaultFS;
}

static std::string getModDirectory(const TModID & modName)
{
	std::string result = modName;
	boost::to_upper(result);
	boost::algorithm::replace_all(result, ".", "/MODS/");
	return "MODS/" + result;
}

static ISimpleResourceLoader * genModFilesystem(const std::string & modName, const JsonNode & conf)
{
	static const JsonNode defaultFS = genDefaultFS();

	if (!conf.isNull())
		return CResourceHandler::createFileSystem(getModDirectory(modName), conf);
	else
		return CResourceHandler::createFileSystem(getModDirectory(modName), defaultFS);
}

//static ui32 calculateModChecksum(const std::string & modName, ISimpleResourceLoader * filesystem)
//{
//	boost::crc_32_type modChecksum;
//	// first - add current VCMI version into checksum to force re-validation on VCMI updates
//	modChecksum.process_bytes(reinterpret_cast<const void*>(GameConstants::VCMI_VERSION.data()), GameConstants::VCMI_VERSION.size());
//
//	// second - add mod.json into checksum because filesystem does not contains this file
//	// FIXME: remove workaround for core mod
//	if (modName != ModScope::scopeBuiltin())
//	{
//		auto modConfFile = CModInfo::getModFile(modName);
//		ui32 configChecksum = CResourceHandler::get("initial")->load(modConfFile)->calculateCRC32();
//		modChecksum.process_bytes(reinterpret_cast<const void *>(&configChecksum), sizeof(configChecksum));
//	}
//	// third - add all detected text files from this mod into checksum
//	auto files = filesystem->getFilteredFiles([](const ResourcePath & resID)
//	{
//		return (resID.getType() == EResType::TEXT || resID.getType() == EResType::JSON) &&
//			   ( boost::starts_with(resID.getName(), "DATA") || boost::starts_with(resID.getName(), "CONFIG"));
//	});
//
//	for (const ResourcePath & file : files)
//	{
//		ui32 fileChecksum = filesystem->load(file)->calculateCRC32();
//		modChecksum.process_bytes(reinterpret_cast<const void *>(&fileChecksum), sizeof(fileChecksum));
//	}
//	return modChecksum.checksum();
//}

void CModHandler::loadModFilesystems()
{
	CGeneralTextHandler::detectInstallParameters();

	const auto & activeMods = modManager->getActiveMods();

	std::map<TModID, ISimpleResourceLoader *> modFilesystems;

	for(const TModID & modName : activeMods)
		modFilesystems[modName] = genModFilesystem(modName, getModInfo(modName).getFilesystemConfig());

	for(const TModID & modName : activeMods)
		CResourceHandler::addFilesystem("data", modName, modFilesystems[modName]);

	if (settings["mods"]["validation"].String() == "full")
		checkModFilesystemsConflicts(modFilesystems);
}

void CModHandler::checkModFilesystemsConflicts(const std::map<TModID, ISimpleResourceLoader *> & modFilesystems)
{
	for(const auto & [leftName, leftFilesystem] : modFilesystems)
	{
		for(const auto & [rightName, rightFilesystem] : modFilesystems)
		{
			if (leftName == rightName)
				continue;

			if (getModDependencies(leftName).count(rightName) || getModDependencies(rightName).count(leftName))
				continue;

			if (getModSoftDependencies(leftName).count(rightName) || getModSoftDependencies(rightName).count(leftName))
				continue;

			const auto & filter = [](const ResourcePath &path){return path.getType() != EResType::DIRECTORY && path.getType() != EResType::JSON;};

			std::unordered_set<ResourcePath> leftResources = leftFilesystem->getFilteredFiles(filter);
			std::unordered_set<ResourcePath> rightResources = rightFilesystem->getFilteredFiles(filter);

			for (auto const & leftFile : leftResources)
			{
				if (rightResources.count(leftFile))
					logMod->warn("Potential confict detected between '%s' and '%s': both mods add file '%s'", leftName, rightName, leftFile.getOriginalName());
			}
		}
	}
}

TModID CModHandler::findResourceOrigin(const ResourcePath & name) const
{
	try
	{
		auto activeMode = modManager->getActiveMods();
		for(const auto & modID : boost::adaptors::reverse(activeMode))
		{
			if(CResourceHandler::get(modID)->existsResource(name))
				return modID;
		}

		if(CResourceHandler::get("core")->existsResource(name))
			return "core";

		if(CResourceHandler::get("mapEditor")->existsResource(name))
			return "core"; // Workaround for loading maps via map editor
	}
	catch( const std::out_of_range & e)
	{
		// no-op
	}
	throw std::runtime_error("Resource with name " + name.getName() + " and type " + EResTypeHelper::getEResTypeAsString(name.getType()) + " wasn't found.");
}

std::string CModHandler::findResourceLanguage(const ResourcePath & name) const
{
	std::string modName = findResourceOrigin(name);
	std::string modLanguage = getModLanguage(modName);
	return modLanguage;
}

std::string CModHandler::findResourceEncoding(const ResourcePath & resource) const
{
	std::string modName = findResourceOrigin(resource);
	std::string modLanguage = findResourceLanguage(resource);

	bool potentiallyUserMadeContent = resource.getType() == EResType::MAP || resource.getType() == EResType::CAMPAIGN;
	if (potentiallyUserMadeContent && modName == ModScope::scopeBuiltin() && modLanguage == "english")
	{
		// this might be a map or campaign that player downloaded manually and placed in Maps/ directory
		// in this case, this file may be in user-preferred language, and not in same language as the rest of H3 data
		// however at the moment we have no way to detect that for sure - file can be either in English or in user-preferred language
		// but since all known H3 encodings (Win125X or GBK) are supersets of ASCII, we can safely load English data using encoding of user-preferred language
		std::string preferredLanguage = VLC->generaltexth->getPreferredLanguage();
		std::string fileEncoding = Languages::getLanguageOptions(preferredLanguage).encoding;
		return fileEncoding;
	}
	else
	{
		std::string fileEncoding = Languages::getLanguageOptions(modLanguage).encoding;
		return fileEncoding;
	}
}

std::string CModHandler::getModLanguage(const TModID& modId) const
{
	if(modId == "core")
		return VLC->generaltexth->getInstalledLanguage();
	if(modId == "map")
		return VLC->generaltexth->getPreferredLanguage();
	return getModInfo(modId).getBaseLanguage();
}

std::set<TModID> CModHandler::getModDependencies(const TModID & modId) const
{
	bool isModFound;
	return getModDependencies(modId, isModFound);
}

std::set<TModID> CModHandler::getModDependencies(const TModID & modId, bool & isModFound) const
{
	isModFound = modManager->isModActive(modId);
	if (isModFound)
		return modManager->getModDescription(modId).getDependencies();

	logMod->error("Mod not found: '%s'", modId);
	return {};
}

std::set<TModID> CModHandler::getModSoftDependencies(const TModID & modId) const
{
	return modManager->getModDescription(modId).getSoftDependencies();
}

std::set<TModID> CModHandler::getModEnabledSoftDependencies(const TModID & modId) const
{
	std::set<TModID> softDependencies = getModSoftDependencies(modId);

	vstd::erase_if(softDependencies, [&](const TModID & dependency){ return !modManager->isModActive(dependency);});

	return softDependencies;
}

void CModHandler::initializeConfig()
{
	for(const TModID & modName : getActiveMods())
	{
		const auto & mod = getModInfo(modName);
		if (!mod.getLocalConfig()["settings"].isNull())
			VLC->settingsHandler->loadBase(mod.getLocalConfig()["settings"]);
	}
}

void CModHandler::loadTranslation(const TModID & modName)
{
	const auto & mod = getModInfo(modName);

	std::string preferredLanguage = VLC->generaltexth->getPreferredLanguage();
	std::string modBaseLanguage = getModInfo(modName).getBaseLanguage();

	JsonNode baseTranslation = JsonUtils::assembleFromFiles(mod.getLocalConfig()["translations"]);
	JsonNode extraTranslation = JsonUtils::assembleFromFiles(mod.getLocalConfig()[preferredLanguage]["translations"]);

	VLC->generaltexth->loadTranslationOverrides(modName, modBaseLanguage, baseTranslation);
	VLC->generaltexth->loadTranslationOverrides(modName, preferredLanguage, extraTranslation);
}

void CModHandler::load()
{
	logMod->info("\tInitializing content handler");

	content->init();

//	for(const TModID & modName : getActiveMods())
//	{
//		logMod->trace("Generating checksum for %s", modName);
//		allMods[modName].updateChecksum(calculateModChecksum(modName, CResourceHandler::get(modName)));
//	}

	for(const TModID & modName : getActiveMods())
		content->preloadData(getModInfo(modName));
	logMod->info("\tParsing mod data");

	for(const TModID & modName : getActiveMods())
		content->load(getModInfo(modName));

#if SCRIPTING_ENABLED
	VLC->scriptHandler->performRegistration(VLC);//todo: this should be done before any other handlers load
#endif

	content->loadCustom();

	for(const TModID & modName : getActiveMods())
		loadTranslation(modName);

	logMod->info("\tLoading mod data");
	VLC->creh->loadCrExpMod();
	VLC->identifiersHandler->finalize();
	logMod->info("\tResolving identifiers");

	content->afterLoadFinalization();
	logMod->info("\tHandlers post-load finalization");
	logMod->info("\tAll game content loaded");
}

void CModHandler::afterLoad(bool onlyEssential)
{
	//JsonNode modSettings;
	//for (auto & modEntry : getActiveMods())
	//{
	//	std::string pointer = "/" + boost::algorithm::replace_all_copy(modEntry.first, ".", "/mods/");

	//	modSettings["activeMods"].resolvePointer(pointer) = modEntry.second.saveLocalData();
	//}
	//modSettings[ModScope::scopeBuiltin()] = coreMod->saveLocalData();
	//modSettings[ModScope::scopeBuiltin()]["name"].String() = "Original game files";

	//if(!onlyEssential)
	//{
	//	std::fstream file(CResourceHandler::get()->getResourceName(ResourcePath("config/modSettings.json"))->c_str(), std::ofstream::out | std::ofstream::trunc);
	//	file << modSettings.toString();
	//}
}

VCMI_LIB_NAMESPACE_END
