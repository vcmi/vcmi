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
#include "../GameLibrary.h"
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
	return modManager->getAllMods();
}

const std::vector<std::string> & CModHandler::getActiveMods() const
{
	return modManager->getActiveMods();
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

void CModHandler::loadModFilesystems()
{
	CGeneralTextHandler::detectInstallParameters();

	const auto & activeMods = modManager->getActiveMods();

	std::map<TModID, ISimpleResourceLoader *> modFilesystems;

	for(const TModID & modName : activeMods)
		modFilesystems[modName] = genModFilesystem(modName, getModInfo(modName).getFilesystemConfig());

	for(const TModID & modName : activeMods)
		if (modName != "core") // virtual mod 'core' has no filesystem on its own - shared with base install
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
			return "mapEditor"; // Workaround for loading maps via map editor
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
		std::string preferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();
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
		return LIBRARY->generaltexth->getInstalledLanguage();
	if(modId == "map")
		return LIBRARY->generaltexth->getPreferredLanguage();
	if(modId == "mapEditor")
		return LIBRARY->generaltexth->getPreferredLanguage();
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

	vstd::erase_if(softDependencies, [this](const TModID & dependency){ return !modManager->isModActive(dependency);});

	return softDependencies;
}

void CModHandler::initializeConfig()
{
	for(const TModID & modName : getActiveMods())
	{
		const auto & mod = getModInfo(modName);
		if (!mod.getLocalConfig()["settings"].isNull())
			LIBRARY->settingsHandler->loadBase(mod.getLocalConfig()["settings"]);
	}
}

void CModHandler::loadTranslation(const TModID & modName)
{
	const auto & mod = getModInfo(modName);

	std::string preferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();
	std::string modBaseLanguage = getModInfo(modName).getBaseLanguage();

	JsonNode baseTranslation = JsonUtils::assembleFromFiles(mod.getLocalConfig()["translations"]);
	JsonNode extraTranslation = JsonUtils::assembleFromFiles(mod.getLocalConfig()[preferredLanguage]["translations"]);

	LIBRARY->generaltexth->loadTranslationOverrides(modName, modBaseLanguage, baseTranslation);
	LIBRARY->generaltexth->loadTranslationOverrides(modName, preferredLanguage, extraTranslation);
}

void CModHandler::load()
{
	logMod->info("\tInitializing content handler");

	content->init();

	const auto & activeMods = getActiveMods();

	validationPassed.insert(activeMods.begin(), activeMods.end());

	for(const TModID & modName : activeMods)
	{
		modChecksums[modName] = this->modManager->computeChecksum(modName);
	}

	for(const TModID & modName : activeMods)
	{
		const auto & modInfo = getModInfo(modName);
		bool isValid = content->preloadData(modInfo, isModValidationNeeded(modInfo));
		if (isValid)
			logGlobal->info("\t\tParsing mod: OK (%s)", modInfo.getID());
		else
			logGlobal->warn("\t\tParsing mod: Issues found! (%s)", modInfo.getID());

		if (!isValid)
			validationPassed.erase(modName);
	}
	logMod->info("\tParsing mod data");

	for(const TModID & modName : activeMods)
	{
		const auto & modInfo = getModInfo(modName);
		bool isValid = content->load(getModInfo(modName), isModValidationNeeded(getModInfo(modName)));
		if (isValid)
			logGlobal->info("\t\tLoading mod: OK (%s)", modInfo.getID());
		else
			logGlobal->warn("\t\tLoading mod: Issues found! (%s)", modInfo.getID());

		if (!isValid)
			validationPassed.erase(modName);
	}

#if SCRIPTING_ENABLED
	LIBRARY->scriptHandler->performRegistration(LIBRARY);//todo: this should be done before any other handlers load
#endif

	content->loadCustom();

	for(const TModID & modName : activeMods)
		loadTranslation(modName);

	logMod->info("\tLoading mod data");
	LIBRARY->creh->loadCrExpMod();
	LIBRARY->identifiersHandler->finalize();
	logMod->info("\tResolving identifiers");

	content->afterLoadFinalization();
	logMod->info("\tHandlers post-load finalization");
	logMod->info("\tAll game content loaded");
}

void CModHandler::afterLoad()
{
	JsonNode modSettings;
	for (const auto & modEntry : getActiveMods())
	{
		if (validationPassed.count(modEntry))
			modManager->setValidatedChecksum(modEntry, modChecksums.at(modEntry));
		else
			modManager->setValidatedChecksum(modEntry, std::nullopt);
	}

	modManager->saveConfigurationState();
}

bool CModHandler::isModValidationNeeded(const ModDescription & mod) const
{
	if (settings["mods"]["validation"].String() == "full")
		return true;

	if (modManager->getValidatedChecksum(mod.getID()) == modChecksums.at(mod.getID()))
		return false;

	if (settings["mods"]["validation"].String() == "off")
		return false;

	return true;
}

VCMI_LIB_NAMESPACE_END
