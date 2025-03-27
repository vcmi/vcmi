/*
 * CRmgTemplate.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include <vstd/ContainerUtils.h>
#include <boost/bimap.hpp>
#include "CRmgTemplate.h"
#include "Functions.h"

#include "../TerrainHandler.h"
#include "../GameLibrary.h"
#include "../constants/StringConstants.h"
#include "../entities/faction/CTownHandler.h"
#include "../modding/ModScope.h"
#include "../serializer/JsonSerializeFormat.h"

#include <boost/lexical_cast.hpp>


VCMI_LIB_NAMESPACE_BEGIN

namespace
{
	si32 decodeZoneId(const std::string & json)
	{
		return boost::lexical_cast<si32>(json);
	}

	std::string encodeZoneId(si32 id)
	{
		return std::to_string(id);
	}
}

CTreasureInfo::CTreasureInfo()
	: min(0),
	max(0),
	density(0)
{

}

CTreasureInfo::CTreasureInfo(ui32 imin, ui32 imax, ui16 idensity)
	: min(imin), max(imax), density(idensity)
{
	
}

bool CTreasureInfo::operator==(const CTreasureInfo & other) const
{
	return (min == other.min) && (max == other.max) && (density == other.density);
}

void CTreasureInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("min", min, 0);
	handler.serializeInt("max", max, 0);
	handler.serializeInt("density", density, 0);
}

namespace rmg
{

const TRmgTemplateZoneId ZoneOptions::NO_ZONE = -1;

ZoneOptions::CTownInfo::CTownInfo()
	: townCount(0),
	castleCount(0),
	townDensity(0),
	castleDensity(0)
{

}

int ZoneOptions::CTownInfo::getTownCount() const
{
	return townCount;
}

int ZoneOptions::CTownInfo::getCastleCount() const
{
	return castleCount;
}

int ZoneOptions::CTownInfo::getTownDensity() const
{
	return townDensity;
}

int ZoneOptions::CTownInfo::getCastleDensity() const
{
	return castleDensity;
}

void ZoneOptions::CTownInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("towns", townCount, 0);
	handler.serializeInt("castles", castleCount, 0);
	handler.serializeInt("townDensity", townDensity, 0);
	handler.serializeInt("castleDensity", castleDensity, 0);
	handler.serializeInt("townTypesLikeZone", townTypesLikeZone, NO_ZONE);
	handler.serializeInt("townTypesNotLikeZone", townTypesNotLikeZone, NO_ZONE);
	handler.serializeInt("townTypesRelatedToZoneTerrain", townTypesRelatedToZoneTerrain, NO_ZONE);
}

ZoneOptions::CTownHints::CTownHints()
	: likeZone(NO_ZONE),
	relatedToZoneTerrain(NO_ZONE)
{

}

void ZoneOptions::CTownHints::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("likeZone", likeZone, NO_ZONE);
	auto node = handler.getCurrent();
	if (node["notLikeZone"].isVector())
	{
		// TODO: Utility to serialize vector of ints?
		auto notLikeZoneData = handler.enterArray("notLikeZone");
		notLikeZone.resize(notLikeZoneData.size());
		for (size_t i = 0; i < notLikeZoneData.size(); ++i)
		{
			notLikeZoneData.serializeInt(i, notLikeZone[i]);
		}
	}
	else
	{
		notLikeZone.resize(1);
		handler.serializeInt("notLikeZone", notLikeZone[0], NO_ZONE);
	}
	handler.serializeInt("relatedToZoneTerrain", relatedToZoneTerrain, NO_ZONE);
}

ZoneOptions::ZoneOptions():
	id(0),
	type(ETemplateZoneType::PLAYER_START),
	size(1),
	maxTreasureValue(0),
	owner(std::nullopt),
	matchTerrainToTown(true),
	townsAreSameType(false),
	monsterStrength(EMonsterStrength::ZONE_NORMAL),
	townsLikeZone(NO_ZONE),
	minesLikeZone(NO_ZONE),
	terrainTypeLikeZone(NO_ZONE),
	treasureLikeZone(NO_ZONE)
{
}

TRmgTemplateZoneId ZoneOptions::getId() const
{
	return id;
}

void ZoneOptions::setId(TRmgTemplateZoneId value)
{
	if(value <= 0)
		throw std::runtime_error(boost::str(boost::format("Zone %d id should be greater than 0.") % id));
	id = value;
}

ETemplateZoneType ZoneOptions::getType() const
{
	return type;
}
	
void ZoneOptions::setType(ETemplateZoneType value)
{
	type = value;
}

int ZoneOptions::getSize() const
{
	return size;
}

void ZoneOptions::setSize(int value)
{
	size = value;
}

std::optional<int> ZoneOptions::getOwner() const
{
	return owner;
}

std::set<TerrainId> ZoneOptions::getTerrainTypes() const
{
	if (terrainTypes.empty())
	{
		return vstd::difference(getDefaultTerrainTypes(), bannedTerrains);
	}
	else
	{
		return terrainTypes;
	}
}

void ZoneOptions::setTerrainTypes(const std::set<TerrainId> & value)
{
	terrainTypes = value;
}

std::set<TerrainId> ZoneOptions::getDefaultTerrainTypes() const
{
	std::set<TerrainId> terrains;
	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
	{
		if (terrain->isLand() && terrain->isPassable())
		{
			terrains.insert(terrain->getId());
		}
	}
	return terrains;
}

std::set<FactionID> ZoneOptions::getDefaultTownTypes() const
{
	return LIBRARY->townh->getDefaultAllowed();
}

std::set<FactionID> ZoneOptions::getTownTypes() const
{
	if (townTypes.empty())
	{
		//Assume that all towns are allowed, unless banned
		return vstd::difference(getDefaultTownTypes(), bannedTownTypes);
	}
	else 
	{
		return vstd::difference(townTypes, bannedTownTypes);
	}
}

void ZoneOptions::setTownTypes(const std::set<FactionID> & value)
{
	townTypes = value;
}

void ZoneOptions::setMonsterTypes(const std::set<FactionID> & value)
{
	monsterTypes = value;
}

std::set<FactionID> ZoneOptions::getMonsterTypes() const
{
	return vstd::difference(monsterTypes, bannedMonsters);
}

void ZoneOptions::setMinesInfo(const std::map<TResource, ui16> & value)
{
	mines = value;
}

std::map<TResource, ui16> ZoneOptions::getMinesInfo() const
{
	return mines;
}

void ZoneOptions::setTreasureInfo(const std::vector<CTreasureInfo> & value)
{
	treasureInfo = value;
	recalculateMaxTreasureValue();
}

void ZoneOptions::recalculateMaxTreasureValue()
{
	maxTreasureValue = 0;
	for (const auto& ti : treasureInfo)
	{
		vstd::amax(maxTreasureValue, ti.max);
	}
}
	
void ZoneOptions::addTreasureInfo(const CTreasureInfo & value)
{
	treasureInfo.push_back(value);
	vstd::amax(maxTreasureValue, value.max);
}

std::vector<CTreasureInfo> ZoneOptions::getTreasureInfo() const
{
	return treasureInfo;
}

ui32 ZoneOptions::getMaxTreasureValue() const
{
	return maxTreasureValue;
}

TRmgTemplateZoneId ZoneOptions::getMinesLikeZone() const
{
	return minesLikeZone;
}

TRmgTemplateZoneId ZoneOptions::getTerrainTypeLikeZone() const
{
	return terrainTypeLikeZone;
}

TRmgTemplateZoneId ZoneOptions::getTreasureLikeZone() const
{
	return treasureLikeZone;
}

ObjectConfig ZoneOptions::getCustomObjects() const
{
	return objectConfig;
}

void ZoneOptions::setCustomObjects(const ObjectConfig & value)
{
	objectConfig = value;
}

TRmgTemplateZoneId ZoneOptions::getCustomObjectsLikeZone() const
{
	return customObjectsLikeZone;
}

TRmgTemplateZoneId ZoneOptions::getTownsLikeZone() const
{
	return townsLikeZone;
}

void ZoneOptions::addConnection(const ZoneConnection & connection)
{
	connectedZoneIds.push_back(connection.getOtherZoneId(getId()));
	connectionDetails.push_back(connection);
}

std::vector<ZoneConnection> ZoneOptions::getConnections() const
{
	return connectionDetails;
}

std::vector<TRmgTemplateZoneId> ZoneOptions::getConnectedZoneIds() const
{
	return connectedZoneIds;
}

bool ZoneOptions::areTownsSameType() const
{
	return townsAreSameType;
}

bool ZoneOptions::isMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

void ZoneOptions::setMatchTerrainToTown(bool value)
{
	matchTerrainToTown = value;
}

const ZoneOptions::CTownInfo & ZoneOptions::getPlayerTowns() const
{
	return playerTowns;
}

void ZoneOptions::setPlayerTowns(const CTownInfo & value)
{
	playerTowns = value;
}

const ZoneOptions::CTownInfo & ZoneOptions::getNeutralTowns() const
{
	return neutralTowns;
}

void ZoneOptions::setNeutralTowns(const CTownInfo & value)
{
	neutralTowns = value;
}

const std::vector<ZoneOptions::CTownHints> & ZoneOptions::getTownHints() const
{
	return townHints;
}

void ZoneOptions::setTownHints(const std::vector<CTownHints> & value)
{
	townHints = value;
}

std::set<FactionID> ZoneOptions::getBannedTownTypes() const
{
	return bannedTownTypes;
}

void ZoneOptions::setBannedTownTypes(const std::set<FactionID> & value)
{
	bannedTownTypes = value;
}

void ZoneOptions::serializeJson(JsonSerializeFormat & handler)
{
	static const std::vector<std::string> zoneTypes =
	{
		"playerStart",
		"cpuStart",
		"treasure",
		"junction",
		"water",
		"sealed"
	};

	handler.serializeEnum("type", type, zoneTypes);
	handler.serializeInt("size", size, 1);
	handler.serializeInt("owner", owner);
	handler.serializeStruct("playerTowns", playerTowns);
	handler.serializeStruct("neutralTowns", neutralTowns);
	handler.serializeBool("matchTerrainToTown", matchTerrainToTown, true);

	#define SERIALIZE_ZONE_LINK(fieldName) handler.serializeInt(#fieldName, fieldName, NO_ZONE);

	SERIALIZE_ZONE_LINK(townsLikeZone);
	SERIALIZE_ZONE_LINK(minesLikeZone);
	SERIALIZE_ZONE_LINK(terrainTypeLikeZone);
	SERIALIZE_ZONE_LINK(treasureLikeZone);
	SERIALIZE_ZONE_LINK(customObjectsLikeZone);

	#undef SERIALIZE_ZONE_LINK

	if(terrainTypeLikeZone == NO_ZONE)
	{
		handler.serializeIdArray("terrainTypes", terrainTypes);
		handler.serializeIdArray("bannedTerrains", bannedTerrains);
	}

	handler.serializeBool("townsAreSameType", townsAreSameType, false);
	handler.serializeIdArray("allowedMonsters", monsterTypes);
	handler.serializeIdArray("bannedMonsters", bannedMonsters);
	handler.serializeIdArray("allowedTowns", townTypes);
	handler.serializeIdArray("bannedTowns", bannedTownTypes);

	handler.enterArray("townHints").serializeStruct(townHints);

	{
		//TODO: add support for std::map to serializeEnum
		static const std::vector<std::string> zoneMonsterStrengths =
		{
			"none",
			"weak",
			"normal",
			"strong"
		};

		int temporaryZoneMonsterStrengthIndex = monsterStrength == EMonsterStrength::ZONE_NONE ? 0 : monsterStrength - EMonsterStrength::ZONE_WEAK + 1 ; // temporary until serializeEnum starts supporting std::map
		// temporaryZoneMonsterStrengthIndex = 0, 1, 2 and 3 for monsterStrength = ZONE_NONE, ZONE_WEAK, ZONE_NORMAL and ZONE_STRONG respectively
		handler.serializeEnum("monsters", temporaryZoneMonsterStrengthIndex, 2, zoneMonsterStrengths); // default is normal monsters
		switch (temporaryZoneMonsterStrengthIndex)
		{
			case 0:
				monsterStrength = EMonsterStrength::ZONE_NONE;
				break;
			case 1:
				monsterStrength = EMonsterStrength::ZONE_WEAK;
				break;
			case 2:
				monsterStrength = EMonsterStrength::ZONE_NORMAL;
				break;
			case 3:
				monsterStrength = EMonsterStrength::ZONE_STRONG;
				break;
		}
	}

	if(treasureLikeZone == NO_ZONE)
	{
		auto treasureData = handler.enterArray("treasure");
		treasureData.serializeStruct(treasureInfo);
		if (!handler.saving)
		{
			recalculateMaxTreasureValue();
		}
	}

	if((minesLikeZone == NO_ZONE) && (!handler.saving || !mines.empty()))
	{
		auto minesData = handler.enterStruct("mines");

		for(TResource idx = 0; idx < (GameConstants::RESOURCE_QUANTITY - 1); idx++)
		{
			handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], mines[idx], 0);
		}
	}

	handler.serializeStruct("customObjects", objectConfig);
}

ZoneConnection::ZoneConnection():
	id(-1),
	zoneA(-1),
	zoneB(-1),
	guardStrength(0),
	connectionType(rmg::EConnectionType::GUARDED),
	hasRoad(rmg::ERoadOption::ROAD_TRUE)
{

}

int ZoneConnection::getId() const
{
	return id;
}

void ZoneConnection::setId(int id)
{
	this->id = id;
}

TRmgTemplateZoneId ZoneConnection::getZoneA() const
{
	return zoneA;
}

TRmgTemplateZoneId ZoneConnection::getZoneB() const
{
	return zoneB;
}

TRmgTemplateZoneId ZoneConnection::getOtherZoneId(TRmgTemplateZoneId id) const
{
	if (id == zoneA)
	{
		return zoneB;
	}
	else if (id == zoneB)
	{
		return zoneA;
	}
	else
	{
		throw rmgException("Zone does not belong to this connection");
	}
}

int ZoneConnection::getGuardStrength() const
{
	return guardStrength;
}

rmg::EConnectionType ZoneConnection::getConnectionType() const
{
	return connectionType;
}

rmg::ERoadOption ZoneConnection::getRoadOption() const
{
	return hasRoad;
}
	
bool operator==(const ZoneConnection & l, const ZoneConnection & r)
{
	return l.id == r.id;
}

void ZoneConnection::serializeJson(JsonSerializeFormat & handler)
{
	static const std::vector<std::string> connectionTypes =
	{
		"guarded",
		"fictive",
		"repulsive",
		"wide",
		"forcePortal"
	};

	static const std::vector<std::string> roadOptions =
	{
		"true",
		"false",
		"random"
	};

	if (handler.saving)
	{
		std::string zoneNameA = std::to_string(zoneA);
		std::string zoneNameB = std::to_string(zoneB);
		handler.serializeString("a", zoneNameA);
		handler.serializeString("b", zoneNameB);
	}
	else
	{
		std::string zoneNameA;
		std::string zoneNameB;
		handler.serializeString("a", zoneNameA);
		handler.serializeString("b", zoneNameB);

		zoneA = std::stoi(zoneNameA);
		zoneB = std::stoi(zoneNameB);
	}

	handler.serializeInt("guard", guardStrength, 0);
	handler.serializeEnum("type", connectionType, connectionTypes);
	handler.serializeEnum("road", hasRoad, roadOptions);
}

}

using namespace rmg;//todo: remove

CRmgTemplate::~CRmgTemplate() = default;

CRmgTemplate::CRmgTemplate()
	: minSize(72, 72, 2),
	maxSize(72, 72, 2),
	mapSettings(std::make_unique<JsonNode>())
{

}

bool CRmgTemplate::matchesSize(const int3 & value) const
{
	const int64_t square = value.x * value.y * value.z;
	const int64_t minSquare = minSize.x * minSize.y * minSize.z;
	const int64_t maxSquare = maxSize.x * maxSize.y * maxSize.z;

	return minSquare <= square && square <= maxSquare;
}

bool CRmgTemplate::isWaterContentAllowed(EWaterContent::EWaterContent waterContent) const
{
	return waterContent == EWaterContent::RANDOM || allowedWaterContent.count(waterContent);
}

const std::set<EWaterContent::EWaterContent> & CRmgTemplate::getWaterContentAllowed() const
{
	return allowedWaterContent;
}

void CRmgTemplate::setId(const std::string & value)
{
	id = value;
}

void CRmgTemplate::setName(const std::string & value)
{
	name = value;
}

const std::string & CRmgTemplate::getName() const
{
	return name;
}

const std::string & CRmgTemplate::getDescription() const
{
	return description;
}

const std::string & CRmgTemplate::getId() const
{
	return id;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getPlayers() const
{
	return players;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getHumanPlayers() const
{
	return humanPlayers;
}

const CRmgTemplate::Zones & CRmgTemplate::getZones() const
{
	return zones;
}

const std::vector<ZoneConnection> & CRmgTemplate::getConnectedZoneIds() const
{
	return connections;
}

void CRmgTemplate::validate() const
{
	//TODO add some validation checks, throw on failure
}

std::pair<int3, int3> CRmgTemplate::getMapSizes() const
{
	return {minSize, maxSize};
}

void CRmgTemplate::CPlayerCountRange::addRange(int lower, int upper)
{
	range.emplace_back(lower, upper);
}

void CRmgTemplate::CPlayerCountRange::addNumber(int value)
{
	range.emplace_back(value, value);
}

bool CRmgTemplate::CPlayerCountRange::isInRange(int count) const
{
	for(const auto & pair : range)
	{
		if(count >= pair.first && count <= pair.second) return true;
	}
	return false;
}

std::set<int> CRmgTemplate::CPlayerCountRange::getNumbers() const
{
	std::set<int> numbers;
	for(const auto & pair : range)
	{
		for(int i = pair.first; i <= pair.second; ++i) numbers.insert(i);
	}
	return numbers;
}

std::string CRmgTemplate::CPlayerCountRange::toString() const
{
	if(range.size() == 1)
	{
		const auto & p = range.front();
		if((p.first == p.second) && (p.first == 0))
			return "";
	}

	std::string ret;

	bool first = true;

	for(const auto & p : range)
	{
		if(!first)
			ret +=",";
		else
			first = false;

		if(p.first == p.second)
		{
			ret += std::to_string(p.first);
		}
		else
		{
			ret += boost::str(boost::format("%d-%d") % p.first % p.second);
		}
	}

	return ret;
}

void CRmgTemplate::CPlayerCountRange::fromString(const std::string & value)
{
	range.clear();

	if(value.empty())
	{
		addNumber(0);
	}
	else
	{
		std::vector<std::string> commaParts;
		boost::split(commaParts, value, boost::is_any_of(","));
		for(const auto & commaPart : commaParts)
		{
			std::vector<std::string> rangeParts;
			boost::split(rangeParts, commaPart, boost::is_any_of("-"));
			if(rangeParts.size() == 2)
			{
				auto lower = boost::lexical_cast<int>(rangeParts[0]);
				auto upper = boost::lexical_cast<int>(rangeParts[1]);
				addRange(lower, upper);
			}
			else if(rangeParts.size() == 1)
			{
				auto val = boost::lexical_cast<int>(rangeParts.front());
				addNumber(val);
			}
		}
	}
}

int CRmgTemplate::CPlayerCountRange::maxValue() const
{
	return *boost::max_element(getNumbers());
}

int CRmgTemplate::CPlayerCountRange::minValue() const
{
	return *boost::min_element(getNumbers());
}

void CRmgTemplate::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeString("description", description);
	serializeSize(handler, minSize, "minSize");
	serializeSize(handler, maxSize, "maxSize");
	serializePlayers(handler, players, "players");
	serializePlayers(handler, humanPlayers, "humans"); // TODO: Rename this parameter

	*mapSettings = handler.getCurrent()["settings"];

	{
		auto connectionsData = handler.enterArray("connections");
		connectionsData.serializeStruct(connections);
		if(!handler.saving)
		{
			for(size_t i = 0; i < connections.size(); ++i)
			{
				connections[i].setId(i);
			}
		}
	}
	
	{
		boost::bimap<EWaterContent::EWaterContent, std::string> enc;
		enc.insert({EWaterContent::NONE, "none"});
		enc.insert({EWaterContent::NORMAL, "normal"});
		enc.insert({EWaterContent::ISLANDS, "islands"});
		JsonNode node;
		if(handler.saving)
		{
			node.setType(JsonNode::JsonType::DATA_VECTOR);
			for(auto wc : allowedWaterContent)
			{
				JsonNode n;
				n.String() = enc.left.at(wc);
				node.Vector().push_back(n);
			}
		}
		handler.serializeRaw("allowedWaterContent", node, std::nullopt);
		if(!handler.saving)
		{
			for(auto wc : node.Vector())
			{
				allowedWaterContent.insert(enc.right.at(std::string(wc.String())));
			}
		}
	}

	{
		auto zonesData = handler.enterStruct("zones");
		if(handler.saving)
		{
			for(auto & idAndZone : zones)
			{
				auto guard = handler.enterStruct(encodeZoneId(idAndZone.first));
				idAndZone.second->serializeJson(handler);
			}
		}
		else
		{
			for(const auto & idAndZone : zonesData->getCurrent().Struct())
			{
				auto guard = handler.enterStruct(idAndZone.first);
				auto zone = std::make_shared<ZoneOptions>();
				zone->setId(decodeZoneId(idAndZone.first));
				zone->serializeJson(handler);
				zones[zone->getId()] = zone;
			}
		}
	}
}

const JsonNode & CRmgTemplate::getMapSettings() const
{
	return *mapSettings;
}

template<typename T>
T CRmgTemplate::inheritZoneProperty(std::shared_ptr<rmg::ZoneOptions> zone, 
									T (rmg::ZoneOptions::*getter)() const,
									void (rmg::ZoneOptions::*setter)(const T&),
									TRmgTemplateZoneId (rmg::ZoneOptions::*inheritFrom)() const,
									const std::string& propertyString,
									uint32_t iteration)
{
	if (iteration >= 50)
	{
		logGlobal->error("Infinite recursion for %s detected in template %s", propertyString, name);
		return T();
	}
	
	if (((*zone).*inheritFrom)() != rmg::ZoneOptions::NO_ZONE)
	{
		iteration++;
		const auto otherZone = zones.at(((*zone).*inheritFrom)());
		T inheritedValue = inheritZoneProperty(otherZone, getter, setter, inheritFrom, propertyString, iteration);
		((*zone).*setter)(inheritedValue);
	}
	
	return ((*zone).*getter)();
}

void CRmgTemplate::afterLoad()
{
	for(auto & idAndZone : zones)
	{
		auto zone = idAndZone.second;

		// Inherit properties recursively
		inheritTownProperties(zone);

		inheritZoneProperty(zone, 
							&rmg::ZoneOptions::getTerrainTypes, 
							&rmg::ZoneOptions::setTerrainTypes, 
							&rmg::ZoneOptions::getTerrainTypeLikeZone,
							"terrain types");
		
		inheritZoneProperty(zone, 
							&rmg::ZoneOptions::getMinesInfo, 
							&rmg::ZoneOptions::setMinesInfo, 
							&rmg::ZoneOptions::getMinesLikeZone,
							"mine types");
		
		inheritZoneProperty(zone, 
							&rmg::ZoneOptions::getTreasureInfo, 
							&rmg::ZoneOptions::setTreasureInfo, 
							&rmg::ZoneOptions::getTreasureLikeZone,
							"treasure info");

		inheritZoneProperty(zone, 
							&rmg::ZoneOptions::getCustomObjects, 
							&rmg::ZoneOptions::setCustomObjects, 
							&rmg::ZoneOptions::getCustomObjectsLikeZone,
							"custom objects");

				//TODO: Inherit monster types as well
		auto monsterTypes = zone->getMonsterTypes();
		if (monsterTypes.empty())
		{
			zone->setMonsterTypes(LIBRARY->townh->getAllowedFactions(false));
		}
	}

	for(const auto & connection : connections)
	{
		auto id1 = connection.getZoneA();
		auto id2 = connection.getZoneB();

		auto zone1 = zones.at(id1);
		auto zone2 = zones.at(id2);

		zone1->addConnection(connection);
		zone2->addConnection(connection);
	}
	
	if(allowedWaterContent.empty() || allowedWaterContent.count(EWaterContent::RANDOM))
	{
		allowedWaterContent.insert(EWaterContent::NONE);
		allowedWaterContent.insert(EWaterContent::NORMAL);
		allowedWaterContent.insert(EWaterContent::ISLANDS);
	}
	allowedWaterContent.erase(EWaterContent::RANDOM);
}

void CRmgTemplate::inheritTownProperties(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration)
{
	if (iteration >= 50)
	{
		logGlobal->error("Infinite recursion for town properties detected in template %s", name);
		return;
	}

	if (zone->getTownsLikeZone() != rmg::ZoneOptions::NO_ZONE)
	{
		const auto otherZone = zones.at(zone->getTownsLikeZone());
		
		// Recursively inherit from the source zone first
		inheritTownProperties(otherZone, iteration + 1);

		// Now copy all town-related properties from the source zone
		zone->setPlayerTowns(otherZone->getPlayerTowns());
		zone->setNeutralTowns(otherZone->getNeutralTowns());
		zone->setMatchTerrainToTown(otherZone->isMatchTerrainToTown());
		zone->setTownHints(otherZone->getTownHints());
		zone->setTownTypes(otherZone->getTownTypes());
		zone->setBannedTownTypes(otherZone->getBannedTownTypes());
	}
}

// TODO: Allow any integer size which does not match enum, as well
void CRmgTemplate::serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName)
{
	static const std::map<std::string, int3> sizeMapping =
	{
		{"s",    {CMapHeader::MAP_SIZE_SMALL, 	CMapHeader::MAP_SIZE_SMALL, 	1}},
		{"s+u",  {CMapHeader::MAP_SIZE_SMALL, 	CMapHeader::MAP_SIZE_SMALL, 	2}},
		{"m",    {CMapHeader::MAP_SIZE_MIDDLE, 	CMapHeader::MAP_SIZE_MIDDLE, 	1}},
		{"m+u",  {CMapHeader::MAP_SIZE_MIDDLE, 	CMapHeader::MAP_SIZE_MIDDLE, 	2}},
		{"l",    {CMapHeader::MAP_SIZE_LARGE, 	CMapHeader::MAP_SIZE_LARGE, 	1}},
		{"l+u",  {CMapHeader::MAP_SIZE_LARGE, 	CMapHeader::MAP_SIZE_LARGE, 	2}},
		{"xl",   {CMapHeader::MAP_SIZE_XLARGE, 	CMapHeader::MAP_SIZE_XLARGE, 	1}}	,
		{"xl+u", {CMapHeader::MAP_SIZE_XLARGE, 	CMapHeader::MAP_SIZE_XLARGE, 	2}}	,
		{"h",    {CMapHeader::MAP_SIZE_HUGE, 	CMapHeader::MAP_SIZE_HUGE, 		1}},
		{"h+u",  {CMapHeader::MAP_SIZE_HUGE, 	CMapHeader::MAP_SIZE_HUGE, 		2}},
		{"xh",   {CMapHeader::MAP_SIZE_XHUGE, 	CMapHeader::MAP_SIZE_XHUGE, 	1}},
		{"xh+u", {CMapHeader::MAP_SIZE_XHUGE, 	CMapHeader::MAP_SIZE_XHUGE, 	2}},
		{"g",    {CMapHeader::MAP_SIZE_GIANT,	CMapHeader::MAP_SIZE_GIANT,		1}},
		{"g+u",  {CMapHeader::MAP_SIZE_GIANT,	CMapHeader::MAP_SIZE_GIANT,		2}}
	};

	static const std::map<int3, std::string> sizeReverseMapping = vstd::invertMap(sizeMapping);

	std::string encodedValue;

	if(handler.saving)
	{
		auto iter = sizeReverseMapping.find(value);
		if(iter == sizeReverseMapping.end())
			encodedValue = boost::str(boost::format("%dx%dx%d") % value.x % value.y % value.z);
		else
			encodedValue = iter->second;
	}

	handler.serializeString(fieldName, encodedValue);

	if(!handler.saving)
	{
		auto iter = sizeMapping.find(encodedValue);

		if(iter == sizeMapping.end())
		{
			std::vector<std::string> parts;
			boost::split(parts, encodedValue, boost::is_any_of("x"));

			value.x = (boost::lexical_cast<int>(parts.at(0)));
			value.y = (boost::lexical_cast<int>(parts.at(1)));
			value.z = (boost::lexical_cast<int>(parts.at(2)));
		}
		else
		{
			value = iter->second;
		}
	}
}

void CRmgTemplate::serializePlayers(JsonSerializeFormat & handler, CPlayerCountRange & value, const std::string & fieldName)
{
	std::string encodedValue;

	if(handler.saving)
		encodedValue = value.toString();

	handler.serializeString(fieldName, encodedValue);

	if(!handler.saving)
		value.fromString(encodedValue);
}

const std::vector<CompoundMapObjectID> & ZoneOptions::getBannedObjects() const
{
	return objectConfig.getBannedObjects();
}

const std::vector<ObjectConfig::EObjectCategory> & ZoneOptions::getBannedObjectCategories() const
{
	return objectConfig.getBannedObjectCategories();
}

const std::vector<ObjectInfo> & ZoneOptions::getConfiguredObjects() const
{
	return objectConfig.getConfiguredObjects();
}

VCMI_LIB_NAMESPACE_END
