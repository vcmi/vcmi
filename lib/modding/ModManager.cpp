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

static std::string getModDirectory(const TModID & modName)
{
	std::string result = modName;
	boost::to_upper(result);
	boost::algorithm::replace_all(result, ".", "/MODS/");
	return "MODS/" + result;
}

/// Converts path of a mod definition file, e.g. 'MODS/HOTA/MODS/MAPS/MOD', into mod ID, e.g. 'hota.maps'
/// Returns nullopt if the path is not a valid mod location, e.g. a 'mod.json' shipped as mod content
static std::optional<TModID> getModIDFromDefinitionFile(const std::string & path)
{
	std::vector<std::string> segments;
	boost::split(segments, path, boost::is_any_of("/"));

	// expected form: MODS / <mod> [ / MODS / <submod> ]... / MOD
	if(segments.size() < 3 || segments.size() % 2 == 0 || segments.back() != "MOD")
		return std::nullopt;

	TModID result;
	for(size_t i = 0; i < segments.size() - 1; i += 2)
	{
		if(segments[i] != "MODS")
			return std::nullopt;

		std::string name = boost::to_lower_copy(segments[i + 1]);

		// '.' separates mod from its submods, so it can not be part of a mod name
		if(name.empty() || name.find('.') != std::string::npos || ModScope::isScopeReserved(name))
			return std::nullopt;

		if(!result.empty())
			result += '.';
		result += name;
	}
	return result;
}

/// Returns true if every mod that the specified submod is nested in is a mod on its own
static bool hasValidParentMods(const TModID & modID, const std::set<TModID> & candidates)
{
	for(size_t separator = modID.find('.'); separator != std::string::npos; separator = modID.find('.', separator + 1))
		if(!candidates.count(modID.substr(0, separator)))
			return false;

	return true;
}

static JsonPath getModDefinitionFile(const TModID & modName)
{
	return JsonPath::builtin(getModDirectory(modName) + "/mod");
}

static TextPath getModDescriptionFile(const TModID & modName)
{
	return TextPath::builtin(getModDirectory(modName) + "/description");
}

ModsState::ModsState()
{
	modList.push_back(ModScope::scopeBuiltin());

	// every mod is identified by its definition file, so a single scan of game data locates all of them
	auto definitionFiles = CResourceHandler::get("initial")->getFilteredFiles([](const ResourcePath & id)
	{
		return id.getType() == EResType::JSON && boost::algorithm::ends_with(id.getName(), "/MOD");
	});

	// sorted, so that mod order does not depend on iteration order of an unordered container
	std::set<TModID> candidates;
	for(const auto & definitionFile : definitionFiles)
	{
		auto modID = getModIDFromDefinitionFile(definitionFile.getName());
		if(modID)
			candidates.insert(*modID);
	}

	for(const auto & modID : candidates)
		if(hasValidParentMods(modID, candidates))
			modList.push_back(modID);
}

TModList ModsState::getInstalledMods() const
{
	return modList;
}

double ModsState::getInstalledModSizeMegabytes(const TModID & modName) const
{
	ResourcePath resDir(getModDirectory(modName), EResType::DIRECTORY);
	// iterate over the path itself - a detour through std::string would convert it using the
	// Windows ANSI codepage, replacing any character the codepage lacks with '?'. The directory
	// would then not exist and recursive_directory_iterator would throw
	const boost::filesystem::path path = *CResourceHandler::get()->getResourceName(resDir);

	size_t sizeBytes = 0;
	for(boost::filesystem::recursive_directory_iterator it(path); it != boost::filesystem::recursive_directory_iterator(); ++it)
	{
		if(!boost::filesystem::is_directory(*it))
			sizeBytes += boost::filesystem::file_size(*it);
	}

	double sizeMegabytes = sizeBytes / static_cast<double>(1024*1024);
	return sizeMegabytes;
}

///////////////////////////////////////////////////////////////////////////////

ModsPresetState::ModsPresetState(bool useTestPreset)
	: settingsPath(JsonPath::builtin(useTestPreset ? "config/testModSettings.json" : "config/modSettings.json"))
{
	if(CResourceHandler::get("local")->existsResource(ResourcePath(settingsPath)))
	{
		modConfig = JsonNode(settingsPath);
	}
	else
	{
		// Probably new install. Create initial configuration
		CResourceHandler::get("local")->createResource(settingsPath.getOriginalName() + ".json");
	}

	if(modConfig["presets"].isNull() || modConfig["presets"].Struct().empty())
	{
		modConfig["activePreset"] = JsonNode("default");
		if(modConfig["activeMods"].isNull())
			createInitialPreset(); // new install
		else
			importInitialPreset(); // 1.5 format import
	}

	auto allPresets = getAllPresets();
	if (!vstd::contains(allPresets, modConfig["activePreset"].String()))
		modConfig["activePreset"] = JsonNode(allPresets.front());

	logGlobal->debug("Loading following mod settings: %s", modConfig.toCompactString());
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
	return getRootMods(getActivePreset());
}

TModList ModsPresetState::getRootMods(const std::string & presetName) const
{
	const JsonNode & modsToActivateJson = modConfig["presets"][presetName]["mods"];
	auto modsToActivate = modsToActivateJson.convertTo<std::vector<TModID>>();
	if (!vstd::contains(modsToActivate, ModScope::scopeBuiltin()))
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

void ModsPresetState::removeOldMods(const TModList & modsToKeep)
{
	const std::string & currentPresetName = modConfig["activePreset"].String();
	JsonNode & currentPreset = modConfig["presets"][currentPresetName];

	vstd::erase_if(currentPreset["mods"].Vector(), [&](const JsonNode & entry){
		return !vstd::contains(modsToKeep, entry.String());
	});

	vstd::erase_if(currentPreset["settings"].Struct(), [&](const auto & entry){
		return !vstd::contains(modsToKeep, entry.first);
	});

	for (auto & modSettings : currentPreset["settings"].Struct())
	{
		vstd::erase_if(modSettings.second.Struct(), [&](const auto & entry){
			return !vstd::contains(modsToKeep, modSettings.first + "." + entry.first);
		});
	}
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
	currentPreset["settings"][modName].Struct().erase(settingName);
}

std::vector<TModID> ModsPresetState::getActiveMods() const
{
	TModList activeRootMods = getActiveRootMods();
	TModList allActiveMods;

	for(const auto & activeMod : activeRootMods)
	{
		assert(!vstd::contains(allActiveMods, activeMod));
		allActiveMods.push_back(activeMod);

		for(const auto & submod : getModSettings(activeMod))
		{
			if(submod.second)
			{
				assert(!vstd::contains(allActiveMods, activeMod + '.' + submod.first));
				allActiveMods.push_back(activeMod + '.' + submod.first);
			}
		}
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
	std::fstream file(CResourceHandler::get()->getResourceName(ResourcePath(settingsPath))->c_str(), std::ofstream::out | std::ofstream::trunc);
	file << modConfig.toCompactString();
}

void ModsPresetState::createNewPreset(const std::string & presetName)
{
	if (modConfig["presets"][presetName].isNull())
		modConfig["presets"][presetName]["mods"].Vector().emplace_back("vcmi");
}

void ModsPresetState::deletePreset(const std::string & presetName)
{
	if (modConfig["presets"].Struct().size() < 2)
		throw std::runtime_error("Unable to delete last preset!");

	modConfig["presets"].Struct().erase(presetName);
}

void ModsPresetState::activatePreset(const std::string & presetName)
{
	if (modConfig["presets"].Struct().count(presetName) == 0)
		throw std::runtime_error("Unable to activate non-exinsting preset!");

	modConfig["activePreset"].String() = presetName;
}

void ModsPresetState::renamePreset(const std::string & oldPresetName, const std::string & newPresetName)
{
	if (oldPresetName == newPresetName)
		throw std::runtime_error("Unable to rename preset to the same name!");

	if (modConfig["presets"].Struct().count(oldPresetName) == 0)
		throw std::runtime_error("Unable to rename non-existing last preset!");

	if (modConfig["presets"].Struct().count(newPresetName) != 0)
		throw std::runtime_error("Unable to rename preset - preset with such name already exists!");

	modConfig["presets"][newPresetName] = modConfig["presets"][oldPresetName];
	modConfig["presets"].Struct().erase(oldPresetName);

	if (modConfig["activePreset"].String() == oldPresetName)
		modConfig["activePreset"].String() = newPresetName;
}

std::vector<std::string> ModsPresetState::getAllPresets() const
{
	std::vector<std::string> presets;

	for (const auto & preset : modConfig["presets"].Struct())
		presets.push_back(preset.first);

	return presets;
}

std::string ModsPresetState::getActivePreset() const
{
	return modConfig["activePreset"].String();
}

JsonNode ModsPresetState::exportCurrentPreset() const
{
	JsonNode data = getActivePresetConfig();
	std::string presetName = getActivePreset();

	data["name"] = JsonNode(presetName);

	vstd::erase_if(data["settings"].Struct(), [&](const auto & pair){
		return !vstd::contains(data["mods"].Vector(), JsonNode(pair.first));
	});

	return data;
}

std::string ModsPresetState::importPreset(const JsonNode & newConfig)
{
	std::string importedPresetName = newConfig["name"].String();

	if (importedPresetName.empty())
		throw std::runtime_error("Attempt to import invalid preset");

	modConfig["presets"][importedPresetName] = newConfig;
	modConfig["presets"][importedPresetName].Struct().erase("name");

	return importedPresetName;
}

ModsStorage::ModsStorage(const std::vector<TModID> & modsToLoad, const JsonNode & repositoryList)
{
	JsonNode coreModConfig(JsonPath::builtin("config/gameConfig.json"));
	coreModConfig.setModScope(ModScope::scopeBuiltin());
	mods.try_emplace(ModScope::scopeBuiltin(), ModScope::scopeBuiltin(), coreModConfig, JsonNode());

	// MODS COMPATIBILITY: in 1.6, repository list contains mod list directly, in 1.7 it is located in 'availableMods' node
	const auto & availableRepositoryMods = repositoryList["availableMods"].isNull() ? repositoryList : repositoryList["availableMods"];

	for(auto modID : modsToLoad)
	{
		if(ModScope::isScopeReserved(modID))
			continue;

		JsonNode modConfig(getModDefinitionFile(modID));
		modConfig.setModScope(modID);

		if(modConfig["modType"].isNull())
		{
			logMod->error("Can not load mod %s - invalid mod config file!", modID);
			continue;
		}

		// existsResource may report a stale entry (e.g. mod updated to a version without description);
		// tolerate a failing load since description is optional and index/disk can diverge until reload
		if (CResourceHandler::get()->existsResource(getModDescriptionFile(modID)))
		{
			try
			{
				auto data = CResourceHandler::get()->load(getModDescriptionFile(modID))->readAll();
				std::string modDescriptions(reinterpret_cast<const char *>(data.first.get()), data.second);
				ModDescription::mergeModDescriptions(modConfig, modDescriptions);
			}
			catch (const std::exception & e)
			{
				logMod->warn("Failed to load description for mod %s: %s", modID, e.what());
			}
		}

		mods.try_emplace(modID, modID, modConfig, availableRepositoryMods[modID]);
	}

	for(const auto & mod : availableRepositoryMods.Struct())
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
	try {
		return mods.at(fullID);
	}
	catch (const std::out_of_range & )
	{
		// rethrow with better error message
		throw std::out_of_range("Failed to find mod " + fullID);
	}
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

ModManager::ModManager(const JsonNode & repositoryList, bool useTestPreset)
	: modsState(std::make_unique<ModsState>())
	, modsPreset(std::make_unique<ModsPresetState>(useTestPreset))
{
	modsStorage = std::make_unique<ModsStorage>(modsState->getInstalledMods(), repositoryList);

	eraseMissingModsFromPreset();
	addNewModsToPreset();

	// Sync roe-demo enabled state with demo data presence
	syncDemoModState();

	std::vector<TModID> desiredModList = modsPreset->getActiveMods();
	// Force-activate the test fixtures mod regardless of any persisted preset or its
	// keepDisabled flag; the preset file lives in the user config dir and survives runs.
	if(useTestPreset && !vstd::contains(desiredModList, "vcmi-test"))
		desiredModList.push_back("vcmi-test");
	ModDependenciesResolver newResolver(desiredModList, *modsStorage);
	updatePreset(newResolver);
}

ModManager::~ModManager() = default;

const ModDescription & ModManager::getModDescription(const TModID & modID) const
{
	assert(boost::to_lower_copy(modID) == modID);
	return modsStorage->getMod(modID);
}

bool ModManager::isModSettingActive(const TModID & rootModID, const TModID & modSettingID) const
{
	return modsPreset->getModSettings(rootModID).at(modSettingID);
}

std::map<TModID, bool> ModManager::getModSettings(const TModID & rootModID) const
{
	return modsPreset->getModSettings(rootModID);
}

bool ModManager::isModActive(const TModID & modID) const
{
	return vstd::contains(getActiveMods(), modID);
}

const TModList & ModManager::getActiveMods() const
{
	return depedencyResolver->getActiveMods();
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
	const TModList & installedMods = getInstalledValidMods();
	const TModList & rootMods = modsPreset->getActiveRootMods();

	modsPreset->removeOldMods(installedMods);

	for(const auto & rootMod : rootMods)
	{
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
	const TModList & installedMods = getInstalledValidMods();

	for(const auto & modID : installedMods)
	{
		size_t dotPos = modID.find('.');

		if(dotPos == std::string::npos)
			continue; // only look up submods aka mod settings

		std::string rootMod = modID.substr(0, dotPos);
		std::string settingID = modID.substr(dotPos + 1);

		const auto & modSettings = modsPreset->getModSettings(rootMod);

		if (!modSettings.count(settingID))
			modsPreset->setSettingActive(rootMod, settingID, !modsStorage->getMod(modID).keepDisabled());
	}
}

void ModManager::syncDemoModState()
{
	static const TModID demoModID = "roe-demo";

	if (!vstd::contains(modsStorage->getAllMods(), demoModID))
		return;

	if (!modsStorage->getMod(demoModID).isInstalled())
		return;

	bool hasFullData = CResourceHandler::get()->existsResource(ResourcePath("DATA/TENTCOLR.TXT"));
	bool hasDemoData = !hasFullData && CResourceHandler::get()->existsResource(ResourcePath("MAPS/H3DEMO.H3M"));

	bool isActive = vstd::contains(modsPreset->getActiveRootMods(), demoModID);

	if (hasDemoData && !isActive)
		modsPreset->setModActive(demoModID, true);
	else if (!hasDemoData && isActive)
		modsPreset->setModActive(demoModID, false);
}

TModList ModManager::getInstalledValidMods() const
{
	TModList installedMods = modsState->getInstalledMods();
	TModList validMods = modsStorage->getAllMods();

	TModList result;
	for (const auto & modID : installedMods)
		if (vstd::contains(validMods, modID))
			result.push_back(modID);

	return result;
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
			throw std::runtime_error("Unable to enable mod " + modID + "! Dependency " + currentModID + " is not installed!");

		for (const auto & dependency : currentMod.getDependencies())
		{
			if (!vstd::contains(result, dependency))
				toTest.push_back(dependency);
		}
	}

	return result;
}

void ModManager::tryEnableMods(const TModList & modList)
{
	TModList requiredActiveMods;
	TModList additionalActiveMods = getActiveMods();

	for (const auto & modName : modList)
	{
		for (const auto & dependency : collectDependenciesRecursive(modName))
		{
			if (!vstd::contains(requiredActiveMods, dependency))
			{
				requiredActiveMods.push_back(dependency);
				vstd::erase(additionalActiveMods, dependency);
			}
		}

		assert(!vstd::contains(additionalActiveMods, modName));
		assert(vstd::contains(requiredActiveMods, modName));// FIXME: fails on attempt to enable broken mod / translation to other language
	}

	ModDependenciesResolver testResolver(requiredActiveMods, *modsStorage);

	testResolver.tryAddMods(additionalActiveMods, *modsStorage);

	TModList additionalActiveSubmods;
	for (const auto & modName : modList)
	{
		if (modName.find('.') != std::string::npos)
			continue;

		auto modSettings = modsPreset->getModSettings(modName);
		for (const auto & entry : modSettings)
		{
			TModID fullModID = modName + '.' + entry.first;
			if (entry.second && !vstd::contains(requiredActiveMods, fullModID))
				additionalActiveSubmods.push_back(fullModID);
		}
	}

	testResolver.tryAddMods(additionalActiveSubmods, *modsStorage);

	for (const auto & modName : modList)
		if (!vstd::contains(testResolver.getActiveMods(), modName))
			logGlobal->error("Failed to enable mod '%s'! This may be caused by a recursive dependency!", modName);

	updatePreset(testResolver);
}

void ModManager::tryDisableMod(const TModID & modName)
{
	auto desiredActiveMods = getActiveMods();
	assert(vstd::contains(desiredActiveMods, modName));

	vstd::erase(desiredActiveMods, modName);

	ModDependenciesResolver testResolver(desiredActiveMods, *modsStorage);

	if (vstd::contains(testResolver.getActiveMods(), modName))
		throw std::runtime_error("Failed to disable mod! Mod " + modName + " remains enabled!");

	modsPreset->setModActive(modName, false);
	updatePreset(testResolver);
}

void ModManager::updatePreset(const ModDependenciesResolver & testResolver)
{
	const auto & newActiveMods = testResolver.getActiveMods();
	const auto & newBrokenMods = testResolver.getBrokenMods();

	for (const auto & modID : newActiveMods)
	{
		assert(vstd::contains(getInstalledValidMods(), modID));
		modsPreset->setModActive(modID, true);
	}

	for (const auto & modID : newBrokenMods)
	{
		const auto & mod = getModDescription(modID);
		if (mod.getTopParentID().empty() || vstd::contains(newActiveMods, mod.getTopParentID()))
			modsPreset->setModActive(modID, false);
	}

	std::vector<TModID> desiredModList = modsPreset->getActiveMods();

	// Try to enable all existing compatibility patches. Ignore on failure
	for (const auto & rootMod : modsPreset->getActiveRootMods())
	{
		for (const auto & modSetting : modsPreset->getModSettings(rootMod))
		{
			if (modSetting.second)
				continue;

			TModID fullModID = rootMod + '.' + modSetting.first;
			const auto & modDescription = modsStorage->getMod(fullModID);

			if (modDescription.isCompatibility())
				desiredModList.push_back(fullModID);
		}
	}

	depedencyResolver = std::make_unique<ModDependenciesResolver>(desiredModList, *modsStorage);
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
	std::ranges::sort(modsToResolve); // Sort mods per name
	std::vector<TModID> sortedValidMods(activeMods.begin(), activeMods.end()); // Vector keeps order of elements (LIFO)
	std::set<TModID> resolvedModIDs(activeMods.begin(), activeMods.end()); // Use a set for validation for performance reason, but set does not keep order of elements
	std::set<TModID> notResolvedModIDs(modsToResolve.begin(), modsToResolve.end()); // Use a set for validation for performance reason

	enum class ModResolveStatus {
		RESOLVED, // ok - mod can be added to load order
		WAITING, // maybe - wait for more iterations before deciding
		BROKEN // fail - this mod definitely can't be loaded
	};

	// Mod is resolved if it has no dependencies or all its dependencies are already resolved
	auto isResolved = [&](const ModDescription & mod) -> ModResolveStatus
	{
		if (mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage())
			return ModResolveStatus::BROKEN;

		if(!mod.isCompatible())
			return ModResolveStatus::BROKEN;

		for(const TModID & dependency : mod.getDependencies())
		{
			if (vstd::contains(sortedValidMods, dependency))
				continue;

			if (vstd::contains(notResolvedModIDs, dependency))
				return ModResolveStatus::WAITING;

			// either not in load list, or dependency is also broken
			return ModResolveStatus::BROKEN;
		}

		for(const TModID & softDependency : mod.getSoftDependencies())
			if(vstd::contains(notResolvedModIDs, softDependency))
				return ModResolveStatus::WAITING;

		for(const TModID & conflict : mod.getConflicts())
			if(vstd::contains(resolvedModIDs, conflict))
				return ModResolveStatus::BROKEN;

		for(const TModID & reverseConflict : resolvedModIDs)
			if(vstd::contains(storage.getMod(reverseConflict).getConflicts(), mod.getID()))
				return ModResolveStatus::BROKEN;

		return ModResolveStatus::RESOLVED;
	};

	while(true)
	{
		std::set<TModID> resolvedOnCurrentTreeLevel;
		for(auto it = modsToResolve.begin(); it != modsToResolve.end();) // One iteration - one level of mods tree
		{
			ModResolveStatus status = isResolved(storage.getMod(*it));

			if (status == ModResolveStatus::RESOLVED)
			{
				resolvedOnCurrentTreeLevel.insert(*it); // Not to the resolvedModIDs, so current node children will be resolved on the next iteration
				assert(!vstd::contains(sortedValidMods, *it));
				sortedValidMods.push_back(*it);
				it = modsToResolve.erase(it);
				continue;
			}
			if (status == ModResolveStatus::BROKEN)
			{
				resolvedOnCurrentTreeLevel.insert(*it);
				brokenMods.push_back(*it);
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

void ModManager::createNewPreset(const std::string & presetName)
{
	modsPreset->createNewPreset(presetName);
	modsPreset->saveConfigurationState();
}

void ModManager::deletePreset(const std::string & presetName)
{
	modsPreset->deletePreset(presetName);
	modsPreset->saveConfigurationState();
}

void ModManager::activatePreset(const std::string & presetName)
{
	modsPreset->activatePreset(presetName);
	modsPreset->saveConfigurationState();
}

void ModManager::renamePreset(const std::string & oldPresetName, const std::string & newPresetName)
{
	modsPreset->renamePreset(oldPresetName, newPresetName);
	modsPreset->saveConfigurationState();
}

std::vector<std::string> ModManager::getAllPresets() const
{
	return modsPreset->getAllPresets();
}

std::string ModManager::getActivePreset() const
{
	return modsPreset->getActivePreset();
}

JsonNode ModManager::exportCurrentPreset() const
{
	return modsPreset->exportCurrentPreset();
}

std::tuple<std::string, TModList> ModManager::importPreset(const JsonNode & data)
{
	std::string presetName = modsPreset->importPreset(data);

	TModList requiredMods = modsPreset->getRootMods(presetName);
	TModList installedMods = getInstalledValidMods();

	TModList missingMods;
	for (const auto & modID : requiredMods)
	{
		if (!vstd::contains(installedMods, modID))
			missingMods.push_back(modID);
	}

	modsPreset->saveConfigurationState();

	return {presetName, missingMods};
}
