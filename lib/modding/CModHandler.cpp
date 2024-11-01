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
#include "../CConfigHandler.h"
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
	std::set <TModID> notResolvedModIDs(modsToResolve.begin(), modsToResolve.end()); // Use a set for validation for performance reason

	// Mod is resolved if it has no dependencies or all its dependencies are already resolved
	auto isResolved = [&](const CModInfo & mod) -> bool
	{
		if(mod.dependencies.size() > resolvedModIDs.size())
			return false;

		for(const TModID & dependency : mod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency))
				return false;
		}

		for(const TModID & softDependency : mod.softDependencies)
		{
			if(vstd::contains(notResolvedModIDs, softDependency))
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
			for(const auto & it : resolvedOnCurrentTreeLevel)
				notResolvedModIDs.erase(it);
			continue;
		}
		// If there are no valid mods on the current mods tree level, no more mod can be resolved, should be ended.
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
		bool showErrorMessage = false;
		for(const TModID & dependency : brokenMod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency) && brokenMod.config["modType"].String() != "Compatibility")
			{
				addErrorMessage("vcmi.server.errors.modNoDependency", brokenModID, dependency);
				showErrorMessage = true;
			}
		}
		for(const TModID & conflict : brokenMod.conflicts)
		{
			if(vstd::contains(resolvedModIDs, conflict))
			{
				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, conflict);
				showErrorMessage = true;
			}
		}
		for(const TModID & reverseConflict : resolvedModIDs)
		{
			if (vstd::contains(allMods.at(reverseConflict).conflicts, brokenModID))
			{
				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, reverseConflict);
				showErrorMessage = true;
			}
		}

		// some mods may in a (soft) dependency loop.
		if(!showErrorMessage && brokenMod.config["modType"].String() != "Compatibility")
		{
			modLoadErrors->appendTextID("vcmi.server.errors.modDependencyLoop");
			if (allMods.count(brokenModID))
				modLoadErrors->replaceRawString(allMods.at(brokenModID).getVerificationInfo().name);
			else
				modLoadErrors->replaceRawString(brokenModID);
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

	std::map<std::string, ISimpleResourceLoader *> modFilesystems;

	for(std::string & modName : activeMods)
		modFilesystems[modName] = genModFilesystem(modName, allMods[modName].config);

	for(std::string & modName : activeMods)
		CResourceHandler::addFilesystem("data", modName, modFilesystems[modName]);

	if (settings["mods"]["validation"].String() == "full")
	{
		for(std::string & leftModName : activeMods)
		{
			for(std::string & rightModName : activeMods)
			{
				if (leftModName == rightModName)
					continue;

				if (getModDependencies(leftModName).count(rightModName) || getModDependencies(rightModName).count(leftModName))
					continue;

				if (getModSoftDependencies(leftModName).count(rightModName) || getModSoftDependencies(rightModName).count(leftModName))
					continue;

				const auto & filter = [](const ResourcePath &path){return path.getType() != EResType::DIRECTORY && path.getType() != EResType::JSON;};

				std::unordered_set<ResourcePath> leftResources = modFilesystems[leftModName]->getFilteredFiles(filter);
				std::unordered_set<ResourcePath> rightResources = modFilesystems[rightModName]->getFilteredFiles(filter);

				for (auto const & leftFile : leftResources)
				{
					if (rightResources.count(leftFile))
						logMod->warn("Potential confict detected between '%s' and '%s': both mods add file '%s'", leftModName, rightModName, leftFile.getOriginalName());
				}
			}
		}
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
		std::string fileEncoding = Languages::getLanguageOptions(modLanguage).encoding;
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
	return allMods.at(modId).baseLanguage;
}

std::set<TModID> CModHandler::getModDependencies(const TModID & modId) const
{
	bool isModFound;
	return getModDependencies(modId, isModFound);
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

std::set<TModID> CModHandler::getModSoftDependencies(const TModID & modId) const
{
	auto it = allMods.find(modId);
	if(it != allMods.end())
		return it->second.softDependencies;
	logMod->error("Mod not found: '%s'", modId);
	return {};
}

std::set<TModID> CModHandler::getModEnabledSoftDependencies(const TModID & modId) const
{
	std::set<TModID> softDependencies = getModSoftDependencies(modId);
	for (auto it = softDependencies.begin(); it != softDependencies.end();)
	{
		if (allMods.find(*it) == allMods.end())
			it = softDependencies.erase(it);
		else
			it++;
	}
	return softDependencies;
}

void CModHandler::initializeConfig()
{
	VLC->settingsHandler->loadBase(JsonUtils::assembleFromFiles(coreMod->config["settings"]));

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

void CModHandler::loadTranslation(const TModID & modName)
{
	const auto & mod = allMods[modName];

	std::string preferredLanguage = VLC->generaltexth->getPreferredLanguage();
	std::string modBaseLanguage = allMods[modName].baseLanguage;

	JsonNode baseTranslation = JsonUtils::assembleFromFiles(mod.config["translations"]);
	JsonNode extraTranslation = JsonUtils::assembleFromFiles(mod.config[preferredLanguage]["translations"]);

	VLC->generaltexth->loadTranslationOverrides(modName, modBaseLanguage, baseTranslation);
	VLC->generaltexth->loadTranslationOverrides(modName, preferredLanguage, extraTranslation);
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
