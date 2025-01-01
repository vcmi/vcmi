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

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class ModDescription;
struct CModVersion;

using TModID = std::string;
using TModList = std::vector<TModID>;
using TModSet = std::set<TModID>;

/// Provides interface to access list of locally installed mods
class ModsState : boost::noncopyable
{
	TModList modList;

	TModList scanModsDirectory(const std::string & modDir) const;

public:
	ModsState();

	TModList getInstalledMods() const;
	double getInstalledModSizeMegabytes(const TModID & modName) const;

	uint32_t computeChecksum(const TModID & modName) const;
};

/// Provides interface to access or change current mod preset
class ModsPresetState : boost::noncopyable
{
	JsonNode modConfig;

	void createInitialPreset();
	void importInitialPreset();

	const JsonNode & getActivePresetConfig() const;

public:
	ModsPresetState();

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
	std::optional<uint32_t> getValidatedChecksum(const TModID & modName) const;
	void setValidatedChecksum(const TModID & modName, std::optional<uint32_t> value);

	void saveConfigurationState() const;
};

/// Provides access to mod properties
class ModsStorage : boost::noncopyable
{
	std::map<TModID, ModDescription> mods;

public:
	ModsStorage(const TModList & modsToLoad, const JsonNode & repositoryList);

	const ModDescription & getMod(const TModID & fullID) const;

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
	std::unique_ptr<ModsStorage> modsStorage;
	std::unique_ptr<ModDependenciesResolver> depedencyResolver;

	void generateLoadOrder(TModList desiredModList);
	void eraseMissingModsFromPreset();
	void addNewModsToPreset();
	void updatePreset(const ModDependenciesResolver & newData);

	TModList getInstalledValidMods() const;
	TModList collectDependenciesRecursive(const TModID & modID) const;

	void tryEnableMod(const TModID & modList);

public:
	ModManager(const JsonNode & repositoryList);
	ModManager();
	~ModManager();

	const ModDescription & getModDescription(const TModID & modID) const;
	const TModList & getActiveMods() const;
	TModList getAllMods() const;

	bool isModSettingActive(const TModID & rootModID, const TModID & modSettingID) const;
	bool isModActive(const TModID & modID) const;
	uint32_t computeChecksum(const TModID & modName) const;
	std::optional<uint32_t> getValidatedChecksum(const TModID & modName) const;
	void setValidatedChecksum(const TModID & modName, std::optional<uint32_t> value);
	void saveConfigurationState() const;
	double getInstalledModSizeMegabytes(const TModID & modName) const;

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

VCMI_LIB_NAMESPACE_END
