/*
 * ETerrainType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ETerrainType.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

const ETerrainType ETerrainType::ANY("ANY");

ETerrainType ETerrainType::createTerrainTypeH3M(int tId)
{
	static std::vector<std::string> terrainsH3M
	{
		"dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
	};
	return ETerrainType(terrainsH3M.at(tId));
}

ETerrainType::Manager::Manager()
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
		}
	}
}

ETerrainType::Manager & ETerrainType::Manager::get()
{
	static ETerrainType::Manager manager;
	return manager;
}

std::vector<ETerrainType> ETerrainType::Manager::terrains()
{
	std::vector<ETerrainType> _terrains;
	for(const auto & info : ETerrainType::Manager::get().terrainInfo)
		_terrains.push_back(info.first);
	return _terrains;
}

const JsonNode & ETerrainType::Manager::getInfo(const ETerrainType & terrain)
{
	return ETerrainType::Manager::get().terrainInfo.at(terrain.toString());
}

std::ostream & operator<<(std::ostream & os, const ETerrainType terrainType)
{
	return os << terrainType.toString();
}
	
std::string ETerrainType::toString() const
{
	return type;
}

ETerrainType::ETerrainType(const std::string & _type) : type(_type)
{}
	
ETerrainType& ETerrainType::operator=(const ETerrainType & _type)
{
	type = _type.type;
	return *this;
}
	
ETerrainType& ETerrainType::operator=(const std::string & _type)
{
	type = _type;
	return *this;
}

bool operator==(const ETerrainType & l, const ETerrainType & r)
{
	return l.type == r.type;
}

bool operator!=(const ETerrainType & l, const ETerrainType & r)
{
	return l.type != r.type;
}
	
bool operator<(const ETerrainType & l, const ETerrainType & r)
{
	return l.type < r.type;
}
	
int ETerrainType::id() const
{
	if(type == "ANY") return -3;
	if(type == "WRONG") return -2;
	if(type == "BORDER") return -1;
	
	auto _terrains = ETerrainType::Manager::terrains();
	auto iter = std::find(_terrains.begin(), _terrains.end(), *this);
	return iter - _terrains.begin();
}
	
bool ETerrainType::isLand() const
{
	return type != "water";
}
bool ETerrainType::isWater() const
{
	return type == "water";
}
bool ETerrainType::isPassable() const
{
	return type != "rock";
}
bool ETerrainType::isUnderground() const
{
	return type != "subterra";
}
