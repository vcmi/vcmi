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
#include "../GameLibrary.h"
#include "../filesystem/Filesystem.h"
#include "../json/JsonUtils.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"

#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <zlib.h>

CModHandler::CModHandler(bool useTestPreset)
	: content(std::make_shared<CContentHandler>())
	, modManager(std::make_unique<ModManager>(JsonNode(), useTestPreset))
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

static JsonPath getModDefinitionFile(const TModID & modName)
{
	return JsonPath::builtin(getModDirectory(modName) + "/mod");
}

/// Contents of all files that a mod lists under a single key of its mod.json
struct ModFiles
{
	JsonNode data;
	uint32_t checksum = 0;
	bool valid = true;
};

/// Reads all files from provided list and merges them into a single node, in the order in which mod.json lists them
static ModFiles assembleModFiles(const TModID & modName, const JsonNode & fileList)
{
	ModFiles result;

	if (!fileList.isVector())
	{
		result.data = fileList; // data is embedded into mod.json instead of being stored in separate files
		return result;
	}

	auto fileNames = fileList.convertTo<std::vector<std::string>>();
	std::vector<std::pair<std::unique_ptr<ui8[]>, si64>> fileContents(fileNames.size());
	uLong checksum = 0;

	// all files of a mod usually come from a single archive, so reading them in sequence reuses its handle
	for (size_t i = 0; i < fileNames.size(); ++i)
	{
		JsonPath path = JsonPath::builtinTODO(fileNames[i]);

		if (!CResourceHandler::get(modName)->existsResource(path))
		{
			logMod->error("Failed to find file %s", fileNames[i]);
			result.valid = false;
			continue;
		}

		fileContents[i] = CResourceHandler::get(modName)->load(path)->readAll();
		checksum = crc32_z(checksum, fileContents[i].first.get(), fileContents[i].second);
	}

	// parsing of a file does not depend on any other file, so files can be parsed in parallel
	std::vector<JsonNode> sections(fileNames.size());
	tbb::parallel_for(tbb::blocked_range<size_t>(0, fileNames.size()), [&](const tbb::blocked_range<size_t> & range)
	{
		for (size_t i = range.begin(); i != range.end(); ++i)
			if (fileContents[i].first)
				sections[i] = JsonNode(reinterpret_cast<std::byte *>(fileContents[i].first.get()), fileContents[i].second, fileNames[i]);
	});

	for (auto & section : sections)
		JsonUtils::merge(result.data, section);

	result.checksum = static_cast<uint32_t>(checksum);
	return result;
}

static std::unique_ptr<ISimpleResourceLoader> genModFilesystem(const std::string & modName, const JsonNode & conf)
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

	// mod filesystems are independent from each other, and their creation is dominated by reading of archive indexes
	std::vector<std::unique_ptr<ISimpleResourceLoader>> loadedFilesystems(activeMods.size());
	tbb::parallel_for(tbb::blocked_range<size_t>(0, activeMods.size()), [this, &activeMods, &loadedFilesystems](const tbb::blocked_range<size_t> & range)
	{
		for(size_t i = range.begin(); i != range.end(); ++i)
			loadedFilesystems[i] = genModFilesystem(activeMods[i], getModInfo(activeMods[i]).getFilesystemConfig());
	});

	std::map<TModID, std::unique_ptr<ISimpleResourceLoader>> modFilesystems;
	for(size_t i = 0; i < activeMods.size(); ++i)
		modFilesystems[activeMods[i]] = std::move(loadedFilesystems[i]);

	if (settings["mods"]["validation"].String() == "full")
		checkModFilesystemsConflicts(modFilesystems);

	for(const TModID & modName : activeMods)
		if (modName != "core") // virtual mod 'core' has no filesystem on its own - shared with base install
			CResourceHandler::addFilesystem("data", modName, std::move(modFilesystems[modName]));
}

void CModHandler::checkModFilesystemsConflicts(const std::map<TModID, std::unique_ptr<ISimpleResourceLoader>> & modFilesystems)
{
	for(const auto & [leftName, leftFilesystem] : modFilesystems)
	{
		for(const auto & [rightName, rightFilesystem] : modFilesystems)
		{
			if (leftName == rightName || !leftFilesystem || !rightFilesystem)
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
		const auto & activeMode = modManager->getActiveMods();
		for(const auto & modID : std::views::reverse(activeMode))
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
	return getResourceEncoding(resource, findResourceOrigin(resource));
}

std::string CModHandler::getResourceEncoding(const ResourcePath & resource, const TModID & modName) const
{
	std::string modLanguage = getModLanguage(modName);

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

const std::set<TModID> & CModHandler::getModDependencies(const TModID & modId) const
{
	bool isModFound;
	return getModDependencies(modId, isModFound);
}

const std::set<TModID> & CModHandler::getModDependencies(const TModID & modId, bool & isModFound) const
{
	static const std::set<TModID> noDependencies;

	isModFound = modManager->isModActive(modId);
	if (isModFound)
		return modManager->getModDescription(modId).getDependencies();

	logMod->error("Mod not found: '%s'", modId);
	return noDependencies;
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

JsonNode CModHandler::loadModContent(const TModID & modName, const std::vector<std::string> & contentTypes, const std::string & preferredLanguage, uint32_t & checksum, bool & isValid) const
{
	const auto & mod = getModInfo(modName);
	const JsonNode & modConfig = mod.getLocalConfig();
	const std::string & modBaseLanguage = mod.getBaseLanguage();

	// content of one type does not depend on content of any other type, so all types can be loaded in parallel
	std::vector<ModFiles> contentFiles(contentTypes.size());
	tbb::parallel_for(tbb::blocked_range<size_t>(0, contentTypes.size()), [&](const tbb::blocked_range<size_t> & range)
	{
		for (size_t i = range.begin(); i != range.end(); ++i)
			contentFiles[i] = assembleModFiles(modName, modConfig[contentTypes[i]]);
	});

	// translations are usually the largest files that a mod provides, so load all languages at once
	ModFiles baseTranslation;
	ModFiles extraTranslation;
	ModFiles fallbackTranslation;

	tbb::parallel_invoke(
		[&]{ baseTranslation = assembleModFiles(modName, modConfig["translations"]); },
		[&]{ extraTranslation = assembleModFiles(modName, modConfig[preferredLanguage]["translations"]); },
		[&]{
			if (preferredLanguage != modBaseLanguage)
				fallbackTranslation = assembleModFiles(modName, modConfig[modBaseLanguage]["translations"]);
		});

	uLong modChecksum = 0;
	// current VCMI version is part of checksum to force re-validation of all mods on VCMI update
	const std::string_view vcmiVersion{GameConstants::VCMI_VERSION};
	modChecksum = crc32_z(modChecksum, reinterpret_cast<const Bytef *>(vcmiVersion.data()), vcmiVersion.size());

	// mod.json is not a part of mod filesystem, so it has to be added into checksum separately
	if (modName != ModScope::scopeBuiltin())
	{
		ui32 configChecksum = CResourceHandler::get("initial")->load(getModDefinitionFile(modName))->calculateCRC32();
		modChecksum = crc32_z(modChecksum, reinterpret_cast<const Bytef *>(&configChecksum), sizeof(configChecksum));
	}

	JsonNode result;

	for (size_t i = 0; i < contentTypes.size(); ++i)
	{
		modChecksum = crc32_z(modChecksum, reinterpret_cast<const Bytef *>(&contentFiles[i].checksum), sizeof(uint32_t));
		isValid = isValid && contentFiles[i].valid;
		contentFiles[i].data.setModScope(modName);
		result[contentTypes[i]] = std::move(contentFiles[i].data);
	}

	for (const ModFiles * translation : { &baseTranslation, &extraTranslation, &fallbackTranslation })
	{
		modChecksum = crc32_z(modChecksum, reinterpret_cast<const Bytef *>(&translation->checksum), sizeof(uint32_t));
		isValid = isValid && translation->valid;
	}

	// Per-key English fallback: for any key missing in the preferred-language
	// translation, substitute the base-language (typically English) string so
	// the player sees readable text instead of a raw key identifier.
	// This handles both completely untranslated mods and partially translated ones.
	if(!fallbackTranslation.data.isNull())
	{
		// Start with the fallback, then let the preferred-language strings
		// overwrite any keys that have already been translated.
		// Guard: merge(dest, null) would clear dest, so skip when there is
		// nothing to overlay from the preferred language.
		if(!extraTranslation.data.isNull())
			JsonUtils::merge(fallbackTranslation.data, extraTranslation.data);
		extraTranslation.data = std::move(fallbackTranslation.data);
	}

	result["translations"] = std::move(baseTranslation.data);
	result[preferredLanguage]["translations"] = std::move(extraTranslation.data);

	checksum = static_cast<uint32_t>(modChecksum);
	return result;
}

void CModHandler::loadModContent()
{
	const auto & activeMods = getActiveMods();
	const std::string preferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();
	const std::vector<std::string> contentTypes = content->getContentTypeNames();

	std::vector<JsonNode> loadedContent(activeMods.size());
	std::vector<uint32_t> checksums(activeMods.size());
	// std::vector<bool> can not be written to from multiple threads
	std::vector<char> validity(activeMods.size(), 1);

	// reading and parsing of files of one mod is independent from every other mod
	tbb::parallel_for(tbb::blocked_range<size_t>(0, activeMods.size()), [&](const tbb::blocked_range<size_t> & range)
	{
		for (size_t i = range.begin(); i != range.end(); ++i)
		{
			bool isValid = true;
			loadedContent[i] = loadModContent(activeMods[i], contentTypes, preferredLanguage, checksums[i], isValid);
			validity[i] = isValid ? 1 : 0;
		}
	});

	for (size_t i = 0; i < activeMods.size(); ++i)
	{
		modContent[activeMods[i]] = std::move(loadedContent[i]);
		modChecksums[activeMods[i]] = checksums[i];
		if (validity[i] == 0)
			modsWithMissingFiles.insert(activeMods[i]);
	}
}

void CModHandler::loadTranslation(const TModID & modName)
{
	const JsonNode & translations = modContent.at(modName);

	LIBRARY->generaltexth->loadTranslationOverrides(modName, getModInfo(modName).getBaseLanguage(), translations["translations"]);
	LIBRARY->generaltexth->loadTranslationOverrides(modName, LIBRARY->generaltexth->getPreferredLanguage(), translations[LIBRARY->generaltexth->getPreferredLanguage()]["translations"]);
}

void CModHandler::load()
{
	logMod->info("\tInitializing content handler");

	content->init();

	const auto & activeMods = getActiveMods();

	validationPassed.insert(activeMods.begin(), activeMods.end());

	loadModContent();

	for(const TModID & modName : activeMods)
	{
		const auto & modInfo = getModInfo(modName);
		bool isValid = content->preloadData(modInfo, modContent.at(modName), isModValidationNeeded(modInfo));

		if (modsWithMissingFiles.count(modName))
			isValid = false;

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

	content->loadCustom();

	logMod->info("\tLoading mod data");
	LIBRARY->creh->loadCrExpMod();
	LIBRARY->identifiersHandler->finalize();
	logMod->info("\tResolving identifiers");

	content->afterLoadFinalization();
	for(const TModID & modName : activeMods)
		loadTranslation(modName);

	modContent.clear();

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
