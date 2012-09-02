#pragma once

#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "int3.h"

/*
 * CTownHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLegacyConfigParser;
class JsonNode;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures
class DLL_LINKAGE CBuilding
{
	std::string name;
	std::string description;

public:
	si32 tid, bid; //town ID and structure ID
	TResources resources;
	std::set<int> requirements; //set of required buildings

	const std::string &Name() const;
	const std::string &Description() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & resources & name & description & requirements;
	}

	friend class CTownHandler;
};

/// This is structure used only by client
/// Consists of all gui-related data about town structures
/// Should be mode from lib to client
struct DLL_LINKAGE CStructure
{
	int ID;
	int3 pos;
	std::string defName, borderName, areaName;
	int townID, group;

	bool operator<(const CStructure & p2) const
	{
		if(pos.z != p2.pos.z)
			return (pos.z) < (p2.pos.z);
		else
			return (ID) < (p2.ID);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & pos & defName & borderName & areaName & townID & group;
	}
};

class DLL_LINKAGE CTown
{
	std::string name; //name of type
	std::vector<std::string> names; //names of the town instances

public:
	ui32 typeID;

	/// level -> list of creatures on this tier
	/// TODO: replace with pointers to CCreature?
	std::vector<std::vector<si32> > creatures;

	bmap<int, ConstTransitivePtr<CBuilding> > buildings;

	// should be removed at least from configs in favour of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	int bonus; //pic number
	ui16 primaryRes, warMachine;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
		std::string hallBackground;
		std::vector< std::vector< std::vector<int> > > hallSlots; /// vector[row][column] = list of buildings in this slot
		bmap<int, ConstTransitivePtr<CStructure> > structures;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & hallBackground & hallSlots & structures;
		}
	} clientInfo;

	const std::vector<std::string> & Names() const;
	const std::string & Name() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & names & typeID & creatures & buildings & hordeLvl & mageLevel & bonus
			& primaryRes & warMachine & clientInfo;
	}

	friend class CTownHandler;
};

class DLL_LINKAGE CTownHandler
{
	/// loads CBuilding's into town
	void loadBuilding(CTown &town, const JsonNode & source);
	void loadBuildings(CTown &town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(CTown &town, const JsonNode & source);
	void loadStructures(CTown &town, const JsonNode & source);

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown &town, const JsonNode & source);

	/// load town and insert it into towns vector
	void loadTown(std::vector<CTown> &towns, const JsonNode & source);

	/// main loading function, accepts merged JSON source and add all entries from it into game
	/// all entries in JSON should be checked for validness before using this function
	void loadTowns(std::vector<CTown> &towns, const JsonNode & source);

	/// load all available data from h3 txt(s) into json structure using format similar to vcmi configs
	/// returns 2d array [townID] [buildID] of buildings
	void loadLegacyData(JsonNode & dest);
public:
	std::vector<CTown> towns;

	CTownHandler(); //c-tor, set pointer in VLC to this

	/// "entry point" for towns loading.
	/// reads legacy txt's from H3 + vcmi json, merges them
	/// and loads resulting structure to game using loadTowns method
	void load();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & towns;
	}
};
