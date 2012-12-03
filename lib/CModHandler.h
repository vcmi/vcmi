#pragma once

#include "Filesystem/CResourceLoader.h"

#include "VCMI_Lib.h"
#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CTownHandler.h"

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

public:
	/// request identifier for specific object name. If ID is not yet resolved callback will be queued
	/// and will be called later
	void requestIdentifier(std::string name, const boost::function<void(si32)> & callback);
	/// registers new object, calls all associated callbacks
	void registerObject(std::string name, si32 identifier);

	/// called at the very end of loading to check for any missing ID's
	void finalize() const;
};

typedef si32 TModID;

class DLL_LINKAGE CModInfo
{
public:
	/// TODO: list of mods that should be loaded before this one
	std::vector <TModID> requirements;

	/// mod configuration (mod.json).
	std::shared_ptr<JsonNode> config; //TODO: unique_ptr can't be serialized

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & requirements & config;
	}
};

class DLL_LINKAGE CModHandler
{
	//std::string currentConfig; //save settings in this file

	std::map <TModID, CModInfo> allMods;
	std::set <TModID> activeMods;//TODO: use me

public:
	CIdentifierStorage identifiers;

	/// management of game settings config
	void loadConfigFromFile (std::string name);	
	void saveConfigToFile (std::string name);

	/// find all available mods and load them into FS
	void findAvailableMods();

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
