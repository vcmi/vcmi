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

#include "../constants/StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

static std::string getModDirectory(const TModID & modName)
{
	std::string result = modName;
	boost::to_upper(result);
	boost::algorithm::replace_all(result, ".", "/MODS/");
	return "MODS/" + result;
}

static std::string getModSettingsDirectory(const TModID & modName)
{
	return getModDirectory(modName) + "/MODS/";
}

static JsonPath getModDescriptionFile(const TModID & modName)
{
	return JsonPath::builtin(getModDirectory(modName) + "/mod");
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
	}
}

TModList ModsState::getInstalledMods() const
{
	return modList;
}

uint32_t ModsState::computeChecksum(const TModID & modName) const
{
	boost::crc_32_type modChecksum;
	// first - add current VCMI version into checksum to force re-validation on VCMI updates
	modChecksum.process_bytes(static_cast<const void*>(GameConstants::VCMI_VERSION.data()), GameConstants::VCMI_VERSION.size());

	// second - add mod.json into checksum because filesystem does not contains this file
	// FIXME: remove workaround for core mod
	if (modName != ModScope::scopeBuiltin())
	{
		auto modConfFile = getModDescriptionFile(modName);
		ui32 configChecksum = CResourceHandler::get("initial")->load(modConfFile)->calculateCRC32();
		modChecksum.process_bytes(static_cast<const void *>(&configChecksum), sizeof(configChecksum));
	}

	// third - add all detected text files from this mod into checksum
	const auto & filesystem = CResourceHandler::get(modName);

	auto files = filesystem->getFilteredFiles([](const ResourcePath & resID)
	{
		return resID.getType() == EResType::JSON && boost::starts_with(resID.getName(), "CONFIG");
	});

	for (const ResourcePath & file : files)
	{
		ui32 fileChecksum = filesystem->load(file)->calculateCRC32();
		modChecksum.process_bytes(static_cast<const void *>(&fileChecksum), sizeof(fileChecksum));
	}
	return modChecksum.checksum();
}

double ModsState::getInstalledModSizeMegabytes(const TModID & modName) const
{
	ResourcePath resDir(getModDirectory(modName), EResType::DIRECTORY);
	std::string path = CResourceHandler::get()->getResourceName(resDir)->string();

	size_t sizeBytes = 0;
	for(boost::filesystem::recursive_directory_iterator it(path); it != boost::filesystem::recursive_directory_iterator(); ++it)
	{
		if(!boost::filesystem::is_directory(*it))
			sizeBytes += boost::filesystem::file_size(*it);
	}

	double sizeMegabytes = sizeBytes / static_cast<double>(1024*1024);
	return sizeMegabytes;
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

		if (ModScope::isScopeReserved(boost::to_lower_copy(name)))
			continue;

		if(!CResourceHandler::get("initial")->existsResource(JsonPath::builtin(entry.getName() + "/MOD")))
			continue;

		foundMods.push_back(name);
	}
	return foundMods;
}

///////////////////////////////////////////////////////////////////////////////

ModsPresetState::ModsPresetState()
{
	static const JsonPath settingsPath = JsonPath::builtin("config/modSettings.json");

	if(CResourceHandler::get("local")->existsResource(ResourcePath(settingsPath)))
	{
		modConfig = JsonNode(settingsPath);
	}
	else
	{
		// Probably new install. Create initial configuration
		CResourceHandler::get("local")->createResource(settingsPath.getOriginalName() + ".json");
	}

	if(modConfig["presets"].isNull())
	{
		modConfig["activePreset"] = JsonNode("default");
		if(modConfig["activeMods"].isNull())
			createInitialPreset(); // new install
		else
			importInitialPreset(); // 1.5 format import
	}
}

void ModsPresetState::createInitialPreset()
{
	// TODO: scan mods directory for all its content? Probably unnecessary since this looks like new install, but who knows?
	modConfig["presets"]["default"]["mods"].Vector().emplace_back("vcmi");
}

void ModsPresetState::importInitialPreset()
{
	JsonNode preset;

	for(const auto & mod : modConfig["activeMods"].Struct())
	{
		if(mod.second["active"].Bool())
			preset["mods"].Vector().emplace_back(mod.first);

		for(const auto & submod : mod.second["mods"].Struct())
			preset["settings"][mod.first][submod.first] = submod.second["active"];
	}
	modConfig["presets"]["default"] = preset;
}

const JsonNode & ModsPresetState::getActivePresetConfig() const
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	const JsonNode & currentPreset = modConfig["presets"][currentPresetName];
	return currentPreset;
}

TModList ModsPresetState::getActiveRootMods() const
{
	const JsonNode & modsToActivateJson = getActivePresetConfig()["mods"];
	auto modsToActivate = modsToActivateJson.convertTo<std::vector<TModID>>();
	modsToActivate.push_back(ModScope::scopeBuiltin());
	return modsToActivate;
}

std::map<TModID, bool> ModsPresetState::getModSettings(const TModID & modID) const
{
	const JsonNode & modSettingsJson = getActivePresetConfig()["settings"][modID];
	auto modSettings = modSettingsJson.convertTo<std::map<TModID, bool>>();
	return modSettings;
}

std::optional<uint32_t> ModsPresetState::getValidatedChecksum(const TModID & modName) const
{
	const JsonNode & node = modConfig["validatedMods"][modName];
	if (node.isNull())
		return std::nullopt;
	else
		return node.Integer();
}

void ModsPresetState::setModActive(const TModID & modID, bool isActive)
{
	size_t dotPos = modID.find('.');

	if(dotPos != std::string::npos)
	{
		std::string rootMod = modID.substr(0, dotPos);
		std::string settingID = modID.substr(dotPos + 1);
		setSettingActive(rootMod, settingID, isActive);
	}
	else
	{
		if (isActive)
			addRootMod(modID);
		else
			eraseRootMod(modID);
	}
}

void ModsPresetState::addRootMod(const TModID & modName)
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	JsonNode & currentPreset = modConfig["presets"][currentPresetName];

	if (!vstd::contains(currentPreset["mods"].Vector(), JsonNode(modName)))
		currentPreset["mods"].Vector().emplace_back(modName);
}

void ModsPresetState::setSettingActive(const TModID & modName, const TModID & settingName, bool isActive)
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	JsonNode & currentPreset = modConfig["presets"][currentPresetName];

	currentPreset["settings"][modName][settingName].Bool() = isActive;
}

void ModsPresetState::eraseRootMod(const TModID & modName)
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	JsonNode & currentPreset = modConfig["presets"][currentPresetName];
	vstd::erase(currentPreset["mods"].Vector(), JsonNode(modName));
}

void ModsPresetState::eraseModSetting(const TModID & modName, const TModID & settingName)
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	JsonNode & currentPreset = modConfig["presets"][currentPresetName];
	currentPreset["settings"][modName].Struct().erase(modName);
}

std::vector<TModID> ModsPresetState::getActiveMods() const
{
	TModList activeRootMods = getActiveRootMods();
	TModList allActiveMods;

	for(const auto & activeMod : activeRootMods)
	{
		allActiveMods.push_back(activeMod);

		for(const auto & submod : getModSettings(activeMod))
			if(submod.second)
				allActiveMods.push_back(activeMod + '.' + submod.first);
	}
	return allActiveMods;
}

void ModsPresetState::setValidatedChecksum(const TModID & modName, std::optional<uint32_t> value)
{
	if (value.has_value())
		modConfig["validatedMods"][modName].Integer() = *value;
	else
		modConfig["validatedMods"].Struct().erase(modName);
}

void ModsPresetState::saveConfigurationState() const
{
	std::fstream file(CResourceHandler::get()->getResourceName(ResourcePath("config/modSettings.json"))->c_str(), std::ofstream::out | std::ofstream::trunc);
	file << modConfig.toCompactString();
}

ModsStorage::ModsStorage(const std::vector<TModID> & modsToLoad, const JsonNode & repositoryList)
{
	JsonNode coreModConfig(JsonPath::builtin("config/gameConfig.json"));
	coreModConfig.setModScope(ModScope::scopeBuiltin());
	mods.try_emplace(ModScope::scopeBuiltin(), ModScope::scopeBuiltin(), coreModConfig, JsonNode());

	for(auto modID : modsToLoad)
	{
		if(ModScope::isScopeReserved(modID))
			continue;

		JsonNode modConfig(getModDescriptionFile(modID));
		modConfig.setModScope(modID);

		if(modConfig["modType"].isNull())
		{
			logMod->error("Can not load mod %s - invalid mod config file!", modID);
			continue;
		}

		mods.try_emplace(modID, modID, modConfig, repositoryList[modID]);
	}

	for(const auto & mod : repositoryList.Struct())
	{
		if (vstd::contains(modsToLoad, mod.first))
			continue;

		if (mod.second["modType"].isNull() || mod.second["name"].isNull())
			continue;

		mods.try_emplace(mod.first, mod.first, JsonNode(), mod.second);
	}
}

const ModDescription & ModsStorage::getMod(const TModID & fullID) const
{
	return mods.at(fullID);
}

TModList ModsStorage::getAllMods() const
{
	TModList result;
	for (const auto & mod : mods)
		result.push_back(mod.first);

	return result;
}

ModManager::ModManager()
	:ModManager(JsonNode())
{
}

ModManager::ModManager(const JsonNode & repositoryList)
	: modsState(std::make_unique<ModsState>())
	, modsPreset(std::make_unique<ModsPresetState>())
{
	modsStorage = std::make_unique<ModsStorage>(modsState->getInstalledMods(), repositoryList);

	eraseMissingModsFromPreset();
	addNewModsToPreset();

	std::vector<TModID> desiredModList = modsPreset->getActiveMods();
	depedencyResolver = std::make_unique<ModDependenciesResolver>(desiredModList, *modsStorage);
}

ModManager::~ModManager() = default;

const ModDescription & ModManager::getModDescription(const TModID & modID) const
{
	assert(boost::to_lower_copy(modID) == modID);
	return modsStorage->getMod(modID);
}

bool ModManager::isModActive(const TModID & modID) const
{
	return vstd::contains(getActiveMods(), modID);
}

const TModList & ModManager::getActiveMods() const
{
	return depedencyResolver->getActiveMods();
}

uint32_t ModManager::computeChecksum(const TModID & modName) const
{
	return modsState->computeChecksum(modName);
}

std::optional<uint32_t> ModManager::getValidatedChecksum(const TModID & modName) const
{
	return modsPreset->getValidatedChecksum(modName);
}

void ModManager::setValidatedChecksum(const TModID & modName, std::optional<uint32_t> value)
{
	modsPreset->setValidatedChecksum(modName, value);
}

void ModManager::saveConfigurationState() const
{
	modsPreset->saveConfigurationState();
}

TModList ModManager::getAllMods() const
{
	return modsStorage->getAllMods();
}

double ModManager::getInstalledModSizeMegabytes(const TModID & modName) const
{
	return modsState->getInstalledModSizeMegabytes(modName);
}

void ModManager::eraseMissingModsFromPreset()
{
	const TModList & installedMods = modsState->getInstalledMods();
	const TModList & rootMods = modsPreset->getActiveRootMods();

	for(const auto & rootMod : rootMods)
	{
		if(!vstd::contains(installedMods, rootMod))
		{
			modsPreset->eraseRootMod(rootMod);
			continue;
		}

		const auto & modSettings = modsPreset->getModSettings(rootMod);

		for(const auto & modSetting : modSettings)
		{
			TModID fullModID = rootMod + '.' + modSetting.first;
			if(!vstd::contains(installedMods, fullModID))
			{
				modsPreset->eraseModSetting(rootMod, modSetting.first);
				continue;
			}
		}
	}
}

void ModManager::addNewModsToPreset()
{
	const TModList & installedMods = modsState->getInstalledMods();

	for(const auto & modID : installedMods)
	{
		size_t dotPos = modID.find('.');

		if(dotPos == std::string::npos)
			continue; // only look up submods aka mod settings

		std::string rootMod = modID.substr(0, dotPos);
		std::string settingID = modID.substr(dotPos + 1);

		const auto & modSettings = modsPreset->getModSettings(rootMod);

		if (!modSettings.count(settingID))
			modsPreset->setSettingActive(rootMod, settingID, modsStorage->getMod(modID).keepDisabled());
	}
}

TModList ModManager::collectDependenciesRecursive(const TModID & modID) const
{
	TModList result;
	TModList toTest;

	toTest.push_back(modID);
	while (!toTest.empty())
	{
		TModID currentModID = toTest.back();
		const auto & currentMod = getModDescription(currentModID);
		toTest.pop_back();
		result.push_back(currentModID);

		if (!currentMod.isInstalled())
			return {}; // failure. TODO: better handling?

		for (const auto & dependency : currentMod.getDependencies())
		{
			if (!vstd::contains(result, dependency))
				toTest.push_back(dependency);
		}
	}

	return result;
}

void ModManager::tryEnableMod(const TModID & modName)
{
	auto requiredActiveMods = collectDependenciesRecursive(modName);
	auto additionalActiveMods = getActiveMods();

	assert(!vstd::contains(additionalActiveMods, modName));
	assert(vstd::contains(requiredActiveMods, modName));// FIXME: fails on attempt to enable broken mod / translation to other language

	ModDependenciesResolver testResolver(requiredActiveMods, *modsStorage);
	assert(testResolver.getBrokenMods().empty());
	assert(vstd::contains(testResolver.getActiveMods(), modName));

	testResolver.tryAddMods(additionalActiveMods, *modsStorage);

	if (!vstd::contains(testResolver.getActiveMods(), modName))
	{
		// FIXME: report?
		return;
	}

	updatePreset(testResolver);
}

void ModManager::tryDisableMod(const TModID & modName)
{
	auto desiredActiveMods = getActiveMods();
	assert(vstd::contains(desiredActiveMods, modName));

	vstd::erase(desiredActiveMods, modName);

	ModDependenciesResolver testResolver(desiredActiveMods, *modsStorage);

	if (vstd::contains(testResolver.getActiveMods(), modName))
	{
		// FIXME: report?
		return;
	}

	updatePreset(testResolver);
}

void ModManager::updatePreset(const ModDependenciesResolver & testResolver)
{
	const auto & newActiveMods = testResolver.getActiveMods();
	const auto & newBrokenMods = testResolver.getBrokenMods();

	for (const auto & modID : newActiveMods)
	{
		assert(vstd::contains(modsState->getInstalledMods(), modID));
		modsPreset->setModActive(modID, true);
	}

	for (const auto & modID : newBrokenMods)
		modsPreset->setModActive(modID, false);

	std::vector<TModID> desiredModList = modsPreset->getActiveMods();
	depedencyResolver = std::make_unique<ModDependenciesResolver>(desiredModList, *modsStorage);

	// TODO: check activation status of submods of new mods

	modsPreset->saveConfigurationState();
}

ModDependenciesResolver::ModDependenciesResolver(const TModList & modsToResolve, const ModsStorage & storage)
{
	tryAddMods(modsToResolve, storage);
}

const TModList & ModDependenciesResolver::getActiveMods() const
{
	return activeMods;
}

const TModList & ModDependenciesResolver::getBrokenMods() const
{
	return brokenMods;
}

void ModDependenciesResolver::tryAddMods(TModList modsToResolve, const ModsStorage & storage)
{
	// Topological sort algorithm.
	boost::range::sort(modsToResolve); // Sort mods per name
	std::vector<TModID> sortedValidMods(activeMods.begin(), activeMods.end()); // Vector keeps order of elements (LIFO)
	std::set<TModID> resolvedModIDs(activeMods.begin(), activeMods.end()); // Use a set for validation for performance reason, but set does not keep order of elements
	std::set<TModID> notResolvedModIDs(modsToResolve.begin(), modsToResolve.end()); // Use a set for validation for performance reason

	// Mod is resolved if it has no dependencies or all its dependencies are already resolved
	auto isResolved = [&](const ModDescription & mod) -> bool
	{
		if (mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage())
			return false;

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
			if(vstd::contains(storage.getMod(reverseConflict).getConflicts(), mod.getID()))
				return false;

		return true;
	};

	while(true)
	{
		std::set<TModID> resolvedOnCurrentTreeLevel;
		for(auto it = modsToResolve.begin(); it != modsToResolve.end();) // One iteration - one level of mods tree
		{
			if(isResolved(storage.getMod(*it)))
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

	assert(!sortedValidMods.empty());
	activeMods = sortedValidMods;
	brokenMods.insert(brokenMods.end(), modsToResolve.begin(), modsToResolve.end());
}

VCMI_LIB_NAMESPACE_END
