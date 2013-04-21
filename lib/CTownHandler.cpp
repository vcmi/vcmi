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
#include "filesystem/CResourceLoader.h"

/*
 * CTownHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const int NAMES_PER_TOWN=16; // number of town names per faction in H3 files. Json can define any number

const std::string & CBuilding::Name() const
{
	return name;
}

const std::string & CBuilding::Description() const
{
	return description;
}

BuildingID CBuilding::getBase() const
{
	const CBuilding * build = this;
	while (build->upgrade >= 0)
		build = build->town->buildings[build->upgrade];

	return build->bid;
}

si32 CBuilding::getDistance(BuildingID buildID) const
{
	const CBuilding * build = town->buildings[buildID];
	int distance = 0;
	while (build->upgrade >= 0 && build != this)
	{
		build = build->town->buildings[build->upgrade];
		distance++;
	}
	if (build == this)
		return distance;
	return -1;
}

CFaction::CFaction()
{

}

CFaction::~CFaction()
{
	delete town;
}

CTown::CTown()
{

}

CTown::~CTown()
{
	BOOST_FOREACH(auto build, buildings)
		build.second.dellNull();

	BOOST_FOREACH(CStructure * str, clientInfo.structures)
		delete str;
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

	cost.Struct().erase("mithril"); // erase mithril to avoid confusing validator

	parser.endLine();
	return ret;
}

std::vector<JsonNode> CTownHandler::loadLegacyData(size_t dataSize)
{
	std::vector<JsonNode> dest(dataSize);
	factions.resize(dataSize);

	auto getBuild = [&](size_t town, size_t building) -> JsonNode &
	{
		return dest[town]["town"]["buildings"][EBuildingType::names[building]];
	};

	CLegacyConfigParser parser("DATA/BUILDING.TXT");

	parser.endLine(); // header
	parser.endLine();

	//Unique buildings
	for (size_t town=0; town<dataSize; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		int buildID = 17;
		do
		{
			getBuild(town, buildID) = readBuilding(parser);
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

		for (size_t town=0; town<dataSize; town++)
			getBuild(town, buildID) = building;

		buildID++;
	}
	while (!parser.isNextEntryEmpty());

	parser.endLine(); //header
	parser.endLine();

	//Dwellings
	for (size_t town=0; town<dataSize; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		for (size_t i=0; i<14; i++)
		{
			getBuild(town, 30+i) = readBuilding(parser);
		}
	}
	{
		CLegacyConfigParser parser("DATA/BLDGNEUT.TXT");

		for(int building=0; building<15; building++)
		{
			std::string name  = parser.readString();
			std::string descr = parser.readString();
			parser.endLine();

			for(int j=0; j<dataSize; j++)
			{
				getBuild(j, building)["name"].String() = name;
				getBuild(j, building)["description"].String() = descr;
			}
		}
		parser.endLine(); // silo
		parser.endLine(); // blacksmith  //unused entries
		parser.endLine(); // moat

		//shipyard with the ship
		std::string name  = parser.readString();
		std::string descr = parser.readString();
		parser.endLine();

		for(int town=0; town<dataSize; town++)
		{
			getBuild(town, 20)["name"].String() = name;
			getBuild(town, 20)["description"].String() = descr;
		}

		//blacksmith
		for(int town=0; town<dataSize; town++)
		{
			getBuild(town, 16)["name"].String() =  parser.readString();
			getBuild(town, 16)["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/BLDGSPEC.TXT");

		for(int town=0; town<dataSize; town++)
		{
			for(int build=0; build<9; build++)
			{
				getBuild(town, 17 + build)["name"].String() =  parser.readString();
				getBuild(town, 17 + build)["description"].String() = parser.readString();
				parser.endLine();
			}
			getBuild(town, 26)["name"].String() =  parser.readString(); // Grail
			getBuild(town, 26)["description"].String() = parser.readString();
			parser.endLine();

			getBuild(town, 15)["name"].String() =  parser.readString(); // Resource silo
			getBuild(town, 15)["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/DWELLING.TXT");

		for(int town=0; town<dataSize; town++)
		{
			for(int build=0; build<14; build++)
			{
				getBuild(town, 30 + build)["name"].String() =  parser.readString();
				getBuild(town, 30 + build)["description"].String() = parser.readString();
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
			dest[townID]["name"].String() = typeParser.readString();

			for (int i=0; i<NAMES_PER_TOWN; i++)
			{
				JsonNode name;
				name.String() = nameParser.readString();
				dest[townID]["town"]["names"].Vector().push_back(name);
				nameParser.endLine();
			}
			townID++;
		}
		while (typeParser.endLine());
	}
	return dest;
}

void CTownHandler::loadBuilding(CTown &town, const JsonNode & source)
{
	CBuilding * ret = new CBuilding;

	static const std::string modes [] = {"normal", "auto", "special", "grail"};

	ret->mode = static_cast<CBuilding::EBuildMode>(boost::find(modes, source["mode"].String()) - modes);

	ret->town = &town;
	ret->bid = BuildingID(source["id"].Float());
	ret->name = source["name"].String();
	ret->description = source["description"].String();
	ret->resources = TResources(source["cost"]);

	BOOST_FOREACH(const JsonNode &building, source["requires"].Vector())
		ret->requirements.insert(BuildingID(building.Float()));

	if (!source["upgrades"].isNull())
	{
		ret->requirements.insert(BuildingID(source["upgrades"].Float()));
		ret->upgrade = BuildingID(source["upgrades"].Float());
	}
	else
		ret->upgrade = BuildingID::NONE;

	town.buildings[ret->bid] = ret;
}

void CTownHandler::loadBuildings(CTown &town, const JsonNode & source)
{
	if (source.getType() == JsonNode::DATA_VECTOR)
	{
		BOOST_FOREACH(auto &node, source.Vector())
		{
			if (!node.isNull())
				loadBuilding(town, node);
		}
	}
	else
	{
		BOOST_FOREACH(auto &node, source.Struct())
		{
			if (!node.second.isNull())
				loadBuilding(town, node.second);
		}
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
		ret->building = town.buildings[BuildingID(source["id"].Float())];

		if (source["builds"].isNull())
			ret->buildable = ret->building;
		else
			ret->buildable = town.buildings[BuildingID(source["builds"].Float())];
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
	if (source.getType() == JsonNode::DATA_VECTOR)
	{
		BOOST_FOREACH(auto &node, source.Vector())
		{
			if (!node.isNull())
				loadStructure(town, node);
		}
	}
	else
	{
		BOOST_FOREACH(auto &node, source.Struct())
		{
			if (!node.second.isNull())
				loadStructure(town, node.second);
		}
	}
}

void CTownHandler::loadTownHall(CTown &town, const JsonNode & source)
{
	BOOST_FOREACH(const JsonNode &row, source.Vector())
	{
		std::vector< std::vector<BuildingID> > hallRow;

		BOOST_FOREACH(const JsonNode &box, row.Vector())
		{
			std::vector<BuildingID> hallBox;

			BOOST_FOREACH(const JsonNode &value, box.Vector())
			{
				hallBox.push_back(BuildingID(value.Float()));
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
		town.clientInfo.siegeShooter = CreatureID(creature);
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
		town.primaryRes = Res::WOOD_AND_ORE; //Wood + Ore
	else
		town.primaryRes = resIter - boost::begin(GameConstants::RESOURCE_NAMES);

	VLC->modh->identifiers.requestIdentifier(std::string("creature." + source["warMachine"].String()),
	[&town](si32 creature)
	{
		town.warMachine = CArtHandler::creatureToMachineID(CreatureID(creature));
	});

	town.moatDamage = source["moatDamage"].Float();

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
				town.creatures[i][j] = CreatureID(creature);
			});
		}
	}

	/// set chance of specific hero class to appear in this town
	BOOST_FOREACH(auto &node, source["tavern"].Struct())
	{
		int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier("heroClass." + node.first, [=, &town](si32 classID)
		{
			VLC->heroh->classes.heroClasses[classID]->selectionProbability[town.faction->index] = chance;
		});
	}

	BOOST_FOREACH(auto &node, source["guildSpells"].Struct())
	{
		int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier("spell." + node.first, [=, &town](si32 spellID)
		{
			SpellID(spellID).toSpell()->probabilities[town.faction->index] = chance;
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

CFaction * CTownHandler::loadFromJson(const JsonNode &source)
{
	CFaction * faction = new CFaction();

	faction->name = source["name"].String();

	VLC->modh->identifiers.requestIdentifier ("creature." + source["commander"].String(),
		[=](si32 commanderID)
		{
			faction->commander = CreatureID(commanderID);
		});

	faction->creatureBg120 = source["creatureBackground"]["120px"].String();
	faction->creatureBg130 = source["creatureBackground"]["130px"].String();

	faction->nativeTerrain = ETerrainType(vstd::find_pos(GameConstants::TERRAIN_NAMES,
		source["nativeTerrain"].String()));
	int alignment = vstd::find_pos(EAlignment::names, source["alignment"].String());
	if (alignment == -1)
		faction->alignment = EAlignment::NEUTRAL;
	else
		faction->alignment = static_cast<EAlignment::EAlignment>(alignment);

	if (!source["town"].isNull())
	{
		faction->town = new CTown;
		faction->town->faction = faction;
		loadTown(*faction->town, source["town"]);
	}
	else
		faction->town = nullptr;

	if (!source["puzzleMap"].isNull())
		loadPuzzle(*faction, source["puzzleMap"]);

	return faction;
}

void CTownHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data);
	object->index = factions.size();

	factions.push_back(object);

	VLC->modh->identifiers.registerObject(scope, "faction", name, object->index);
}

void CTownHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data);
	object->index = index;

	assert(factions[index] == nullptr); // ensure that this id was not loaded before
	factions[index] = object;

	VLC->modh->identifiers.registerObject(scope, "faction", name, object->index);
}

std::vector<bool> CTownHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedFactions;
	BOOST_FOREACH(auto town, factions)
	{
		allowedFactions.push_back(town->town != nullptr);
	}
	return allowedFactions;
}
