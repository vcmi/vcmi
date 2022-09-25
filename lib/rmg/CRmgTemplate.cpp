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

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CTownHandler.h"
#include "../Terrain.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../StringConstants.h"

namespace
{
	si32 decodeZoneId(const std::string & json)
	{
		return boost::lexical_cast<si32>(json);
	}

	std::string encodeZoneId(si32 id)
	{
		return boost::lexical_cast<std::string>(id);
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

class TerrainEncoder
{
public:
	static si32 decode(const std::string & identifier)
	{
		return VLC->terrainTypeHandler->getInfoByCode(identifier)->id;
	}

	static std::string encode(const si32 index)
	{
		const auto& terrains = VLC->terrainTypeHandler->terrains();
		return (index >=0 && index < terrains.size()) ? terrains[index]->name : "<INVALID TERRAIN>";
	}
};

class ZoneEncoder
{
public:
	static si32 decode(const std::string & json)
	{
		return boost::lexical_cast<si32>(json);
	}

	static std::string encode(si32 id)
	{
		return boost::lexical_cast<std::string>(id);
	}
};

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
}


ZoneOptions::ZoneOptions()
	: id(0),
	type(ETemplateZoneType::PLAYER_START),
	size(1),
	owner(boost::none),
	playerTowns(),
	neutralTowns(),
	matchTerrainToTown(true),
	townsAreSameType(false),
	townTypes(),
	monsterTypes(),
	zoneMonsterStrength(EMonsterStrength::ZONE_NORMAL),
	mines(),
	treasureInfo(),
	connections(),
	minesLikeZone(NO_ZONE),
	terrainTypeLikeZone(NO_ZONE),
	treasureLikeZone(NO_ZONE)
{
	for(const auto * terr : VLC->terrainTypeHandler->terrains())
		if(terr->isLand() && terr->isPassable())
			terrainTypes.insert(terr->id);
}

ZoneOptions & ZoneOptions::operator=(const ZoneOptions & other)
{
	id = other.id;
	type = other.type;
	size = other.size;
	owner = other.owner;
	playerTowns = other.playerTowns;
	neutralTowns = other.neutralTowns;
	matchTerrainToTown = other.matchTerrainToTown;
	terrainTypes = other.terrainTypes;
	townsAreSameType = other.townsAreSameType;
	townTypes = other.townTypes;
	monsterTypes = other.monsterTypes;
	zoneMonsterStrength = other.zoneMonsterStrength;
	mines = other.mines;
	treasureInfo = other.treasureInfo;
	connections = other.connections;
	minesLikeZone = other.minesLikeZone;
	terrainTypeLikeZone = other.terrainTypeLikeZone;
	treasureLikeZone = other.treasureLikeZone;
	return *this;
}

TRmgTemplateZoneId ZoneOptions::getId() const
{
	return id;
}

void ZoneOptions::setId(TRmgTemplateZoneId value)
{
	if(value <= 0)
		throw std::runtime_error(boost::to_string(boost::format("Zone %d id should be greater than 0.") % id));
	id = value;
}

ETemplateZoneType::ETemplateZoneType ZoneOptions::getType() const
{
	return type;
}
	
void ZoneOptions::setType(ETemplateZoneType::ETemplateZoneType value)
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

boost::optional<int> ZoneOptions::getOwner() const
{
	return owner;
}

const std::set<TTerrainId> & ZoneOptions::getTerrainTypes() const
{
	return terrainTypes;
}

void ZoneOptions::setTerrainTypes(const std::set<TTerrainId> & value)
{
	//assert(value.find(ETerrainType::WRONG) == value.end() && value.find(ETerrainType::BORDER) == value.end() &&
	//	   value.find(ETerrainType::WATER) == value.end() && value.find(ETerrainType::ROCK) == value.end());
	terrainTypes = value;
}

std::set<TFaction> ZoneOptions::getDefaultTownTypes() const
{
	std::set<TFaction> defaultTowns;
	auto towns = VLC->townh->getDefaultAllowed();
	for(int i = 0; i < towns.size(); ++i)
	{
		if(towns[i]) defaultTowns.insert(i);
	}
	return defaultTowns;
}

const std::set<TFaction> & ZoneOptions::getTownTypes() const
{
	return townTypes;
}

void ZoneOptions::setTownTypes(const std::set<TFaction> & value)
{
	townTypes = value;
}

void ZoneOptions::setMonsterTypes(const std::set<TFaction> & value)
{
	monsterTypes = value;
}

const std::set<TFaction> & ZoneOptions::getMonsterTypes() const
{
	return monsterTypes;
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
}
	
void ZoneOptions::addTreasureInfo(const CTreasureInfo & value)
{
	treasureInfo.push_back(value);
}

const std::vector<CTreasureInfo> & ZoneOptions::getTreasureInfo() const
{
	return treasureInfo;
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

void ZoneOptions::addConnection(TRmgTemplateZoneId otherZone)
{
	connections.push_back (otherZone);
}

std::vector<TRmgTemplateZoneId> ZoneOptions::getConnections() const
{
	return connections;
}

bool ZoneOptions::areTownsSameType() const
{
	return townsAreSameType;
}

bool ZoneOptions::isMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

const ZoneOptions::CTownInfo & ZoneOptions::getPlayerTowns() const
{
	return playerTowns;
}

const ZoneOptions::CTownInfo & ZoneOptions::getNeutralTowns() const
{
	return neutralTowns;
}

void ZoneOptions::serializeJson(JsonSerializeFormat & handler)
{
	static const std::vector<std::string> zoneTypes =
	{
		"playerStart",
		"cpuStart",
		"treasure",
		"junction",
		"water"
	};

	handler.serializeEnum("type", type, zoneTypes);
	handler.serializeInt("size", size, 1);
	handler.serializeInt("owner", owner);
	handler.serializeStruct("playerTowns", playerTowns);
	handler.serializeStruct("neutralTowns", neutralTowns);
	handler.serializeBool("matchTerrainToTown", matchTerrainToTown, true);

	#define SERIALIZE_ZONE_LINK(fieldName) handler.serializeInt(#fieldName, fieldName, NO_ZONE);

	SERIALIZE_ZONE_LINK(minesLikeZone);
	SERIALIZE_ZONE_LINK(terrainTypeLikeZone);
	SERIALIZE_ZONE_LINK(treasureLikeZone);

	#undef SERIALIZE_ZONE_LINK

	if(terrainTypeLikeZone == NO_ZONE)
	{
		JsonNode node;
		if(handler.saving)
		{
			node.setType(JsonNode::JsonType::DATA_VECTOR);
			for(auto & ttype : terrainTypes)
			{
				JsonNode n;
				n.String() = ttype;
				node.Vector().push_back(n);
			}
		}
		handler.serializeRaw("terrainTypes", node, boost::none);
		if(!handler.saving)
		{
			if(!node.Vector().empty())
			{
				terrainTypes.clear();
				for(auto ttype : node.Vector())
				{
					terrainTypes.emplace(VLC->terrainTypeHandler->getInfoByName(ttype.String())->id);
				}
			}
		}
	}

	handler.serializeBool("townsAreSameType", townsAreSameType, false);
	handler.serializeIdArray<TFaction, FactionID>("allowedMonsters", monsterTypes, VLC->townh->getAllowedFactions(false));
	handler.serializeIdArray<TFaction, FactionID>("allowedTowns", townTypes, VLC->townh->getAllowedFactions(true));

	{
		//TODO: add support for std::map to serializeEnum
		static const std::vector<std::string> STRENGTH =
		{
			"weak",
			"normal",
			"strong"
		};

		si32 rawStrength = 0;
		if(handler.saving)
		{
			rawStrength = static_cast<decltype(rawStrength)>(zoneMonsterStrength);
			rawStrength++;
		}
		handler.serializeEnum("monsters", rawStrength, EMonsterStrength::ZONE_NORMAL + 1, STRENGTH);
		if(!handler.saving)
		{
			rawStrength--;
			zoneMonsterStrength = static_cast<decltype(zoneMonsterStrength)>(rawStrength);
		}
	}

	if(treasureLikeZone == NO_ZONE)
	{
		auto treasureData = handler.enterArray("treasure");
		treasureData.serializeStruct(treasureInfo);
	}

	if((minesLikeZone == NO_ZONE) && (!handler.saving || !mines.empty()))
	{
		auto minesData = handler.enterStruct("mines");

		for(TResource idx = 0; idx < (GameConstants::RESOURCE_QUANTITY - 1); idx++)
		{
			handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], mines[idx], 0);
		}
	}
}

ZoneConnection::ZoneConnection()
	: zoneA(-1),
	zoneB(-1),
	guardStrength(0)
{

}

TRmgTemplateZoneId ZoneConnection::getZoneA() const
{
	return zoneA;
}

TRmgTemplateZoneId ZoneConnection::getZoneB() const
{
	return zoneB;
}

int ZoneConnection::getGuardStrength() const
{
	return guardStrength;
}
	
bool operator==(const ZoneConnection & l, const ZoneConnection & r)
{
	return l.zoneA == r.zoneA && l.zoneB == r.zoneB && l.guardStrength == r.guardStrength;
}

void ZoneConnection::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeId<TRmgTemplateZoneId, TRmgTemplateZoneId, ZoneEncoder>("a", zoneA, -1);
	handler.serializeId<TRmgTemplateZoneId, TRmgTemplateZoneId, ZoneEncoder>("b", zoneB, -1);
	handler.serializeInt("guard", guardStrength, 0);
}

}

using namespace rmg;//todo: remove

CRmgTemplate::CRmgTemplate()
	: minSize(72, 72, 2),
	maxSize(72, 72, 2)
{

}

CRmgTemplate::~CRmgTemplate()
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
	return waterContent == EWaterContent::EWaterContent::RANDOM || allowedWaterContent.count(waterContent);
}

const std::set<EWaterContent::EWaterContent> & CRmgTemplate::getWaterContentAllowed() const
{
	return allowedWaterContent;
}

void CRmgTemplate::setId(const std::string & value)
{
	id = value;
}

const std::string & CRmgTemplate::getName() const
{
	return name.empty() ? id : name;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getPlayers() const
{
	return players;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getCpuPlayers() const
{
	return cpuPlayers;
}

const CRmgTemplate::Zones & CRmgTemplate::getZones() const
{
	return zones;
}

const std::vector<ZoneConnection> & CRmgTemplate::getConnections() const
{
	return connections;
}

void CRmgTemplate::validate() const
{
	//TODO add some validation checks, throw on failure
}

void CRmgTemplate::CPlayerCountRange::addRange(int lower, int upper)
{
	range.push_back(std::make_pair(lower, upper));
}

void CRmgTemplate::CPlayerCountRange::addNumber(int value)
{
	range.push_back(std::make_pair(value, value));
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

	for(auto & p : range)
	{
		if(!first)
			ret +=",";
		else
			first = false;

		if(p.first == p.second)
		{
			ret += boost::lexical_cast<std::string>(p.first);
		}
		else
		{
			ret += boost::to_string(boost::format("%d-%d") % p.first % p.second);
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

void CRmgTemplate::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	serializeSize(handler, minSize, "minSize");
	serializeSize(handler, maxSize, "maxSize");
	serializePlayers(handler, players, "players");
	serializePlayers(handler, cpuPlayers, "cpu");

	{
		auto connectionsData = handler.enterArray("connections");
		connectionsData.serializeStruct(connections);
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
		handler.serializeRaw("allowedWaterContent", node, boost::none);
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
			for(auto & idAndZone : zonesData->getCurrent().Struct())
			{
				auto guard = handler.enterStruct(idAndZone.first);
				auto zone = std::make_shared<ZoneOptions>();
				zone->setId(decodeZoneId(idAndZone.first));
				zone->serializeJson(handler);
				zones[zone->getId()] = zone;
			}
		}
	}
	if(!handler.saving)
		afterLoad();
}

void CRmgTemplate::afterLoad()
{
	for(auto & idAndZone : zones)
	{
		auto zone = idAndZone.second;
		if(zone->getMinesLikeZone() != ZoneOptions::NO_ZONE)
		{
			const auto otherZone = zones.at(zone->getMinesLikeZone());
			zone->setMinesInfo(otherZone->getMinesInfo());
		}

		if(zone->getTerrainTypeLikeZone() != ZoneOptions::NO_ZONE)
		{
			const auto otherZone = zones.at(zone->getTerrainTypeLikeZone());
			zone->setTerrainTypes(otherZone->getTerrainTypes());
		}

		if(zone->getTreasureLikeZone() != ZoneOptions::NO_ZONE)
		{
			const auto otherZone = zones.at(zone->getTreasureLikeZone());
			zone->setTreasureInfo(otherZone->getTreasureInfo());
		}
	}

	for(const auto & connection : connections)
	{
		auto id1 = connection.getZoneA();
		auto id2 = connection.getZoneB();

		auto zone1 = zones.at(id1);
		auto zone2 = zones.at(id2);

		zone1->addConnection(id2);
		zone2->addConnection(id1);
	}
	
	if(allowedWaterContent.empty() || allowedWaterContent.count(EWaterContent::EWaterContent::RANDOM))
	{
		allowedWaterContent.insert(EWaterContent::EWaterContent::NONE);
		allowedWaterContent.insert(EWaterContent::EWaterContent::NORMAL);
		allowedWaterContent.insert(EWaterContent::EWaterContent::ISLANDS);
	}
	allowedWaterContent.erase(EWaterContent::EWaterContent::RANDOM);
}

void CRmgTemplate::serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName)
{
	static const std::map<std::string, int3> sizeMapping =
	{
		{"s",    { 36,  36, 1}},
		{"s+u",  { 36,  36, 2}},
		{"m",    { 72,  72, 1}},
		{"m+u",  { 72,  72, 2}},
		{"l",    {108, 108, 1}},
		{"l+u",  {108, 108, 2}},
		{"xl",   {144, 144, 1}},
		{"xl+u", {144, 144, 2}},
		{"h",    {180, 180, 1}},
		{"h+u",  {180, 180, 2}},
		{"xh",   {216, 216, 1}},
		{"xh+u", {216, 216, 2}},
		{"g",    {252, 252, 1}},
		{"g+u",  {252, 252, 2}}
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

