#pragma once

#include "Filesystem/CResourceLoader.h"

#include "VCMI_Lib.h"

/*
 * CModHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CModHandler;
class CModIndentifier;
class CModInfo;
class JsonNode;

/// class that stores all object identifiers strings and maps them to numeric ID's
/// if possible, objects ID's should be in format <type>.<name>, camelCase e.g. "creature.grandElf"
class CIdentifierStorage
{
	std::map<std::string, si32 > registeredObjects;
	std::map<std::string, std::vector<boost::function<void(si32)> > > missingObjects;

	//Check if identifier can be valid (camelCase, point as separator)
	void checkIdentifier(std::string & ID);
public:
	/// request identifier for specific object name. If ID is not yet resolved callback will be queued
	/// and will be called later
	void requestIdentifier(std::string name, const boost::function<void(si32)> & callback);
	/// registers new object, calls all associated callbacks
	void registerObject(std::string name, si32 identifier);

	/// called at the very end of loading to check for any missing ID's
	void finalize() const;
};

typedef std::string TModID;

class DLL_LINKAGE CModInfo
{
public:
	/// identifier, identical to name of folder with mod
	std::string identifier;

	/// human-readable strings
	std::string name;
	std::string description;

	/// list of mods that should be loaded before this one
	std::set <TModID> dependencies;

	/// list of mods that can't be used in the same time as this one
	std::set <TModID> conflicts;

	// mod configuration (mod.json). (no need to store it right now)
	// std::shared_ptr<JsonNode> config; //TODO: unique_ptr can't be serialized

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & description & name & dependencies & conflicts;
	}
};

class DLL_LINKAGE CModHandler
{
	std::map <TModID, CModInfo> allMods;
	std::vector <TModID> activeMods;//active mods, in order in which they were loaded

	void loadConfigFromFile (std::string name);

	bool hasCircularDependency(TModID mod, std::set <TModID> currentList = std::set <TModID>()) const;

	//returns false if mod list is incorrect and prints error to console. Possible errors are:
	// - missing dependency mod
	// - conflicting mod in load order
	// - circular dependencies
	bool checkDependencies(const std::vector <TModID> & input) const;

	// returns load order in which all dependencies are resolved, e.g. loaded after required mods
	// function assumes that input list is valid (checkDependencies returned true)
	std::vector <TModID> resolveDependencies(std::vector<TModID> input) const;

	// helper for loadActiveMods. Loads content from list of files
	template<typename Handler>
	void handleData(Handler handler, const JsonNode & source, std::string listName, std::string schemaName);
public:

	CIdentifierStorage identifiers;

	/// receives list of available mods and trying to load mod.json from all of them
	void initialize(std::vector<std::string> availableMods);

	/// returns list of mods that should be active with order in which they shoud be loaded
	std::vector<std::string> getActiveMods();

	/// load content from all available mods
	void loadActiveMods();

	/// actions that should be triggered on map restart
	/// TODO: merge into appropriate handlers?
	void reload();

	struct DLL_LINKAGE hardcodedFeatures
	{
		int CREEP_SIZE; // neutral stacks won't grow beyond this number
		int WEEKLY_GROWTH; //percent
		int NEUTRAL_STACK_EXP; 
		int MAX_BUILDING_PER_TURN;
		bool DWELLINGS_ACCUMULATE_CREATURES;
		bool ALL_CREATURES_GET_DOUBLE_MONTHS;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & CREEP_SIZE & WEEKLY_GROWTH & NEUTRAL_STACK_EXP;
			h & DWELLINGS_ACCUMULATE_CREATURES & ALL_CREATURES_GET_DOUBLE_MONTHS;
		}
	} settings;

	struct DLL_LINKAGE gameModules
	{
		bool STACK_EXP;
		bool STACK_ARTIFACT;
		bool COMMANDERS;
		bool MITHRIL;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & STACK_EXP & STACK_ARTIFACT & COMMANDERS & MITHRIL;
		}
	} modules;

	CModHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & allMods & activeMods & settings & modules;
	}
};
