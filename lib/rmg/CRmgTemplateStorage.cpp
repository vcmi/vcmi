
/*
 * CRmgTemplateStorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRmgTemplateStorage.h"

#include "CRmgTemplate.h"
#include "CRmgTemplateZone.h"
#include "../filesystem/Filesystem.h"
#include "../JsonNode.h"
#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CModHandler.h"
#include "../CTownHandler.h"
#include "../GameConstants.h"
#include "../StringConstants.h"

const std::map<std::string, CRmgTemplate *> & CRmgTemplateStorage::getTemplates() const
{
	return templates;
}

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	//unused
	loadObject(scope, name, data);
}

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto tpl = new CRmgTemplate();
	try
	{
		const auto & templateNode = data;
		if (!templateNode["name"].isNull())
			tpl->setName(templateNode["name"].String()); //name can be customised. Allow duplicated names for different template versions.
		else
			tpl->setName(name); //identifier becomes default name

		// Parse main template data
		tpl->setMinSize(parseMapTemplateSize(templateNode["minSize"].String()));
		tpl->setMaxSize(parseMapTemplateSize(templateNode["maxSize"].String()));
		tpl->setPlayers(parsePlayers(templateNode["players"].String()));
		tpl->setCpuPlayers(parsePlayers(templateNode["cpu"].String()));

		// Parse zones
		std::map<TRmgTemplateZoneId, CRmgTemplateZone *> zones;
		for (const auto & zonePair : templateNode["zones"].Struct())
		{
			auto zone = new CRmgTemplateZone();
			auto zoneId = boost::lexical_cast<TRmgTemplateZoneId>(zonePair.first);
			zone->setId(zoneId);

			const auto & zoneNode = zonePair.second;
			zone->setType(parseZoneType(zoneNode["type"].String()));
			zone->setSize(zoneNode["size"].Float());
			if (!zoneNode["owner"].isNull()) zone->setOwner(zoneNode["owner"].Float());

			zone->setPlayerTowns(parseTemplateZoneTowns(zoneNode["playerTowns"]));
			zone->setNeutralTowns(parseTemplateZoneTowns(zoneNode["neutralTowns"]));
			if (!zoneNode["matchTerrainToTown"].isNull()) //default : true
				zone->setMatchTerrainToTown(zoneNode["matchTerrainToTown"].Bool());
			zone->setTerrainTypes(parseTerrainTypes(zoneNode["terrainTypes"].Vector(), zone->getDefaultTerrainTypes()));

			if (!zoneNode["townsAreSameType"].isNull()) //default : false
				zone->setTownsAreSameType((zoneNode["townsAreSameType"].Bool()));

			for (int i = 0; i < 2; ++i)
			{
				std::set<TFaction> allowedTownTypes;
				if (i)
				{
					if (zoneNode["allowedTowns"].isNull())
						allowedTownTypes = zone->getDefaultTownTypes();
				}
				else
				{
					if (zoneNode["allowedMonsters"].isNull())
						allowedTownTypes = VLC->townh->getAllowedFactions(false);
				}

				if (allowedTownTypes.empty())
				{
					for (const JsonNode & allowedTown : zoneNode[i ? "allowedTowns" : "allowedMonsters"].Vector())
					{
						//complain if the town type is not present in our game
						if (auto id = VLC->modh->identifiers.getIdentifier("faction", allowedTown, false))
							allowedTownTypes.insert(id.get());
					}
				}

				if (!zoneNode[i ? "bannedTowns" : "bannedMonsters"].isNull())
				{
					for (const JsonNode & bannedTown : zoneNode[i ? "bannedTowns" : "bannedMonsters"].Vector())
					{
						//erase unindentified towns silently
						if (auto id = VLC->modh->identifiers.getIdentifier("faction", bannedTown, true))
							vstd::erase_if_present(allowedTownTypes, id.get());
					}
				}
				if (i)
					zone->setTownTypes(allowedTownTypes);
				else
					zone->setMonsterTypes(allowedTownTypes);
			}

			const std::string monsterStrength = zoneNode["monsters"].String();
			if (monsterStrength == "weak")
				zone->setMonsterStrength(EMonsterStrength::ZONE_WEAK);
			else if (monsterStrength == "normal")
				zone->setMonsterStrength(EMonsterStrength::ZONE_NORMAL);
			else if (monsterStrength == "strong")
				zone->setMonsterStrength(EMonsterStrength::ZONE_STRONG);
			else
				throw (rmgException("incorrect monster power"));

			if (!zoneNode["mines"].isNull())
			{
				auto mines = zoneNode["mines"].Struct();
				//FIXME: maybe there is a smarter way to parse it already?
				zone->setMinesAmount(Res::WOOD, mines["wood"].Float());
				zone->setMinesAmount(Res::ORE, mines["ore"].Float());
				zone->setMinesAmount(Res::GEMS, mines["gems"].Float());
				zone->setMinesAmount(Res::CRYSTAL, mines["crystal"].Float());
				zone->setMinesAmount(Res::SULFUR, mines["sulfur"].Float());
				zone->setMinesAmount(Res::MERCURY, mines["mercury"].Float());
				zone->setMinesAmount(Res::GOLD, mines["gold"].Float());
				//TODO: Mithril
			}

			//treasures
			if (!zoneNode["treasure"].isNull())
			{
				//TODO: parse vector of different treasure settings
				if (zoneNode["treasure"].getType() == JsonNode::DATA_STRUCT)
				{
					auto treasureInfo = zoneNode["treasure"].Struct();
					{
						CTreasureInfo ti;
						ti.min = treasureInfo["min"].Float();
						ti.max = treasureInfo["max"].Float();
						ti.density = treasureInfo["density"].Float(); //TODO: use me
						zone->addTreasureInfo(ti);
					}
				}
				else if (zoneNode["treasure"].getType() == JsonNode::DATA_VECTOR)
				{
					for (auto treasureInfo : zoneNode["treasure"].Vector())
					{
						CTreasureInfo ti;
						ti.min = treasureInfo["min"].Float();
						ti.max = treasureInfo["max"].Float();
						ti.density = treasureInfo["density"].Float();
						zone->addTreasureInfo(ti);
					}
				}
			}

			zones[zone->getId()] = zone;
		}

		//copy settings from already parsed zones
		for (const auto & zonePair : templateNode["zones"].Struct())
		{
			auto zoneId = boost::lexical_cast<TRmgTemplateZoneId>(zonePair.first);
			auto zone = zones[zoneId];

			const auto & zoneNode = zonePair.second;

			if (!zoneNode["terrainTypeLikeZone"].isNull())
			{
				int id = zoneNode["terrainTypeLikeZone"].Float();
				zone->setTerrainTypes(zones[id]->getTerrainTypes());
				zone->setMatchTerrainToTown(zones[id]->getMatchTerrainToTown());
			}

			if (!zoneNode["townTypeLikeZone"].isNull())
				zone->setTownTypes (zones[zoneNode["townTypeLikeZone"].Float()]->getTownTypes());

			if (!zoneNode["treasureLikeZone"].isNull())
			{
				for (auto treasureInfo : zones[zoneNode["treasureLikeZone"].Float()]->getTreasureInfo())
				{
					zone->addTreasureInfo(treasureInfo);
				}
			}

			if (!zoneNode["minesLikeZone"].isNull())
			{
				for (auto mineInfo : zones[zoneNode["minesLikeZone"].Float()]->getMinesInfo())
				{
					zone->setMinesAmount (mineInfo.first, mineInfo.second);
				}

			}
		}

		tpl->setZones(zones);

		// Parse connections
		std::list<CRmgTemplateZoneConnection> connections;
		for(const auto & connPair : templateNode["connections"].Vector())
		{
			CRmgTemplateZoneConnection conn;
			conn.setZoneA(zones.find(boost::lexical_cast<TRmgTemplateZoneId>(connPair["a"].String()))->second);
			conn.setZoneB(zones.find(boost::lexical_cast<TRmgTemplateZoneId>(connPair["b"].String()))->second);
			conn.setGuardStrength(connPair["guard"].Float());
			connections.push_back(conn);
		}
		tpl->setConnections(connections);
		{
			auto zones = tpl->getZones();
			for (auto con : tpl->getConnections())
			{
				auto idA = con.getZoneA()->getId();
				auto idB = con.getZoneB()->getId();
				zones[idA]->addConnection(idB);
				zones[idB]->addConnection(idA);
			}
		}
		tpl->validate();
		templates[tpl->getName()] = tpl;
	}
	catch(const std::exception & e)
	{
		logGlobal->errorStream() << boost::format("Template %s has errors. Message: %s.") % tpl->getName() % std::string(e.what());
	}
}

CRmgTemplate::CSize CRmgTemplateStorage::parseMapTemplateSize(const std::string & text) const
{
	CRmgTemplate::CSize size;
	if(text.empty()) return size;

	std::vector<std::string> parts;
	boost::split(parts, text, boost::is_any_of("+"));
	static const std::map<std::string, int> mapSizeMapping =
	{
		{"s", CMapHeader::MAP_SIZE_SMALL},
		{"m", CMapHeader::MAP_SIZE_MIDDLE},
		{"l", CMapHeader::MAP_SIZE_LARGE},
		{"xl", CMapHeader::MAP_SIZE_XLARGE},
	};
	auto it = mapSizeMapping.find(parts[0]);
	if(it == mapSizeMapping.end())
	{
		// Map size is given as a number representation
		const auto & numericalRep = parts[0];
		parts.clear();
		boost::split(parts, numericalRep, boost::is_any_of("x"));
		assert(parts.size() == 3);
		size.setWidth(boost::lexical_cast<int>(parts[0]));
		size.setHeight(boost::lexical_cast<int>(parts[1]));
		size.setUnder(boost::lexical_cast<int>(parts[2]) == 1);
	}
	else
	{
		size.setWidth(it->second);
		size.setHeight(it->second);
		size.setUnder(parts.size() > 1 ? parts[1] == std::string("u") : false);
	}
	return size;
}

ETemplateZoneType::ETemplateZoneType CRmgTemplateStorage::parseZoneType(const std::string & type) const
{
	static const std::map<std::string, ETemplateZoneType::ETemplateZoneType> zoneTypeMapping =
	{
		{"playerStart", ETemplateZoneType::PLAYER_START},
		{"cpuStart", ETemplateZoneType::CPU_START},
		{"treasure", ETemplateZoneType::TREASURE},
		{"junction", ETemplateZoneType::JUNCTION},
	};
	auto it = zoneTypeMapping.find(type);
	if(it == zoneTypeMapping.end()) throw std::runtime_error("Zone type unknown.");
	return it->second;
}

CRmgTemplateZone::CTownInfo CRmgTemplateStorage::parseTemplateZoneTowns(const JsonNode & node) const
{
	CRmgTemplateZone::CTownInfo towns;
	towns.setTownCount(node["towns"].Float());
	towns.setCastleCount(node["castles"].Float());
	towns.setTownDensity(node["townDensity"].Float());
	towns.setCastleDensity(node["castleDensity"].Float());
	return towns;
}

std::set<TFaction> CRmgTemplateStorage::parseTownTypes(const JsonVector & townTypesVector, const std::set<TFaction> & defaultTownTypes) const
{
	std::set<TFaction> townTypes;
	for(const auto & townTypeNode : townTypesVector)
	{
		auto townTypeStr = townTypeNode.String();
		if(townTypeStr == "all") return defaultTownTypes;

		bool foundFaction = false;
		for(auto factionPtr : VLC->townh->factions)
		{
			if(factionPtr->town != nullptr && townTypeStr == factionPtr->name)
			{
				townTypes.insert(factionPtr->index);
				foundFaction = true;
			}
		}
		if(!foundFaction) throw std::runtime_error("Given faction is invalid.");
	}
	return townTypes;
}

std::set<ETerrainType> CRmgTemplateStorage::parseTerrainTypes(const JsonVector & terTypeStrings, const std::set<ETerrainType> & defaultTerrainTypes) const
{
	std::set<ETerrainType> terTypes;
	if (terTypeStrings.empty()) //nothing was specified
		return defaultTerrainTypes;

	for(const auto & node : terTypeStrings)
	{
		const auto & terTypeStr = node.String();
		if(terTypeStr == "all") return defaultTerrainTypes;
		auto pos = vstd::find_pos(GameConstants::TERRAIN_NAMES, terTypeStr);
		if (pos != -1)
		{
			terTypes.insert(ETerrainType(pos));
		}
		else
		{
			throw std::runtime_error("Terrain type is invalid.");
		}
	}
	return terTypes;
}

CRmgTemplate::CPlayerCountRange CRmgTemplateStorage::parsePlayers(const std::string & players) const
{
	CRmgTemplate::CPlayerCountRange playerRange;
	if(players.empty())
	{
		playerRange.addNumber(0);
		return playerRange;
	}
	std::vector<std::string> commaParts;
	boost::split(commaParts, players, boost::is_any_of(","));
	for(const auto & commaPart : commaParts)
	{
		std::vector<std::string> rangeParts;
		boost::split(rangeParts, commaPart, boost::is_any_of("-"));
		if(rangeParts.size() == 2)
		{
			auto lower = boost::lexical_cast<int>(rangeParts[0]);
			auto upper = boost::lexical_cast<int>(rangeParts[1]);
			playerRange.addRange(lower, upper);
		}
		else if(rangeParts.size() == 1)
		{
			auto val = boost::lexical_cast<int>(rangeParts.front());
			playerRange.addNumber(val);
		}
	}
	return playerRange;
}

CRmgTemplateStorage::CRmgTemplateStorage()
{
	//TODO: load all
}

CRmgTemplateStorage::~CRmgTemplateStorage()
{
	for (auto & pair : templates) delete pair.second;
}

std::vector<bool> CRmgTemplateStorage::getDefaultAllowed() const
{
	//all templates are allowed
	return std::vector<bool>();
}

std::vector<JsonNode> CRmgTemplateStorage::loadLegacyData(size_t dataSize)
{
	return std::vector<JsonNode>();
	//it would be cool to load old rmg.txt files
};
