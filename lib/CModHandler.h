#pragma once

#include "filesystem/Filesystem.h"

#include "VCMI_Lib.h"
#include "JsonNode.h"

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
class IHandlerBase;

/// class that stores all object identifiers strings and maps them to numeric ID's
/// if possible, objects ID's should be in format <type>.<name>, camelCase e.g. "creature.grandElf"
class CIdentifierStorage
{
	struct ObjectCallback // entry created on ID request
	{
		std::string localScope;  /// scope from which this ID was requested
		std::string remoteScope; /// scope in which this object must be found
		std::string type;        /// type, e.g. creature, faction, hero, etc
		std::string name;        /// string ID
		std::function<void(si32)> callback;

		ObjectCallback(std::string localScope, std::string remoteScope, std::string type, std::string name, const std::function<void(si32)> & callback);
	};

	struct ObjectData // entry created on ID registration
	{
		si32 id;
		std::string scope; /// scope in which this ID located
	};

	std::multimap<std::string, ObjectData > registeredObjects;
	std::vector<ObjectCallback> scheduledRequests;

	/// Check if identifier can be valid (camelCase, point as separator)
	void checkIdentifier(std::string & ID);

	void requestIdentifier(ObjectCallback callback);
	bool resolveIdentifier(const ObjectCallback & callback);
public:
	/// request identifier for specific object name. If ID is not yet resolved callback will be queued
	/// and will be called later
	void requestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback);
	void requestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback);
	void requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback);

	/// registers new object, calls all associated callbacks
	void registerObject(std::string scope, std::string type, std::string name, si32 identifier);

	/// called at the very end of loading to check for any missing ID's
	void finalize();
};

/// class used to load all game data into handlers. Used only during loading
class CContentHandler
{
	/// internal type to handle loading of one data type (e.g. artifacts, creatures)
	class ContentTypeHandler
	{
		struct ModInfo
		{
			/// mod data from this mod and for this mod
			JsonNode modData;
			/// mod data for this mod from other mods (patches)
			JsonNode patches;
		};

		/// handler to which all data will be loaded
		IHandlerBase * handler;

		std::string objectName;

		/// contains all loaded H3 data
		std::vector<JsonNode> originalData;
		std::map<std::string, ModInfo> modData;

	public:
		ContentTypeHandler(IHandlerBase * handler, std::string objectName);

		/// local version of methods in ContentHandler
		/// returns true if loading was successfull
		bool preloadModData(std::string modName, std::vector<std::string> fileList, bool validate);
		bool loadMod(std::string modName, bool validate);
		void afterLoadFinalization();
	};

	std::map<std::string, ContentTypeHandler> handlers;
public:
	/// fully initialize object. Will cause reading of H3 config files
	CContentHandler();

	/// preloads all data from fileList as data from modName.
	/// returns true if loading was successfull
	bool preloadModData(std::string modName, JsonNode modConfig, bool validate);

	/// actually loads data in mod
	/// returns true if loading was successfull
	bool loadMod(std::string modName, bool validate);

	/// all data was loaded, time for final validation / integration
	void afterLoadFinalization();
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

	/// CRC-32 checksum of the mod
	ui32 checksum;

	/// true if mod has passed validation successfully
	bool validated;

	/// true if mod is enabled
	bool enabled;

	// mod configuration (mod.json). (no need to store it right now)
	// std::shared_ptr<JsonNode> config; //TODO: unique_ptr can't be serialized

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & description & name & dependencies & conflicts & checksum & validated & enabled;
	}
};

class DLL_LINKAGE CModHandler
{
	std::map <TModID, CModInfo> allMods;
	std::vector <TModID> activeMods;//active mods, in order in which they were loaded

	void loadConfigFromFile(std::string name);
	void loadModFilesystems();

	bool hasCircularDependency(TModID mod, std::set <TModID> currentList = std::set <TModID>()) const;

	//returns false if mod list is incorrect and prints error to console. Possible errors are:
	// - missing dependency mod
	// - conflicting mod in load order
	// - circular dependencies
	bool checkDependencies(const std::vector <TModID> & input) const;

	// returns load order in which all dependencies are resolved, e.g. loaded after required mods
	// function assumes that input list is valid (checkDependencies returned true)
	std::vector <TModID> resolveDependencies(std::vector<TModID> input) const;
public:

	CIdentifierStorage identifiers;

	/// receives list of available mods and trying to load mod.json from all of them
	void initializeMods(std::vector<std::string> availableMods);
	void initializeConfig();

	CModInfo & getModData(TModID modId);

	/// load content from all available mods
	void load();
	void afterLoad();

	/// actions that should be triggered on map restart
	/// TODO: merge into appropriate handlers?
	void reload();

	struct DLL_LINKAGE hardcodedFeatures
	{
		JsonNode data;

		int CREEP_SIZE; // neutral stacks won't grow beyond this number
		int WEEKLY_GROWTH; //percent
		int NEUTRAL_STACK_EXP; 
		int MAX_BUILDING_PER_TURN;
		bool DWELLINGS_ACCUMULATE_CREATURES;
		bool ALL_CREATURES_GET_DOUBLE_MONTHS;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & data & CREEP_SIZE & WEEKLY_GROWTH & NEUTRAL_STACK_EXP & MAX_BUILDING_PER_TURN;
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
