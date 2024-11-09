/*
 * ModManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModManager.h"

#include "ModDescription.h"
#include "ModScope.h"

#include "../filesystem/Filesystem.h"
#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

static std::string getModSettingsDirectory(const TModID & modName)
{
	std::string result = modName;
	boost::to_upper(result);
	boost::algorithm::replace_all(result, ".", "/MODS/");
	return "MODS/" + result + "/MODS/";
}

static JsonPath getModDescriptionFile(const TModID & modName)
{
	std::string result = modName;
	boost::to_upper(result);
	boost::algorithm::replace_all(result, ".", "/MODS/");
	return JsonPath::builtin("MODS/" + result + "/mod");
}

ModsState::ModsState()
{
	modList.push_back(ModScope::scopeBuiltin());

	std::vector<TModID> testLocations = scanModsDirectory("MODS/");

	while(!testLocations.empty())
	{
		std::string target = testLocations.back();
		testLocations.pop_back();
		modList.push_back(boost::algorithm::to_lower_copy(target));

		for(const auto & submod : scanModsDirectory(getModSettingsDirectory(target)))
			testLocations.push_back(target + '.' + submod);

		// TODO: check that this is vcmi mod and not era mod?
		// TODO: check that mod name is not reserved (ModScope::isScopeReserved(modFullName)))
	}
}

TModList ModsState::getAllMods() const
{
	return modList;
}

std::vector<TModID> ModsState::scanModsDirectory(const std::string & modDir) const
{
	size_t depth = boost::range::count(modDir, '/');

	const auto & modScanFilter = [&](const ResourcePath & id) -> bool
	{
		if(id.getType() != EResType::DIRECTORY)
			return false;
		if(!boost::algorithm::starts_with(id.getName(), modDir))
			return false;
		if(boost::range::count(id.getName(), '/') != depth)
			return false;
		return true;
	};

	auto list = CResourceHandler::get("initial")->getFilteredFiles(modScanFilter);

	//storage for found mods
	std::vector<TModID> foundMods;
	for(const auto & entry : list)
	{
		std::string name = entry.getName();
		name.erase(0, modDir.size()); //Remove path prefix

		if(name.empty())
			continue;

		if(name.find('.') != std::string::npos)
			continue;

		if(!CResourceHandler::get("initial")->existsResource(JsonPath::builtin(entry.getName() + "/MOD")))
			continue;

		foundMods.push_back(name);
	}
	return foundMods;
}

///////////////////////////////////////////////////////////////////////////////

static JsonNode loadModSettings(const JsonPath & path)
{
	if(CResourceHandler::get("local")->existsResource(ResourcePath(path)))
	{
		return JsonNode(path);
	}
	// Probably new install. Create initial configuration
	CResourceHandler::get("local")->createResource(path.getOriginalName() + ".json");
	return JsonNode();
}

ModsPresetState::ModsPresetState()
{
	modConfig = loadModSettings(JsonPath::builtin("config/modSettings.json"));

	if(modConfig["presets"].isNull())
	{
		modConfig["activePreset"] = JsonNode("default");
		if(modConfig["activeMods"].isNull())
			createInitialPreset(); // new install
		else
			importInitialPreset(); // 1.5 format import

		saveConfiguration(modConfig);
	}
}

void ModsPresetState::saveConfiguration(const JsonNode & modSettings)
{
	std::fstream file(CResourceHandler::get()->getResourceName(ResourcePath("config/modSettings.json"))->c_str(), std::ofstream::out | std::ofstream::trunc);
	file << modSettings.toString();
}

void ModsPresetState::createInitialPreset()
{
	// TODO: scan mods directory for all its content? Probably unnecessary since this looks like new install, but who knows?
	modConfig["presets"]["default"]["mods"].Vector().push_back(JsonNode("vcmi"));
}

void ModsPresetState::importInitialPreset()
{
	JsonNode preset;

	for(const auto & mod : modConfig["activeMods"].Struct())
	{
		if(mod.second["active"].Bool())
			preset["mods"].Vector().push_back(JsonNode(mod.first));

		for(const auto & submod : mod.second["mods"].Struct())
			preset["settings"][mod.first][submod.first] = submod.second["active"];
	}
	modConfig["presets"]["default"] = preset;
}

std::vector<TModID> ModsPresetState::getActiveMods() const
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	const JsonNode & currentPreset = modConfig["presets"][currentPresetName];
	const JsonNode & modsToActivateJson = currentPreset["mods"];
	std::vector<TModID> modsToActivate = modsToActivateJson.convertTo<std::vector<TModID>>();

	modsToActivate.push_back(ModScope::scopeBuiltin());

	for(const auto & settings : currentPreset["settings"].Struct())
	{
		if(!vstd::contains(modsToActivate, settings.first))
			continue; // settings for inactive mod

		for(const auto & submod : settings.second.Struct())
			if(submod.second.Bool())
				modsToActivate.push_back(settings.first + '.' + submod.first);
	}
	return modsToActivate;
}

ModsStorage::ModsStorage(const std::vector<TModID> & modsToLoad)
{
	JsonNode coreModConfig(JsonPath::builtin("config/gameConfig.json"));
	coreModConfig.setModScope(ModScope::scopeBuiltin());
	mods.try_emplace(ModScope::scopeBuiltin(), ModScope::scopeBuiltin(), coreModConfig);

	for(auto modID : modsToLoad)
	{
		if(ModScope::isScopeReserved(modID))
		{
			logMod->error("Can not load mod %s - this name is reserved for internal use!", modID);
			continue;
		}

		JsonNode modConfig(getModDescriptionFile(modID));
		modConfig.setModScope(modID);

		if(modConfig["modType"].isNull())
		{
			logMod->error("Can not load mod %s - invalid mod config file!", modID);
			continue;
		}

		mods.try_emplace(modID, modID, modConfig);
	}
}

const ModDescription & ModsStorage::getMod(const TModID & fullID) const
{
	return mods.at(fullID);
}

ModManager::ModManager()
	: modsState(std::make_unique<ModsState>())
	, modsPreset(std::make_unique<ModsPresetState>())
{
	std::vector<TModID> desiredModList = modsPreset->getActiveMods();
	const std::vector<TModID> & installedModList = modsState->getAllMods();

	vstd::erase_if(desiredModList, [&](const TModID & mod){
		return !vstd::contains(installedModList, mod);
	});

	modsStorage = std::make_unique<ModsStorage>(desiredModList);

	generateLoadOrder(desiredModList);
}

ModManager::~ModManager() = default;

const ModDescription & ModManager::getModDescription(const TModID & modID) const
{
	return modsStorage->getMod(modID);
}

bool ModManager::isModActive(const TModID & modID) const
{
	return vstd::contains(activeMods, modID);
}

const TModList & ModManager::getActiveMods() const
{
	return activeMods;
}

void ModManager::generateLoadOrder(std::vector<TModID> modsToResolve)
{
	// Topological sort algorithm.
	boost::range::sort(modsToResolve); // Sort mods per name
	std::vector<TModID> sortedValidMods; // Vector keeps order of elements (LIFO)
	sortedValidMods.reserve(modsToResolve.size()); // push_back calls won't cause memory reallocation
	std::set<TModID> resolvedModIDs; // Use a set for validation for performance reason, but set does not keep order of elements
	std::set<TModID> notResolvedModIDs(modsToResolve.begin(), modsToResolve.end()); // Use a set for validation for performance reason

	// Mod is resolved if it has no dependencies or all its dependencies are already resolved
	auto isResolved = [&](const ModDescription & mod) -> bool
	{
		if(mod.getDependencies().size() > resolvedModIDs.size())
			return false;

		for(const TModID & dependency : mod.getDependencies())
			if(!vstd::contains(resolvedModIDs, dependency))
				return false;

		for(const TModID & softDependency : mod.getSoftDependencies())
			if(vstd::contains(notResolvedModIDs, softDependency))
				return false;

		for(const TModID & conflict : mod.getConflicts())
			if(vstd::contains(resolvedModIDs, conflict))
				return false;

		for(const TModID & reverseConflict : resolvedModIDs)
			if(vstd::contains(modsStorage->getMod(reverseConflict).getConflicts(), mod.getID()))
				return false;

		return true;
	};

	while(true)
	{
		std::set<TModID> resolvedOnCurrentTreeLevel;
		for(auto it = modsToResolve.begin(); it != modsToResolve.end();) // One iteration - one level of mods tree
		{
			if(isResolved(modsStorage->getMod(*it)))
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

	activeMods = sortedValidMods;
	brokenMods = modsToResolve;
}

//	modLoadErrors = std::make_unique<MetaString>();
//
//	auto addErrorMessage = [this](const std::string & textID, const std::string & brokenModID, const std::string & missingModID)
//	{
//		modLoadErrors->appendTextID(textID);
//
//		if (allMods.count(brokenModID))
//			modLoadErrors->replaceRawString(allMods.at(brokenModID).getVerificationInfo().name);
//		else
//			modLoadErrors->replaceRawString(brokenModID);
//
//		if (allMods.count(missingModID))
//			modLoadErrors->replaceRawString(allMods.at(missingModID).getVerificationInfo().name);
//		else
//			modLoadErrors->replaceRawString(missingModID);
//
//	};
//
//	// Left mods have unresolved dependencies, output all to log.
//	for(const auto & brokenModID : modsToResolve)
//	{
//		const CModInfo & brokenMod = allMods.at(brokenModID);
//		bool showErrorMessage = false;
//		for(const TModID & dependency : brokenMod.dependencies)
//		{
//			if(!vstd::contains(resolvedModIDs, dependency) && brokenMod.config["modType"].String() != "Compatibility")
//			{
//				addErrorMessage("vcmi.server.errors.modNoDependency", brokenModID, dependency);
//				showErrorMessage = true;
//			}
//		}
//		for(const TModID & conflict : brokenMod.conflicts)
//		{
//			if(vstd::contains(resolvedModIDs, conflict))
//			{
//				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, conflict);
//				showErrorMessage = true;
//			}
//		}
//		for(const TModID & reverseConflict : resolvedModIDs)
//		{
//			if (vstd::contains(allMods.at(reverseConflict).conflicts, brokenModID))
//			{
//				addErrorMessage("vcmi.server.errors.modConflict", brokenModID, reverseConflict);
//				showErrorMessage = true;
//			}
//		}
//
//		// some mods may in a (soft) dependency loop.
//		if(!showErrorMessage && brokenMod.config["modType"].String() != "Compatibility")
//		{
//			modLoadErrors->appendTextID("vcmi.server.errors.modDependencyLoop");
//			if (allMods.count(brokenModID))
//				modLoadErrors->replaceRawString(allMods.at(brokenModID).getVerificationInfo().name);
//			else
//				modLoadErrors->replaceRawString(brokenModID);
//		}
//
//	}
//	return sortedValidMods;
//}

VCMI_LIB_NAMESPACE_END
