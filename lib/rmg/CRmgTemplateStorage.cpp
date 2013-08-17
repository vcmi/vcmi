
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
#include "../CTownHandler.h"
#include "../GameConstants.h"
#include "../StringConstants.h"

const std::map<std::string, CRmgTemplate *> & CRmgTemplateLoader::getTemplates() const
{
	return templates;
}

void CJsonRmgTemplateLoader::loadTemplates()
{
	const JsonNode rootNode(ResourceID("config/rmg.json"));
	for(const auto & templatePair : rootNode.Struct())
	{
		auto tpl = new CRmgTemplate();
		try
		{
			tpl->setName(templatePair.first);
			const auto & templateNode = templatePair.second;

			// Parse main template data
			tpl->setMinSize(parseMapTemplateSize(templateNode["minSize"].String()));
			tpl->setMaxSize(parseMapTemplateSize(templateNode["maxSize"].String()));
			tpl->setPlayers(parsePlayers(templateNode["players"].String()));
			tpl->setCpuPlayers(parsePlayers(templateNode["cpu"].String()));

			// Parse zones
			std::map<TRmgTemplateZoneId, CRmgTemplateZone *> zones;
			for(const auto & zonePair : templateNode["zones"].Struct())
			{
				auto zone = new CRmgTemplateZone();
				auto zoneId = boost::lexical_cast<TRmgTemplateZoneId>(zonePair.first);
				zone->setId(zoneId);

				const auto & zoneNode = zonePair.second;
				zone->setType(parseZoneType(zoneNode["type"].String()));
				zone->setSize(zoneNode["size"].Float());
				if(!zoneNode["owner"].isNull()) zone->setOwner(zoneNode["owner"].Float());

				zone->setPlayerTowns(parseTemplateZoneTowns(zoneNode["playerTowns"]));
				zone->setNeutralTowns(parseTemplateZoneTowns(zoneNode["neutralTowns"]));
				zone->setTownTypes(parseTownTypes(zoneNode["townTypes"].Vector(), zone->getDefaultTownTypes()));
				zone->setMatchTerrainToTown(zoneNode["matchTerrainToTown"].Bool());
				zone->setTerrainTypes(parseTerrainTypes(zoneNode["terrainTypes"].Vector(), zone->getDefaultTerrainTypes()));
				zone->setTownsAreSameType((zoneNode["townsAreSameType"].Bool()));
				if(!zoneNode["terrainTypeLikeZone"].isNull()) zone->setTerrainTypeLikeZone(boost::lexical_cast<int>(zoneNode["terrainTypeLikeZone"].String()));
				if(!zoneNode["townTypeLikeZone"].isNull()) zone->setTownTypeLikeZone(boost::lexical_cast<int>(zoneNode["townTypeLikeZone"].String()));

				zones[zone->getId()] = zone;
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
			tpl->validate();
			templates[tpl->getName()] = tpl;
		}
		catch(const std::exception & e)
		{
			logGlobal->errorStream() << boost::format("Template %s has errors. Message: %s.") % tpl->getName() % std::string(e.what());
		}
	}
}

CRmgTemplate::CSize CJsonRmgTemplateLoader::parseMapTemplateSize(const std::string & text) const
{
	CRmgTemplate::CSize size;
	if(text.empty()) return size;

	std::vector<std::string> parts;
	boost::split(parts, text, boost::is_any_of("+"));
	static const std::map<std::string, int> mapSizeMapping = boost::assign::map_list_of("s", CMapHeader::MAP_SIZE_SMALL)
			("m", CMapHeader::MAP_SIZE_MIDDLE)("l", CMapHeader::MAP_SIZE_LARGE)("xl", CMapHeader::MAP_SIZE_XLARGE);
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

ETemplateZoneType::ETemplateZoneType CJsonRmgTemplateLoader::parseZoneType(const std::string & type) const
{
	static const std::map<std::string, ETemplateZoneType::ETemplateZoneType> zoneTypeMapping = boost::assign::map_list_of
			("playerStart", ETemplateZoneType::PLAYER_START)("cpuStart", ETemplateZoneType::CPU_START)
			("treasure", ETemplateZoneType::TREASURE)("junction", ETemplateZoneType::JUNCTION);
	auto it = zoneTypeMapping.find(type);
	if(it == zoneTypeMapping.end()) throw std::runtime_error("Zone type unknown.");
	return it->second;
}

CRmgTemplateZone::CTownInfo CJsonRmgTemplateLoader::parseTemplateZoneTowns(const JsonNode & node) const
{
	CRmgTemplateZone::CTownInfo towns;
	towns.setTownCount(node["towns"].Float());
	towns.setCastleCount(node["castles"].Float());
	towns.setTownDensity(node["townDensity"].Float());
	towns.setCastleDensity(node["castleDensity"].Float());
	return towns;
}

std::set<TFaction> CJsonRmgTemplateLoader::parseTownTypes(const JsonVector & townTypesVector, const std::set<TFaction> & defaultTownTypes) const
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

std::set<ETerrainType> CJsonRmgTemplateLoader::parseTerrainTypes(const JsonVector & terTypeStrings, const std::set<ETerrainType> & defaultTerrainTypes) const
{
	std::set<ETerrainType> terTypes;
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

CRmgTemplate::CPlayerCountRange CJsonRmgTemplateLoader::parsePlayers(const std::string & players) const
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

const std::map<std::string, CRmgTemplate *> & CRmgTemplateStorage::getTemplates() const
{
	return templates;
}

CRmgTemplateStorage::CRmgTemplateStorage()
{
	auto jsonLoader = make_unique<CJsonRmgTemplateLoader>();
	jsonLoader->loadTemplates();

	const auto & tpls = jsonLoader->getTemplates();
	templates.insert(tpls.begin(), tpls.end());
}

CRmgTemplateStorage::~CRmgTemplateStorage()
{
	for (auto & pair : templates) delete pair.second;
}
