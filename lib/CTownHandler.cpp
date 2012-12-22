#include "StdInc.h"
#include "CTownHandler.h"

#include "VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "JsonNode.h"
#include "StringConstants.h"
#include "CModHandler.h"
#include "CHeroHandler.h"
#include "CArtHandler.h"
#include "CSpellHandler.h"
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

CTown::ClientInfo::Point JsonToPoint(const JsonNode & node)
{
	CTown::ClientInfo::Point ret;
	ret.x = node["x"].Float();
	ret.y = node["y"].Float();
	return ret;
}

void CTownHandler::loadSiegeScreen(CTown &town, const JsonNode & source)
{
	town.clientInfo.siegePrefix = source["imagePrefix"].String();
	VLC->modh->identifiers.requestIdentifier(std::string("creature.") + source["shooter"].String(), [&town](si32 creature)
	{
		town.clientInfo.siegeShooter = creature;
	});

	town.clientInfo.siegeShooterCropHeight = source["shooterHeight"].Float();

	auto & pos = town.clientInfo.siegePositions;
	pos.resize(21);

	pos[8]  = JsonToPoint(source["towers"]["top"]["tower"]);
	pos[17] = JsonToPoint(source["towers"]["top"]["battlement"]);
	pos[20] = JsonToPoint(source["towers"]["top"]["creature"]);

	pos[2]  = JsonToPoint(source["towers"]["keep"]["tower"]);
	pos[15] = JsonToPoint(source["towers"]["keep"]["battlement"]);
	pos[18] = JsonToPoint(source["towers"]["keep"]["creature"]);

	pos[3]  = JsonToPoint(source["towers"]["bottom"]["tower"]);
	pos[16] = JsonToPoint(source["towers"]["bottom"]["battlement"]);
	pos[19] = JsonToPoint(source["towers"]["bottom"]["creature"]);

	pos[9]  = JsonToPoint(source["gate"]["gate"]);
	pos[10]  = JsonToPoint(source["gate"]["arch"]);

	pos[7]  = JsonToPoint(source["walls"]["upper"]);
	pos[6]  = JsonToPoint(source["walls"]["upperMid"]);
	pos[5]  = JsonToPoint(source["walls"]["bottomMid"]);
	pos[4]  = JsonToPoint(source["walls"]["bottom"]);

	pos[13] = JsonToPoint(source["moat"]["moat"]);
	pos[14] = JsonToPoint(source["moat"]["bank"]);

	pos[11] = JsonToPoint(source["static"]["bottom"]);
	pos[12] = JsonToPoint(source["static"]["top"]);
	pos[1]  = JsonToPoint(source["static"]["background"]);
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

	town.clientInfo.advMapVillage = source["adventureMap"]["village"].String();
	town.clientInfo.advMapCastle  = source["adventureMap"]["castle"].String();
	town.clientInfo.advMapCapitol = source["adventureMap"]["capitol"].String();

	const JsonNode *value = &source["adventureMap"]["dwellings"];
	if (!value->isNull())
	{
		BOOST_FOREACH (const JsonNode &d, value->Vector())
		{
			town.dwellings.push_back (d["graphics"].String());
			town.dwellingNames.push_back (d["name"].String());
		}
	}

	loadTownHall(town,   source["hallSlots"]);
	loadStructures(town, source["structures"]);
	loadSiegeScreen(town, source["siege"]);
}

void CTownHandler::loadTown(CTown &town, const JsonNode & source)
{
	auto resIter = boost::find(GameConstants::RESOURCE_NAMES, source["primaryResource"].String());
	if (resIter == boost::end(GameConstants::RESOURCE_NAMES))
		town.primaryRes = 127; //Wood + Ore
	else
		town.primaryRes = resIter - boost::begin(GameConstants::RESOURCE_NAMES);

	VLC->modh->identifiers.requestIdentifier(std::string("creature." + source["warMachine"].String()),
	[&town](si32 creature)
	{
		town.warMachine = CArtHandler::convertMachineID(creature, true);
	});

	town.mageLevel = source["mageGuild"].Float();
	town.names = source["names"].convertTo<std::vector<std::string> >();

	//  Horde building creature level
	BOOST_FOREACH(const JsonNode &node, source["horde"].Vector())
	{
		town.hordeLvl[town.hordeLvl.size()] = node.Float();
	}

	const JsonVector & creatures = source["creatures"].Vector();

	town.creatures.resize(creatures.size());

	for (size_t i=0; i< creatures.size(); i++)
	{
		const JsonVector & level = creatures[i].Vector();

		town.creatures[i].resize(level.size());

		for (size_t j=0; j<level.size(); j++)
		{
			VLC->modh->identifiers.requestIdentifier(std::string("creature.") + level[j].String(), [=, &town](si32 creature)
			{
				town.creatures[i][j] = creature;
			});
		}
	}

	/// set chance of specific hero class to appear in this town
	BOOST_FOREACH(auto &node, source["tavern"].Struct())
	{
		int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier("heroClass." + node.first, [=, &town](si32 classID)
		{
			VLC->heroh->classes.heroClasses[classID]->selectionProbability[town.typeID] = chance;
		});
	}

	BOOST_FOREACH(auto &node, source["guildSpells"].Struct())
	{
		int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier("spell." + node.first, [=, &town](si32 spellID)
		{
			VLC->spellh->spells[spellID]->probabilities[town.typeID] = chance;
		});
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

void CTownHandler::load(const JsonNode &source)
{
	BOOST_FOREACH(auto & node, source.Struct())
	{
		int id;

		if (node.second["index"].isNull())
			id = factions.rbegin()->first + 1;
		else
			id = node.second["index"].Float();

		CFaction & faction = factions[id];

		faction.factionID = id;
		faction.name = node.second["name"].String();

		VLC->modh->identifiers.requestIdentifier ("creature." + node.second["commander"].String(),
			[=](si32 commanderID)
			{
				factions[id].commander = commanderID;
			});

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

		tlog3 << "Added faction: " << node.first << "\n";
		VLC->modh->identifiers.registerObject(std::string("faction.") + node.first, faction.factionID);
	}
}

void CTownHandler::load()
{
	JsonNode gameConf(ResourceID("config/gameConfig.json"));
	JsonNode buildingsConf = JsonUtils::assembleFromFiles(gameConf["factions"].convertTo<std::vector<std::string> >());

	JsonNode legacyConfig;
	loadLegacyData(legacyConfig);

	// semi-manually merge legacy config with towns json

	for (size_t i=0; i< legacyConfig.Vector().size(); i++)
	{
		JsonNode & legacyFaction = legacyConfig.Vector()[i];
		JsonNode & outputFaction = buildingsConf[ETownType::names[i]];

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
						JsonUtils::merge(building, legacyBuilding);
				}
			}
		}
	}
	load(buildingsConf);
}

std::set<TFaction> CTownHandler::getDefaultAllowedFactions() const
{
	std::set<TFaction> allowedFactions;
	BOOST_FOREACH(auto town, towns)
	{
		allowedFactions.insert(town.first);
	}
	return allowedFactions;
}
