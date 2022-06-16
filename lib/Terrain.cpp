/*
 * Terrain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "Terrain.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

const Terrain Terrain::ANY("ANY");

Terrain Terrain::createTerrainTypeH3M(int tId)
{
	static std::vector<std::string> terrainsH3M
	{
		"dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
	};
	return Terrain(terrainsH3M.at(tId));
}

Terrain::Manager::Manager()
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

Terrain::Manager & Terrain::Manager::get()
{
	static Terrain::Manager manager;
	return manager;
}

std::vector<Terrain> Terrain::Manager::terrains()
{
	std::vector<Terrain> _terrains;
	for(const auto & info : Terrain::Manager::get().terrainInfo)
		_terrains.push_back(info.first);
	return _terrains;
}

const JsonNode & Terrain::Manager::getInfo(const Terrain & terrain)
{
	return Terrain::Manager::get().terrainInfo.at(terrain.toString());
}

std::ostream & operator<<(std::ostream & os, const Terrain terrainType)
{
	return os << terrainType.toString();
}
	
std::string Terrain::toString() const
{
	return type;
}

Terrain::Terrain(const std::string & _type) : type(_type)
{}
	
Terrain& Terrain::operator=(const Terrain & _type)
{
	type = _type.type;
	return *this;
}
	
Terrain& Terrain::operator=(const std::string & _type)
{
	type = _type;
	return *this;
}

bool operator==(const Terrain & l, const Terrain & r)
{
	return l.type == r.type;
}

bool operator!=(const Terrain & l, const Terrain & r)
{
	return l.type != r.type;
}
	
bool operator<(const Terrain & l, const Terrain & r)
{
	return l.type < r.type;
}
	
int Terrain::id() const
{
	if(type == "ANY") return -3;
	if(type == "WRONG") return -2;
	if(type == "BORDER") return -1;
	
	auto _terrains = Terrain::Manager::terrains();
	auto iter = std::find(_terrains.begin(), _terrains.end(), *this);
	return iter - _terrains.begin();
}
	
bool Terrain::isLand() const
{
	return !isWater();
}
bool Terrain::isWater() const
{
	return Terrain::Manager::getInfo(*this)["type"].String() == "WATER";
}
bool Terrain::isPassable() const
{
	return Terrain::Manager::getInfo(*this)["type"].String() != "ROCK";
}
bool Terrain::isUnderground() const
{
	return Terrain::Manager::getInfo(*this)["type"].String() == "SUB";
}
