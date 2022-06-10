/*
 * ETerrainType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CTerrainType.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

const CTerrainType CTerrainType::ANY("ANY");

CTerrainType CTerrainType::createTerrainTypeH3M(int tId)
{
	static std::vector<std::string> terrainsH3M
	{
		"dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
	};
	return CTerrainType(terrainsH3M.at(tId));
}

CTerrainType::Manager::Manager()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), "core");
	for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourceID("config/terrains.json")))
			continue;
		
		JsonNode terrs(mod, ResourceID("config/terrains.json"));
		for(auto & terr : terrs.Struct())
		{
			terrainInfo[terr.first] = terr.second;
			if(terr.second["type"].isNull())
			{
				terrainInfo[terr.first]["type"].setType(JsonNode::JsonType::DATA_STRING);
				terrainInfo[terr.first]["type"].String() = "LAND";
			}
		}
	}
}

CTerrainType::Manager & CTerrainType::Manager::get()
{
	static CTerrainType::Manager manager;
	return manager;
}

std::vector<CTerrainType> CTerrainType::Manager::terrains()
{
	std::vector<CTerrainType> _terrains;
	for(const auto & info : CTerrainType::Manager::get().terrainInfo)
		_terrains.push_back(info.first);
	return _terrains;
}

const JsonNode & CTerrainType::Manager::getInfo(const CTerrainType & terrain)
{
	return CTerrainType::Manager::get().terrainInfo.at(terrain.toString());
}

std::ostream & operator<<(std::ostream & os, const CTerrainType terrainType)
{
	return os << terrainType.toString();
}
	
std::string CTerrainType::toString() const
{
	return type;
}

CTerrainType::CTerrainType(const std::string & _type) : type(_type)
{}
	
CTerrainType& CTerrainType::operator=(const CTerrainType & _type)
{
	type = _type.type;
	return *this;
}
	
CTerrainType& CTerrainType::operator=(const std::string & _type)
{
	type = _type;
	return *this;
}

bool operator==(const CTerrainType & l, const CTerrainType & r)
{
	return l.type == r.type;
}

bool operator!=(const CTerrainType & l, const CTerrainType & r)
{
	return l.type != r.type;
}
	
bool operator<(const CTerrainType & l, const CTerrainType & r)
{
	return l.type < r.type;
}
	
int CTerrainType::id() const
{
	if(type == "ANY") return -3;
	if(type == "WRONG") return -2;
	if(type == "BORDER") return -1;
	
	auto _terrains = CTerrainType::Manager::terrains();
	auto iter = std::find(_terrains.begin(), _terrains.end(), *this);
	return iter - _terrains.begin();
}
	
bool CTerrainType::isLand() const
{
	return !isWater();
}
bool CTerrainType::isWater() const
{
	return CTerrainType::Manager::getInfo(*this)["type"].String() == "WATER";
}
bool CTerrainType::isPassable() const
{
	return CTerrainType::Manager::getInfo(*this)["type"].String() != "ROCK";
}
bool CTerrainType::isUnderground() const
{
	return CTerrainType::Manager::getInfo(*this)["type"].String() == "SUB";
}
