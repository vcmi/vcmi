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

const std::string & CBuilding::Name() const
{
	if(name.length())
		return name;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].first;
	tlog2 << "Warning: Cannot find name text for building " << bid << "for " << tid << "town.\n";
	return name;
}

const std::string & CBuilding::Description() const
{
	if(description.length())
		return description;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].second;
	tlog2 << "Warning: Cannot find description text for building " << bid << "for " << tid << "town.\n";
	return description;
}

CBuilding::BuildingType CBuilding::getBase() const
{
	const CBuilding * build = this;
	while (build->upgrade >= 0)
		build = VLC->townh->towns[build->tid].buildings[build->upgrade];

	return build->bid;
}

si32 CBuilding::getDistance(CBuilding::BuildingType buildID) const
{
	const CBuilding * build = VLC->townh->towns[tid].buildings[buildID];
	int distance = 0;
	while (build->upgrade >= 0 && build != this)
	{
		build = VLC->townh->towns[build->tid].buildings[build->upgrade];
		distance++;
	}
	if (build == this)
		return distance;
	return -1;
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

	//note: this code will try to parse mithril as well but wil always return 0 for it
	BOOST_FOREACH(const std::string & resID, GameConstants::RESOURCE_NAMES)
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

	static const std::string modes [] = {"normal", "auto", "special", "grail"};

	ret->mode = boost::find(modes, source["mode"].String()) - modes;

	ret->tid = town.typeID;
	ret->bid = source["id"].Float();
	ret->name = source["name"].String();
	ret->description = source["description"].String();
	ret->resources = TResources(source["cost"]);

	BOOST_FOREACH(const JsonNode &building, source["requires"].Vector())
		ret->requirements.insert(building.Float());

	if (!source["upgrades"].isNull())
	{
		ret->requirements.insert(source["upgrades"].Float());
		ret->upgrade = source["upgrades"].Float();
	}
	else
		ret->upgrade = -1;

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

	if (source["id"].isNull())
	{
		ret->building = nullptr;
		ret->buildable = nullptr;
	}
	else
	{
		ret->building = town.buildings[source["id"].Float()];

		if (source["builds"].isNull())
			ret->buildable = ret->building;
		else
			ret->buildable = town.buildings[source["builds"].Float()];
	}

	ret->pos.x = source["x"].Float();
	ret->pos.y = source["y"].Float();
	ret->pos.z = source["z"].Float();

	ret->hiddenUpgrade = source["hidden"].Bool();
	ret->defName = source["animation"].String();
	ret->borderName = source["border"].String();
	ret->areaName = source["area"].String();

	town.clientInfo.structures.push_back(ret);
}

void CTownHandler::loadStructures(CTown &town, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode &node, source.Vector())
	{
		loadStructure(town, node);
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

void CTownHandler::loadClientData(CTown &town, const JsonNode & source)
{
	town.clientInfo.hallBackground = source["hallBackground"].String();
	town.clientInfo.musicTheme = source["musicTheme"].String();
	town.clientInfo.townBackground = source["townBackground"].String();
	town.clientInfo.guildWindow = source["guildWindow"].String();
	town.clientInfo.buildingsIcons = source["buildingsIcons"].String();

	loadTownHall(town,   source["hallSlots"]);
	loadStructures(town, source["structures"]);
}

void CTownHandler::loadTown(CTown &town, const JsonNode & source)
{
	town.bonus = town.typeID;
	if (town.bonus==8)
		town.bonus=3;

	town.mageLevel = source["mageGuild"].Float();
	town.primaryRes  = source["primaryResource"].Float();
	town.warMachine = source["warMachine"].Float();

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

	loadBuildings(town, source["buildings"]);
	loadClientData(town,source);
}

void CTownHandler::loadPuzzle(CFaction &faction, const JsonNode &source)
{
	faction.puzzleMap.reserve(GameConstants::PUZZLE_MAP_PIECES);

	std::string prefix = source["prefix"].String();
	BOOST_FOREACH(const JsonNode &piece, source["pieces"].Vector())
	{
		size_t index = faction.puzzleMap.size();
		SPuzzleInfo spi;

		spi.x = piece["x"].Float();
		spi.y = piece["y"].Float();
		spi.whenUncovered = piece["index"].Float();
		spi.number = index;

		// filename calculation
		std::ostringstream suffix;
		suffix << std::setfill('0') << std::setw(2) << index;

		spi.filename = prefix + suffix.str();

		faction.puzzleMap.push_back(spi);
	}
	assert(faction.puzzleMap.size() == GameConstants::PUZZLE_MAP_PIECES);
}

void CTownHandler::loadFactions(const JsonNode &source)
{
	BOOST_FOREACH(auto & node, source.Struct())
	{
		int id = node.second["index"].Float();
		CFaction & faction = factions[id];

		faction.factionID = id;
		faction.name = node.first;

		faction.creatureBg120 = node.second["creatureBackground"]["120px"].String();
		faction.creatureBg130 = node.second["creatureBackground"]["130px"].String();

		if (!node.second["nativeTerrain"].isNull())
		{
			//get terrain as string and converto to numeric ID
			faction.nativeTerrain = boost::find(GameConstants::TERRAIN_NAMES, node.second["nativeTerrain"].String()) - GameConstants::TERRAIN_NAMES;
		}
		else
			faction.nativeTerrain = -1;

		if (!node.second["town"].isNull())
		{
			towns[id].typeID = id;
			loadTown(towns[id], node.second["town"]);
		}
		if (!node.second["puzzleMap"].isNull())
			loadPuzzle(faction, node.second["puzzleMap"]);
	}
}

void CTownHandler::load()
{
	JsonNode buildingsConf(ResourceID("config/buildings.json"));

	JsonNode legacyConfig;
	loadLegacyData(legacyConfig);

	//hardocoded list of H3 factions. Should be only used to convert H3 configs
	static const std::string factionName [GameConstants::F_NUMBER] =
	{
		"castle", "rampart", "tower",
		"inferno", "necropolis", "dungeon",
		"stronghold", "fortress", "conflux"
	};

	// semi-manually merge legacy config with towns json
	// legacy config have only one item: town buildings stored in 2d vector

	for (size_t i=0; i< legacyConfig.Vector().size(); i++)
	{
		JsonNode & buildings = buildingsConf[factionName[i]]["town"]["buildings"];
		BOOST_FOREACH(JsonNode & building, buildings.Vector())
		{
			JsonNode & legacyFaction = legacyConfig.Vector()[i];
			if (vstd::contains(building.Struct(), "id"))
			{
				//find same buildings in legacy and json configs
				JsonNode & legacyBuilding = legacyFaction.Vector()[building["id"].Float()];

				if (!legacyBuilding.isNull()) //merge if h3 config was found for this building
					JsonNode::merge(building, legacyBuilding);
			}
		}
	}

	loadFactions(buildingsConf);
}
