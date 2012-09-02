#include "StdInc.h"
#include "CTownHandler.h"

#include "VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "JsonNode.h"
#include "GameConstants.h"
#include "Filesystem/CResourceLoader.h"

/*
 * CTownHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static std::string emptyStr = "";

const std::string & CBuilding::Name() const
{
	if(name.length())
		return name;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].first;
	tlog2 << "Warning: Cannot find name text for building " << bid << "for " << tid << "town.\n";
	return emptyStr;
}

const std::string & CBuilding::Description() const
{
	if(description.length())
		return description;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].second;
	tlog2 << "Warning: Cannot find description text for building " << bid << "for " << tid << "town.\n";
	return emptyStr;
}

const std::string & CTown::Name() const
{
	if(name.length())
		return name;
	else
		return VLC->generaltexth->townTypes[typeID];
}

const std::vector<std::string> & CTown::Names() const
{
	if(names.size())
		return names;
	else
		return VLC->generaltexth->townNames[typeID];
}

CTownHandler::CTownHandler()
{
	VLC->townh = this;
}

JsonNode readBuilding(CLegacyConfigParser & parser)
{
	JsonNode ret;
	JsonNode & cost = ret["cost"];

	const std::string resources [] = {"wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold"};

	BOOST_FOREACH(const std::string & resID, resources)
		cost[resID].Float() = parser.readNumber();

	parser.endLine();
	return ret;
}

void CTownHandler::loadLegacyData(JsonNode & dest)
{
	CLegacyConfigParser parser("DATA/BUILDING.TXT");
	dest.Vector().resize(GameConstants::F_NUMBER);

	parser.endLine(); // header
	parser.endLine();

	//Unique buildings
	for (size_t town=0; town<GameConstants::F_NUMBER; town++)
	{
		JsonVector & buildList = dest.Vector()[town].Vector();

		buildList.resize( 30 ); //prepare vector for first set of buildings

		parser.endLine(); //header
		parser.endLine();

		int buildID = 17;
		do
		{
			buildList[buildID] = readBuilding(parser);
			buildID++;
		}
		while (!parser.isNextEntryEmpty());
	}

	// Common buildings
	parser.endLine(); // header
	parser.endLine();
	parser.endLine();

	int buildID = 0;
	do
	{
		JsonNode building = readBuilding(parser);

		for (size_t town=0; town<GameConstants::F_NUMBER; town++)
			dest.Vector()[town].Vector()[buildID] = building;

		buildID++;
	}
	while (!parser.isNextEntryEmpty());

	parser.endLine(); //header
	parser.endLine();

	//Dwellings
	for (size_t town=0; town<GameConstants::F_NUMBER; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		do
		{
			dest.Vector()[town].Vector().push_back(readBuilding(parser));
		}
		while (!parser.isNextEntryEmpty());
	}
}

void CTownHandler::loadBuilding(CTown &town, const JsonNode & source)
{
	CBuilding * ret = new CBuilding;

	ret->name = source["name"].String();
	ret->description = source["description"].String();

	ret->tid = town.typeID;
	ret->bid = source["id"].Float();

	ret->resources = TResources(source["cost"]);

	BOOST_FOREACH(const JsonNode &building, source["requires"].Vector())
		ret->requirements.insert(building.Float());

	town.buildings[ret->bid] = ret;
}

void CTownHandler::loadBuildings(CTown &town, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode &node, source.Vector())
	{
		loadBuilding(town, node);
	}
}

void CTownHandler::loadStructure(CTown &town, const JsonNode & source)
{
	CStructure * ret = new CStructure;

	ret->ID = source["id"].Float();
	ret->pos.x = source["x"].Float();
	ret->pos.y = source["y"].Float();
	ret->pos.z = 0;

	ret->defName = source["defname"].String();
	ret->borderName = source["border"].String();
	ret->areaName = source["area"].String();

	ret->group = -1;
	ret->townID = town.typeID;

	town.clientInfo.structures[ret->ID] = ret;
}

void CTownHandler::loadStructures(CTown &town, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode &node, source["structures"].Vector())
	{
		loadStructure(town, node);
	}

	// Read buildings blit order for that city
	int itr = 1;
	BOOST_FOREACH(const JsonNode &node, source["blit_order"].Vector())
	{
		int buildingID = node.Float();

		/* Find the building and set its order. */
		auto i2 = town.clientInfo.structures.find(buildingID);
		if (i2 != (town.clientInfo.structures.end()))
			i2->second->pos.z = itr++;
	}

	// Iterate for each group for that city
	int groupID = 0;
	BOOST_FOREACH(const JsonNode &group, source["groups"].Vector())
	{
		groupID++;

		// Iterate for each bulding value in the group
		BOOST_FOREACH(const JsonNode &value, group.Vector())
		{
			auto buildingIter = town.clientInfo.structures.find(value.Float());

			if (buildingIter != town.clientInfo.structures.end())
			{
				buildingIter->second->group = groupID;
			}
		}
	}
}

void CTownHandler::loadTownHall(CTown &town, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode &row, source.Vector())
	{
		std::vector< std::vector<int> > hallRow;

		BOOST_FOREACH(const JsonNode &box, row.Vector())
		{
			std::vector<int> hallBox;

			BOOST_FOREACH(const JsonNode &value, box.Vector())
			{
				hallBox.push_back(value.Float());
			}
			hallRow.push_back(hallBox);
		}
		town.clientInfo.hallSlots.push_back(hallRow);
	}
}

void CTownHandler::loadTown(std::vector<CTown> &towns, const JsonNode & source)
{
	towns.push_back(CTown());
	CTown & town = towns.back();

	//TODO: allow loading name and names vector from json and from h3 txt's

	town.typeID = towns.size() - 1;

	town.bonus = town.typeID;
	if (town.bonus==8)
		town.bonus=3;

	town.clientInfo.hallBackground = source["hallBackground"].String();
	town.mageLevel = source["mage_guild"].Float();
	town.primaryRes  = source["primary_resource"].Float();
	town.warMachine = source["war_machine"].Float();

	//  Horde building creature level
	BOOST_FOREACH(const JsonNode &node, source["horde"].Vector())
	{
		town.hordeLvl[town.hordeLvl.size()] = node.Float();
	}

	BOOST_FOREACH(const JsonNode &list, source["creatures"].Vector())
	{
		std::vector<si32> level;
		BOOST_FOREACH(const JsonNode &node, list.Vector())
		{
			level.push_back(node.Float());
		}
		town.creatures.push_back(level);
	}

	loadTownHall(town,   source["hallSlots"]);
	loadBuildings(town,  source["buildings"]);
	loadStructures(town, source);
}

void CTownHandler::loadTowns(std::vector<CTown> &towns, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode & node, source.Vector())
	{
		loadTown(towns, node);
	}

	// ensure that correct number of town have been loaded. Safe to remove
	assert(towns.size() == GameConstants::F_NUMBER);
}

void CTownHandler::load()
{
	JsonNode buildingsConf(ResourceID("config/buildings.json"));

	JsonNode legacyConfig;
	loadLegacyData(legacyConfig);

	// semi-manually merge legacy config with towns json
	// legacy config have only one item: town buildings stored in 2d vector
	size_t townsToMerge = std::min(buildingsConf["towns"].Vector().size(), legacyConfig.Vector().size());
	for (size_t i=0; i< townsToMerge; i++)
	{
		JsonNode & buildings = buildingsConf["towns"].Vector()[i]["buildings"];
		BOOST_FOREACH(JsonNode & building, buildings.Vector())
		{
			if (vstd::contains(building.Struct(), "id"))
			{
				JsonNode & legacyBuilding = legacyConfig.Vector()[i].Vector()[building["id"].Float()];

				if (!legacyBuilding.isNull())
					JsonNode::merge(building, legacyBuilding);
			}
		}
	}

	loadTowns(towns, buildingsConf["towns"]);
}
