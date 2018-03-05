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
#include "CRmgTemplate.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CTownHandler.h"

namespace rmg
{

const std::set<ETerrainType> ZoneOptions::DEFAULT_TERRAIN_TYPES =
{
	ETerrainType::DIRT,
	ETerrainType::SAND,
	ETerrainType::GRASS,
	ETerrainType::SNOW,
	ETerrainType::SWAMP,
	ETerrainType::ROUGH,
	ETerrainType::SUBTERRANEAN,
	ETerrainType::LAVA
};

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

void ZoneOptions::CTownInfo::setTownCount(int value)
{
	if(value < 0)
		throw std::runtime_error("Negative value for town count not allowed.");
	townCount = value;
}

int ZoneOptions::CTownInfo::getCastleCount() const
{
	return castleCount;
}

void ZoneOptions::CTownInfo::setCastleCount(int value)
{
	if(value < 0)
		throw std::runtime_error("Negative value for castle count not allowed.");
	castleCount = value;
}

int ZoneOptions::CTownInfo::getTownDensity() const
{
	return townDensity;
}

void ZoneOptions::CTownInfo::setTownDensity(int value)
{
	if(value < 0)
		throw std::runtime_error("Negative value for town density not allowed.");
	townDensity = value;
}

int ZoneOptions::CTownInfo::getCastleDensity() const
{
	return castleDensity;
}

void ZoneOptions::CTownInfo::setCastleDensity(int value)
{
	if(value < 0)
		throw std::runtime_error("Negative value for castle density not allowed.");
	castleDensity = value;
}

ZoneOptions::ZoneOptions()
	: id(0),
	type(ETemplateZoneType::PLAYER_START),
	size(1),
	owner(boost::none),
	playerTowns(),
	neutralTowns(),
	matchTerrainToTown(true),
	terrainTypes(DEFAULT_TERRAIN_TYPES),
	townsAreSameType(false),
	townTypes(),
	monsterTypes(),
	zoneMonsterStrength(EMonsterStrength::ZONE_NORMAL),
	mines(),
	treasureInfo(),
	connections()
{

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
	if(value <= 0)
		throw std::runtime_error(boost::to_string(boost::format("Zone %d size needs to be greater than 0.") % id));
	size = value;
}

boost::optional<int> ZoneOptions::getOwner() const
{
	return owner;
}

void ZoneOptions::setOwner(boost::optional<int> value)
{
	if(value && !(*value >= 0 && *value <= PlayerColor::PLAYER_LIMIT_I))
		throw std::runtime_error(boost::to_string(boost::format ("Owner of zone %d has to be in range 0 to max player count.") % id));
	owner = value;
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

bool ZoneOptions::getMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

void ZoneOptions::setMatchTerrainToTown(bool value)
{
	matchTerrainToTown = value;
}

const std::set<ETerrainType> & ZoneOptions::getTerrainTypes() const
{
	return terrainTypes;
}

void ZoneOptions::setTerrainTypes(const std::set<ETerrainType> & value)
{
	assert(value.find(ETerrainType::WRONG) == value.end() && value.find(ETerrainType::BORDER) == value.end() &&
		   value.find(ETerrainType::WATER) == value.end() && value.find(ETerrainType::ROCK) == value.end());
	terrainTypes = value;
}

bool ZoneOptions::getTownsAreSameType() const
{
	return townsAreSameType;
}

void ZoneOptions::setTownsAreSameType(bool value)
{
	townsAreSameType = value;
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

void ZoneOptions::setMonsterStrength(EMonsterStrength::EMonsterStrength val)
{
	assert (vstd::iswithin(val, EMonsterStrength::ZONE_WEAK, EMonsterStrength::ZONE_STRONG));
	zoneMonsterStrength = val;
}

void ZoneOptions::setMinesAmount(TResource res, ui16 amount)
{
	mines[res] = amount;
}

std::map<TResource, ui16> ZoneOptions::getMinesInfo() const
{
	return mines;
}

void ZoneOptions::addTreasureInfo(const CTreasureInfo & info)
{
	treasureInfo.push_back(info);
}

const std::vector<CTreasureInfo> & ZoneOptions::getTreasureInfo() const
{
	return treasureInfo;
}

void ZoneOptions::addConnection(TRmgTemplateZoneId otherZone)
{
	connections.push_back (otherZone);
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

void ZoneConnection::setZoneA(TRmgTemplateZoneId value)
{
	zoneA = value;
}

TRmgTemplateZoneId ZoneConnection::getZoneB() const
{
	return zoneB;
}

void ZoneConnection::setZoneB(TRmgTemplateZoneId value)
{
	zoneB = value;
}

int ZoneConnection::getGuardStrength() const
{
	return guardStrength;
}

void ZoneConnection::setGuardStrength(int value)
{
	if(value < 0) throw std::runtime_error("Negative value for guard strength not allowed.");
	guardStrength = value;
}

}


using namespace rmg;//todo: remove


CRmgTemplate::CSize::CSize() : width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), under(true)
{

}

CRmgTemplate::CSize::CSize(int width, int height, bool under) : under(under)
{
	setWidth(width);
	setHeight(height);
}

int CRmgTemplate::CSize::getWidth() const
{
	return width;
}

void CRmgTemplate::CSize::setWidth(int value)
{
	if(value <= 0) throw std::runtime_error("Width > 0 failed.");
	width = value;
}

int CRmgTemplate::CSize::getHeight() const
{
	return height;
}

void CRmgTemplate::CSize::setHeight(int value)
{
	if(value <= 0) throw std::runtime_error("Height > 0 failed.");
	height = value;
}

bool CRmgTemplate::CSize::getUnder() const
{
	return under;
}

void CRmgTemplate::CSize::setUnder(bool value)
{
	under = value;
}

bool CRmgTemplate::CSize::operator<=(const CSize & value) const
{
	if(width < value.width && height < value.height)
	{
		return true;
	}
	else if(width == value.width && height == value.height)
	{
		return under ? value.under : true;
	}
	else
	{
		return false;
	}
}

bool CRmgTemplate::CSize::operator>=(const CSize & value) const
{
	if(width > value.width && height > value.height)
	{
		return true;
	}
	else if(width == value.width && height == value.height)
	{
		return under ? true : !value.under;
	}
	else
	{
		return false;
	}
}

CRmgTemplate::CRmgTemplate()
{

}

CRmgTemplate::~CRmgTemplate()
{
	for (auto & pair : zones) delete pair.second;
}

const std::string & CRmgTemplate::getName() const
{
	return name;
}

void CRmgTemplate::setName(const std::string & value)
{
	name = value;
}

const CRmgTemplate::CSize & CRmgTemplate::getMinSize() const
{
	return minSize;
}

void CRmgTemplate::setMinSize(const CSize & value)
{
	minSize = value;
}

const CRmgTemplate::CSize & CRmgTemplate::getMaxSize() const
{
	return maxSize;
}

void CRmgTemplate::setMaxSize(const CSize & value)
{
	maxSize = value;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getPlayers() const
{
	return players;
}

void CRmgTemplate::setPlayers(const CPlayerCountRange & value)
{
	players = value;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getCpuPlayers() const
{
	return cpuPlayers;
}

void CRmgTemplate::setCpuPlayers(const CPlayerCountRange & value)
{
	cpuPlayers = value;
}

const std::map<TRmgTemplateZoneId, ZoneOptions *> & CRmgTemplate::getZones() const
{
	return zones;
}

void CRmgTemplate::setZones(const std::map<TRmgTemplateZoneId, ZoneOptions *> & value)
{
	zones = value;
}

const std::list<ZoneConnection> & CRmgTemplate::getConnections() const
{
	return connections;
}

void CRmgTemplate::setConnections(const std::list<ZoneConnection> & value)
{
	connections = value;
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
