/*
 * ModManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../json/JsonNode.h"
#include "../filesystem/ResourcePath.h"

class JsonNode;
class ModDescription;
class ModSnapshot;
struct CModVersion;

using TModID = std::string;
using TModList = std::vector<TModID>;
using TModSet = std::set<TModID>;

/// Provides interface to access list of locally installed mods
class ModsState : boost::noncopyable
{
	TModList modList;

public:
	ModsState();

	TModList getInstalledMods() const;
	double getInstalledModSizeMegabytes(const TModID & modName) const;
};

/// Provides interface to access or change current mod preset
class ModsPresetState : boost::noncopyable
{
	JsonNode modConfig;
	JsonPath settingsPath; // which mod-settings file we read AND write

	void createInitialPreset();
	void importInitialPreset();

	const JsonNode & getActivePresetConfig() const;

public:
	explicit ModsPresetState(bool useTestPreset = false);

	void createNewPreset(const std::string & presetName);
	void deletePreset(const std::string & presetName);
	void activatePreset(const std::string & presetName);
	void renamePreset(const std::string & oldPresetName, const std::string & newPresetName);

	std::vector<std::string> getAllPresets() const;
	std::string getActivePreset() const;

	JsonNode exportCurrentPreset() const;

	/// Imports preset from provided json
	/// Returns name of imported preset on success
	std::string importPreset(const JsonNode & data);

	void setModActive(const TModID & modName, bool isActive);

	void addRootMod(const TModID & modName);
	void eraseRootMod(const TModID & modName);
	void removeOldMods(const TModList & modsToKeep);

	void setSettingActive(const TModID & modName, const TModID & settingName, bool isActive);
	void eraseModSetting(const TModID & modName, const TModID & settingName);

	/// Returns list of all mods active in current preset. Mod order is unspecified
	TModList getActiveMods() const;

	/// Returns list of currently active root mods (non-submod)
	TModList getActiveRootMods() const;
	/// Returns list of root mods present in specified preset
	TModList getRootMods(const std::string & presetName) const;

	/// Returns list of all known settings (submods) for a specified mod
	std::map<TModID, bool> getModSettings(const TModID & modID) const;

	void saveConfigurationState() const;
};

/// Stores checksums of mods that have passed validation, to skip validation of unchanged mods on next load
class ModsValidationState : boost::noncopyable
{
	JsonNode checksums;

public:
	ModsValidationState();

	std::map<TModID, uint32_t> getValidatedChecksums() const;
	void setValidatedChecksum(const TModID & modName, std::optional<uint32_t> value);

	void saveConfigurationState() const;
};

/// Provides access to mod properties
class ModsStorage : boost::noncopyable
{
	std::map<TModID, ModDescription> mods;

	/// Root mods that affect gameplay themselves or through any of their submods
	std::set<TModID> gameplayRoots;

public:
	ModsStorage(const TModList & modsToLoad, const JsonNode & repositoryList);

	const ModDescription & getMod(const TModID & fullID) const;

	/// Returns true if root mod or any of its submods affects gameplay
	bool isGameplayRoot(const TModID & rootID) const;

	TModList getAllMods() const;
};

class ModDependenciesResolver : boost::noncopyable
{
	/// all currently active mods, in their load order
	TModList activeMods;

	/// Mods from current preset that failed to load due to invalid dependencies
	TModList brokenMods;

public:
	ModDependenciesResolver(const TModList & modsToResolve, const ModsStorage & storage);

	void tryAddMods(TModList modsToResolve, const ModsStorage & storage);

	const TModList & getActiveMods() const;
	const TModList & getBrokenMods() const;
};

/// Provides public interface to access mod state
class DLL_LINKAGE ModManager : boost::noncopyable
{
	std::unique_ptr<ModsState> modsState;
	std::unique_ptr<ModsPresetState> modsPreset;
	std::unique_ptr<ModsValidationState> modsValidation;
	std::shared_ptr<const ModsStorage> modsStorage;
	std::unique_ptr<ModDependenciesResolver> depedencyResolver;

	void generateLoadOrder(TModList desiredModList);
	void eraseMissingModsFromPreset();
	void addNewModsToPreset();
	void syncDemoModState();
	void updatePreset(const ModDependenciesResolver & newData);

	TModList getInstalledValidMods() const;
	/// Returns mod and all mods it depends on, directly or not. Includes dependencies that are not installed, but not their own dependencies
	TModList collectDependenciesRecursive(const TModID & modID) const;
	/// Returns submods of provided root mods that are enabled in active preset
	TModList getEnabledSubmodsFromPreset(const TModList & rootMods, bool includeGameplaySubmods) const;
	/// Resolves provided mods that resolver has not processed yet
	void tryAddNewMods(ModDependenciesResolver & resolver, const TModList & mods) const;

	void tryEnableMod(const TModID & modList);

public:
	/// newMods - installed mods to activate together with mods from active preset
	ModManager(const JsonNode & repositoryList, bool useTestPreset = false, const TModList & newMods = {});
	ModManager();
	~ModManager();

	const ModDescription & getModDescription(const TModID & modID) const;
	const TModList & getActiveMods() const;
	TModList getAllMods() const;

	bool isModSettingActive(const TModID & rootModID, const TModID & modSettingID) const;
	std::map<TModID, bool> getModSettings(const TModID & rootModID) const;
	bool isModActive(const TModID & modID) const;
	double getInstalledModSizeMegabytes(const TModID & modName) const;

	/// Returns mod list for a game library, with currently active mods
	ModSnapshot createSnapshot() const;

	/// Returns mod list for a game library that loads a game with provided mods, in the form of "mods" section of scenario metadata. Active preset is not changed.
	/// exact: gameplay mods missing from the list are disabled (saves); otherwise active mods are kept (maps).
	/// Required mods that can not be enabled are left disabled, see ScenarioMetadata::verifyMods
	ModSnapshot createSnapshot(const JsonNode & requiredMods, bool exact) const;

	/// Stores result of validation of mods by a loaded game library: checksum of a mod without issues, or nothing for a mod with issues
	void setValidatedChecksums(const std::map<TModID, std::optional<uint32_t>> & checksums);

	void tryEnableMods(const TModList & modList);
	void tryDisableMod(const TModID & modName);

	void createNewPreset(const std::string & presetName);
	void deletePreset(const std::string & presetName);
	void activatePreset(const std::string & presetName);
	void renamePreset(const std::string & oldPresetName, const std::string & newPresetName);

	std::vector<std::string> getAllPresets() const;
	std::string getActivePreset() const;

	JsonNode exportCurrentPreset() const;

	/// Imports preset from provided json
	/// Returns name of imported preset and list of mods that must be installed to activate preset
	std::tuple<std::string, TModList> importPreset(const JsonNode & data);
};
