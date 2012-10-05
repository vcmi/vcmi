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
	return name;
}

const std::string & CBuilding::Description() const
{
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
		JsonVector & buildList = dest.Vector()[town]["buildings"].Vector();

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
			dest.Vector()[town]["buildings"].Vector()[buildID] = building;

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
			dest.Vector()[town]["buildings"].Vector().push_back(readBuilding(parser));
		}
		while (!parser.isNextEntryEmpty());
	}
	{
		CLegacyConfigParser parser("DATA/BLDGNEUT.TXT");

		for(int building=0; building<15; building++)
		{
			std::string name  = parser.readString();
			std::string descr = parser.readString();
			parser.endLine();

			for(int j=0; j<GameConstants::F_NUMBER; j++)
			{
				JsonVector & buildings = dest.Vector()[j]["buildings"].Vector();
				buildings[building]["name"].String() = name;
				buildings[building]["description"].String() = descr;
			}
		}
		parser.endLine(); // silo
		parser.endLine(); // blacksmith  //unused entries
		parser.endLine(); // moat

		//shipyard with the ship
		std::string name  = parser.readString();
		std::string descr = parser.readString();
		parser.endLine();

		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			JsonVector & buildings = dest.Vector()[town]["buildings"].Vector();
			buildings[20]["name"].String() = name;
			buildings[20]["description"].String() = descr;
		}

		//blacksmith
		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			JsonVector & buildings = dest.Vector()[town]["buildings"].Vector();
			buildings[16]["name"].String() =  parser.readString();
			buildings[16]["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/BLDGSPEC.TXT");

		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			JsonVector & buildings = dest.Vector()[town]["buildings"].Vector();
			for(int build=0; build<9; build++)
			{
				buildings[17+build]["name"].String() =  parser.readString();
				buildings[17+build]["description"].String() = parser.readString();
				parser.endLine();
			}
			buildings[26]["name"].String() =  parser.readString(); // Grail
			buildings[26]["description"].String() = parser.readString();
			parser.endLine();

			buildings[15]["name"].String() =  parser.readString(); // Resource silo
			buildings[15]["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/DWELLING.TXT");

		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			JsonVector & buildings = dest.Vector()[town]["buildings"].Vector();
			for(int build=0; build<14; build++)
			{
				buildings[30+build]["name"].String() =  parser.readString();
				buildings[30+build]["description"].String() = parser.readString();
				parser.endLine();
			}
		}
	}
	{
		CLegacyConfigParser typeParser("DATA/TOWNTYPE.TXT");
		CLegacyConfigParser nameParser("DATA/TOWNNAME.TXT");
		size_t townID=0;
		do
		{
			JsonNode & town = dest.Vector()[townID];

			town["name"].String() = typeParser.readString();


			for (int i=0; i<GameConstants::NAMES_PER_TOWN; i++)
			{
				JsonNode name;
				name.String() = nameParser.readString();
				town["names"].Vector().push_back(name);
				nameParser.endLine();
			}
			townID++;
		}
		while (typeParser.endLine());
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
	town.clientInfo.icons[0][0] = source["icons"]["village"]["normal"].Float();
	town.clientInfo.icons[0][1] = source["icons"]["village"]["built"].Float();
	town.clientInfo.icons[1][0] = source["icons"]["fort"]["normal"].Float();
	town.clientInfo.icons[1][1] = source["icons"]["fort"]["built"].Float();

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
	town.mageLevel = source["mageGuild"].Float();
	town.primaryRes  = source["primaryResource"].Float();
	town.warMachine = source["warMachine"].Float();
	town.names = source["names"].StdVector<std::string>();

	//  Horde building creature level
	BOOST_FOREACH(const JsonNode &node, source["horde"].Vector())
	{
		town.hordeLvl[town.hordeLvl.size()] = node.Float();
	}

	BOOST_FOREACH(const JsonNode &list, source["creatures"].Vector())
	{
		std::vector<TCreature> level;
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
		faction.name = node.second["name"].String();

		faction.creatureBg120 = node.second["creatureBackground"]["120px"].String();
		faction.creatureBg130 = node.second["creatureBackground"]["130px"].String();

		faction.nativeTerrain = vstd::find_pos(GameConstants::TERRAIN_NAMES, node.second["nativeTerrain"].String());
		int alignment = vstd::find_pos(EAlignment::names, node.second["alignment"].String());
		if (alignment == -1)
			faction.alignment = EAlignment::NEUTRAL;
		else
			faction.alignment = alignment;

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

	//hardcoded list of H3 factions. Should be only used to convert H3 configs
	static const std::string factionName [GameConstants::F_NUMBER] =
	{
		"castle", "rampart", "tower",
		"inferno", "necropolis", "dungeon",
		"stronghold", "fortress", "conflux"
	};

	// semi-manually merge legacy config with towns json

	for (size_t i=0; i< legacyConfig.Vector().size(); i++)
	{
		JsonNode & legacyFaction = legacyConfig.Vector()[i];
		JsonNode & outputFaction = buildingsConf[factionName[i]];

		if (outputFaction["name"].isNull())
			outputFaction["name"] = legacyFaction["name"];

		if (!outputFaction["town"].isNull())
		{
			if (outputFaction["town"]["names"].isNull())
				outputFaction["town"]["names"] = legacyFaction["names"];

			JsonNode & outputBuildings = outputFaction["town"]["buildings"];
			JsonVector & legacyBuildings = legacyFaction["buildings"].Vector();
			BOOST_FOREACH(JsonNode & building, outputBuildings.Vector())
			{
				if (vstd::contains(building.Struct(), "id") &&
				    legacyBuildings.size() > building["id"].Float() )
				{
					//find same buildings in legacy and json configs
					JsonNode & legacyBuilding = legacyBuildings[building["id"].Float()];

					if (!legacyBuilding.isNull()) //merge if h3 config was found for this building
						JsonNode::merge(building, legacyBuilding);
				}
			}
		}
	}
	loadFactions(buildingsConf);
}
