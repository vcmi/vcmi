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

	TModList getAllMods() const;
};

/// Provides interface to access or change current mod preset
class ModsPresetState : boost::noncopyable
{
	JsonNode modConfig;

	void saveConfiguration(const JsonNode & config);

	void createInitialPreset();
	void importInitialPreset();

public:
	ModsPresetState();

	/// Returns true if mod is active in current preset
	bool isModActive(const TModID & modName) const;

	void activateModInPreset(const TModID & modName);
	void dectivateModInAllPresets(const TModID & modName);

	/// Returns list of all mods active in current preset. Mod order is unspecified
	TModList getActiveMods() const;
};

/// Provides access to mod properties
class ModsStorage : boost::noncopyable
{
	std::map<TModID, ModDescription> mods;

public:
	ModsStorage(const TModList & modsToLoad);

	const ModDescription & getMod(const TModID & fullID) const;
};

/// Provides public interface to access mod state
class ModManager : boost::noncopyable
{
	/// all currently active mods, in their load order
	TModList activeMods;

	/// Mods from current preset that failed to load due to invalid dependencies
	TModList brokenMods;

	std::unique_ptr<ModsState> modsState;
	std::unique_ptr<ModsPresetState> modsPreset;
	std::unique_ptr<ModsStorage> modsStorage;

	void generateLoadOrder(TModList desiredModList);

public:
	ModManager();
	~ModManager();

	const ModDescription & getModDescription(const TModID & modID) const;
	const TModList & getActiveMods() const;
	bool isModActive(const TModID & modID) const;
};

VCMI_LIB_NAMESPACE_END
