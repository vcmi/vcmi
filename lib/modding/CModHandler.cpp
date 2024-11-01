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

#include "CModInfo.h"
#include "ModScope.h"
#include "ContentTypeHandler.h"
#include "IdentifierStorage.h"
#include "ModIncompatibility.h"

#include "../CCreatureHandler.h"
#include "../CStopWatch.h"
#include "../GameSettings.h"
#include "../ScriptHandler.h"
#include "../constants/StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "../json/JsonUtils.h"
#include "../spells/CSpellHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"
#include "../VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

static JsonNode loadModSettings(const JsonPath & path)
{
	if (CResourceHandler::get("local")->existsResource(ResourcePath(path)))
	{
		return JsonNode(path);
	}
	// Probably new install. Create initial configuration
	CResourceHandler::get("local")->createResource(path.getOriginalName() + ".json");
	return JsonNode();
}

CModHandler::CModHandler()
	: content(std::make_shared<CContentHandler>())
	, coreMod(std::make_unique<CModInfo>())
{
}

CModHandler::~CModHandler() = default;

// currentList is passed by value to get current list of depending mods
bool CModHandler::hasCircularDependency(const TModID & modID, std::set<TModID> currentList) const
{
	const CModInfo & mod = allMods.at(modID);

	// Mod already present? We found a loop
	if (vstd::contains(currentList, modID))
	{
		logMod->error("Error: Circular dependency detected! Printing dependency list:");
		logMod->error("\t%s -> ", mod.getVerificationInfo().name);
		return true;
	}

	currentList.insert(modID);

	// recursively check every dependency of this mod
	for(const TModID & dependency : mod.dependencies)
	{
		if (hasCircularDependency(dependency, currentList))
		{
			logMod->error("\t%s ->\n", mod.getVerificationInfo().name); // conflict detected, print dependency list
			return true;
		}
	}
	return false;
}

// Returned vector affects the resource loaders call order (see CFilesystemList::load).
// The loaders call order matters when dependent mod overrides resources in its dependencies.
std::vector <TModID> CModHandler::validateAndSortDependencies(std::vector <TModID> modsToResolve) const
{
	// Topological sort algorithm.
	// TODO: Investigate possible ways to improve performance.
	boost::range::sort(modsToResolve); // Sort mods per name
	std::vector <TModID> sortedValidMods; // Vector keeps order of elements (LIFO)
	sortedValidMods.reserve(modsToResolve.size()); // push_back calls won't cause memory reallocation
	std::set <TModID> resolvedModIDs; // Use a set for validation for performance reason, but set does not keep order of elements

	// Mod is resolved if it has not dependencies or all its dependencies are already resolved
	auto isResolved = [&](const CModInfo & mod) -> bool
	{
		if(mod.dependencies.size() > resolvedModIDs.size())
			return false;

		for(const TModID & dependency : mod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency))
				return false;
		}

		for(const TModID & conflict : mod.conflicts)
		{
			if(vstd::contains(resolvedModIDs, conflict))
				return false;
		}
		for(const TModID & reverseConflict : resolvedModIDs)
		{
			if (vstd::contains(allMods.at(reverseConflict).conflicts, mod.identifier))
				return false;
		}
		return true;
	};

	while(true)
	{
		std::set <TModID> resolvedOnCurrentTreeLevel;
		for(auto it = modsToResolve.begin(); it != modsToResolve.end();) // One iteration - one level of mods tree
		{
			if(isResolved(allMods.at(*it)))
			{
				resolvedOnCurrentTreeLevel.insert(*it); // Not to the resolvedModIDs, so current node children will be resolved on the next iteration
				sortedValidMods.push_back(*it);
				it = modsToResolve.erase(it);
				continue;
			}
			it++;
		}
		if(!resolvedOnCurrentTreeLevel.empty())
		{
			resolvedModIDs.insert(resolvedOnCurrentTreeLevel.begin(), resolvedOnCurrentTreeLevel.end());
					continue;
		}
		// If there're no valid mods on the current mods tree level, no more mod can be resolved, should be end.
		break;
	}

	modLoadErrors = std::make_unique<MetaString>();

	auto addErrorMessage = [this](const std::string & textID, const std::string & brokenModID, const std::string & missingModID)
	{
		modLoadErrors->appendTextID(textID);

		if (allMods.count(brokenModID))
			modLoadErrors->replaceRawString(allMods.at(brokenModID).getVerificationInfo().name);
		else
			modLoadErrors->replaceRawString(brokenModID);

		if (allMods.count(missingModID))
			modLoadErrors->replaceRawString(allMods.at(missingModID).getVerificationInfo().name);
		else
			modLoadErrors->replaceRawString(missingModID);

	};

	// Left mods have unresolved dependencies, output all to log.
	for(const auto & brokenModID : modsToResolve)
	{
		const CModInfo & brokenMod = allMods.at(brokenModID);
		for(const TModID & dependency : brokenMod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency) && brokenMod.config["modType"].String() != "Compatibility")
				addErrorMessage("vcmi.server.errors.modNoDependency", brokenModID, dependency);
		}
		for(const TModID & conflict : brokenMod.conflicts)
		{
			if(vstd::contains(resolvedModIDs, conflict))
				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, conflict);
		}
		for(const TModID & reverseConflict : resolvedModIDs)
		{
			if (vstd::contains(allMods.at(reverseConflict).conflicts, brokenModID))
				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, reverseConflict);
		}
	}
	return sortedValidMods;
}

std::vector<std::string> CModHandler::getModList(const std::string & path) const
{
	std::string modDir = boost::to_upper_copy(path + "MODS/");
	size_t depth = boost::range::count(modDir, '/');

	auto list = CResourceHandler::get("initial")->getFilteredFiles([&](const ResourcePath & id) ->  bool
	{
		if (id.getType() != EResType::DIRECTORY)
			return false;
		if (!boost::algorithm::starts_with(id.getName(), modDir))
			return false;
		if (boost::range::count(id.getName(), '/') != depth )
			return false;
		return true;
	});

	//storage for found mods
	std::vector<std::string> foundMods;
	for(const auto & entry : list)
	{
		std::string name = entry.getName();
		name.erase(0, modDir.size()); //Remove path prefix

		if (!name.empty())
			foundMods.push_back(name);
	}
	return foundMods;
}



void CModHandler::loadMods(const std::string & path, const std::string & parent, const JsonNode & modSettings, bool enableMods)
{
	for(const std::string & modName : getModList(path))
		loadOneMod(modName, parent, modSettings, enableMods);
}

void CModHandler::loadOneMod(std::string modName, const std::string & parent, const JsonNode & modSettings, bool enableMods)
{
	boost::to_lower(modName);
	std::string modFullName = parent.empty() ? modName : parent + '.' + modName;

	if ( ModScope::isScopeReserved(modFullName))
	{
		logMod->error("Can not load mod %s - this name is reserved for internal use!", modFullName);
		return;
	}

	if(CResourceHandler::get("initial")->existsResource(CModInfo::getModFile(modFullName)))
	{
		CModInfo mod(modFullName, modSettings[modName], JsonNode(CModInfo::getModFile(modFullName)));
		if (!parent.empty()) // this is submod, add parent to dependencies
			mod.dependencies.insert(parent);

		allMods[modFullName] = mod;
		if (mod.isEnabled() && enableMods)
			activeMods.push_back(modFullName);

		loadMods(CModInfo::getModDir(modFullName) + '/', modFullName, modSettings[modName]["mods"], enableMods && mod.isEnabled());
	}
}

void CModHandler::loadMods()
{
	JsonNode modConfig;

	modConfig = loadModSettings(JsonPath::builtin("config/modSettings.json"));
	loadMods("", "", modConfig["activeMods"], true);

	coreMod = std::make_unique<CModInfo>(ModScope::scopeBuiltin(), modConfig[ModScope::scopeBuiltin()], JsonNode(JsonPath::builtin("config/gameConfig.json")));
}

std::vector<std::string> CModHandler::getAllMods() const
{
	std::vector<std::string> modlist;
	modlist.reserve(allMods.size());
	for (auto & entry : allMods)
		modlist.push_back(entry.first);
	return modlist;
}

std::vector<std::string> CModHandler::getActiveMods() const
{
	return activeMods;
}

std::string CModHandler::getModLoadErrors() const
{
	return modLoadErrors->toString();
}

const CModInfo & CModHandler::getModInfo(const TModID & modId) const
{
	return allMods.at(modId);
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

static ISimpleResourceLoader * genModFilesystem(const std::string & modName, const JsonNode & conf)
{
	static const JsonNode defaultFS = genDefaultFS();

	if (!conf["filesystem"].isNull())
		return CResourceHandler::createFileSystem(CModInfo::getModDir(modName), conf["filesystem"]);
	else
		return CResourceHandler::createFileSystem(CModInfo::getModDir(modName), defaultFS);
}

static ui32 calculateModChecksum(const std::string & modName, ISimpleResourceLoader * filesystem)
{
	boost::crc_32_type modChecksum;
	// first - add current VCMI version into checksum to force re-validation on VCMI updates
	modChecksum.process_bytes(reinterpret_cast<const void*>(GameConstants::VCMI_VERSION.data()), GameConstants::VCMI_VERSION.size());

	// second - add mod.json into checksum because filesystem does not contains this file
	// FIXME: remove workaround for core mod
	if (modName != ModScope::scopeBuiltin())
	{
		auto modConfFile = CModInfo::getModFile(modName);
		ui32 configChecksum = CResourceHandler::get("initial")->load(modConfFile)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&configChecksum), sizeof(configChecksum));
	}
	// third - add all detected text files from this mod into checksum
	auto files = filesystem->getFilteredFiles([](const ResourcePath & resID)
	{
		return (resID.getType() == EResType::TEXT || resID.getType() == EResType::JSON) &&
			   ( boost::starts_with(resID.getName(), "DATA") || boost::starts_with(resID.getName(), "CONFIG"));
	});

	for (const ResourcePath & file : files)
	{
		ui32 fileChecksum = filesystem->load(file)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&fileChecksum), sizeof(fileChecksum));
	}
	return modChecksum.checksum();
}

void CModHandler::loadModFilesystems()
{
	CGeneralTextHandler::detectInstallParameters();

	activeMods = validateAndSortDependencies(activeMods);

	coreMod->updateChecksum(calculateModChecksum(ModScope::scopeBuiltin(), CResourceHandler::get(ModScope::scopeBuiltin())));

	for(std::string & modName : activeMods)
	{
		CModInfo & mod = allMods[modName];
		CResourceHandler::addFilesystem("data", modName, genModFilesystem(modName, mod.config));
	}
}

TModID CModHandler::findResourceOrigin(const ResourcePath & name) const
{
	try
	{
		for(const auto & modID : boost::adaptors::reverse(activeMods))
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

std::string CModHandler::getModLanguage(const TModID& modId) const
{
	if(modId == "core")
		return VLC->generaltexth->getInstalledLanguage();
	if(modId == "map")
		return VLC->generaltexth->getPreferredLanguage();
	return allMods.at(modId).baseLanguage;
}

std::set<TModID> CModHandler::getModDependencies(const TModID & modId, bool & isModFound) const
{
	auto it = allMods.find(modId);
	isModFound = (it != allMods.end());

	if(isModFound)
		return it->second.dependencies;

	logMod->error("Mod not found: '%s'", modId);
	return {};
}

void CModHandler::initializeConfig()
{
	VLC->settingsHandler->loadBase(coreMod->config["settings"]);

	for(const TModID & modName : activeMods)
	{
		const auto & mod = allMods[modName];
		if (!mod.config["settings"].isNull())
			VLC->settingsHandler->loadBase(mod.config["settings"]);
	}
}

CModVersion CModHandler::getModVersion(TModID modName) const
{
	if (allMods.count(modName))
		return allMods.at(modName).getVerificationInfo().version;
	return {};
}

bool CModHandler::validateTranslations(TModID modName) const
{
	bool result = true;
	const auto & mod = allMods.at(modName);

	{
		auto fileList = mod.config["translations"].convertTo<std::vector<std::string> >();
		JsonNode json = JsonUtils::assembleFromFiles(fileList);
		result |= VLC->generaltexth->validateTranslation(mod.baseLanguage, modName, json);
	}

	for(const auto & language : Languages::getLanguageList())
	{
		if (mod.config[language.identifier].isNull())
			continue;

		if (mod.config[language.identifier]["skipValidation"].Bool())
			continue;

		auto fileList = mod.config[language.identifier]["translations"].convertTo<std::vector<std::string> >();
		JsonNode json = JsonUtils::assembleFromFiles(fileList);
		result |= VLC->generaltexth->validateTranslation(language.identifier, modName, json);
	}

	return result;
}

void CModHandler::loadTranslation(const TModID & modName)
{
	const auto & mod = allMods[modName];

	std::string preferredLanguage = VLC->generaltexth->getPreferredLanguage();
	std::string modBaseLanguage = allMods[modName].baseLanguage;

	auto baseTranslationList = mod.config["translations"].convertTo<std::vector<std::string> >();
	auto extraTranslationList = mod.config[preferredLanguage]["translations"].convertTo<std::vector<std::string> >();

	JsonNode baseTranslation = JsonUtils::assembleFromFiles(baseTranslationList);
	JsonNode extraTranslation = JsonUtils::assembleFromFiles(extraTranslationList);

	VLC->generaltexth->loadTranslationOverrides(modBaseLanguage, modName, baseTranslation);
	VLC->generaltexth->loadTranslationOverrides(preferredLanguage, modName, extraTranslation);
}

void CModHandler::load()
{
	CStopWatch totalTime;
	CStopWatch timer;

	logMod->info("\tInitializing content handler: %d ms", timer.getDiff());

	content->init();

	for(const TModID & modName : activeMods)
	{
		logMod->trace("Generating checksum for %s", modName);
		allMods[modName].updateChecksum(calculateModChecksum(modName, CResourceHandler::get(modName)));
	}

	// first - load virtual builtin mod that contains all data
	// TODO? move all data into real mods? RoE, AB, SoD, WoG
	content->preloadData(*coreMod);
	for(const TModID & modName : activeMods)
		content->preloadData(allMods[modName]);
	logMod->info("\tParsing mod data: %d ms", timer.getDiff());

	content->load(*coreMod);
	for(const TModID & modName : activeMods)
		content->load(allMods[modName]);

#if SCRIPTING_ENABLED
	VLC->scriptHandler->performRegistration(VLC);//todo: this should be done before any other handlers load
#endif

	content->loadCustom();

	for(const TModID & modName : activeMods)
		loadTranslation(modName);

#if 0
	for(const TModID & modName : activeMods)
		if (!validateTranslations(modName))
			allMods[modName].validation = CModInfo::FAILED;
#endif

	logMod->info("\tLoading mod data: %d ms", timer.getDiff());
	VLC->creh->loadCrExpMod();
	VLC->identifiersHandler->finalize();
	logMod->info("\tResolving identifiers: %d ms", timer.getDiff());

	content->afterLoadFinalization();
	logMod->info("\tHandlers post-load finalization: %d ms ", timer.getDiff());
	logMod->info("\tAll game content loaded in %d ms", totalTime.getDiff());
}

void CModHandler::afterLoad(bool onlyEssential)
{
	JsonNode modSettings;
	for (auto & modEntry : allMods)
	{
		std::string pointer = "/" + boost::algorithm::replace_all_copy(modEntry.first, ".", "/mods/");

		modSettings["activeMods"].resolvePointer(pointer) = modEntry.second.saveLocalData();
	}
	modSettings[ModScope::scopeBuiltin()] = coreMod->saveLocalData();
	modSettings[ModScope::scopeBuiltin()]["name"].String() = "Original game files";

	if(!onlyEssential)
	{
		std::fstream file(CResourceHandler::get()->getResourceName(ResourcePath("config/modSettings.json"))->c_str(), std::ofstream::out | std::ofstream::trunc);
		file << modSettings.toString();
	}
}

VCMI_LIB_NAMESPACE_END
