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

typedef si32 artID;
typedef si32 creID;

class DLL_LINKAGE CModIdentifier
{
	//TODO? are simple integer identifiers enough?
	int id;
public:
	// int operator ()() {return 0;};
	bool operator < (CModIdentifier rhs) const {return true;}; //for map

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};


class DLL_LINKAGE CModInfo
{
public:
	std::vector <CModIdentifier> requirements;
	std::vector <ResourceID> usedFiles;
	//TODO: config options?

	//items added by this mod
	std::vector <artID> artifacts;
	std::vector <creID> creatures;

	//TODO: some additional scripts?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & requirements & artifacts & creatures;
		//h & usedFiles; //TODO: make seralizable?
	}
};

class DLL_LINKAGE CModHandler
{
public:
	std::string currentConfig; //save settings in this file
	//list of all possible objects in game, including inactive mods or not allowed
	std::vector <ConstTransitivePtr<CCreature> > creatures;
	std::vector <ConstTransitivePtr<CArtifact> > artifacts;

	std::map <CModIdentifier, CModInfo> allMods;
	std::set <CModIdentifier> activeMods;

	//create unique object indentifier
	artID addNewArtifact (CArtifact * art);
	creID addNewCreature (CCreature * cre);

	void loadConfigFromFile (std::string name);
	void saveConfigToFile (std::string name);
	CCreature * loadCreature (const JsonNode &node);
	void recreateAdvMapDefs();
	void recreateHandlers();

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
	~CModHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & currentConfig;
		h & creatures & artifacts;
		h & allMods & activeMods & settings & modules;
	}
};
