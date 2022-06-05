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

const ETerrainType ETerrainType::ANY("ANY");

const std::array<std::string, 10> ETerrainTypePredefined
{
	"dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
};

const std::set<ETerrainType> ETerrainType::DEFAULT_TERRAIN_TYPES
{
	ETerrainType("dirt"),
	ETerrainType("sand"),
	ETerrainType("grass"),
	ETerrainType("snow"),
	ETerrainType("swamp"),
	ETerrainType("rough"),
	ETerrainType("subterra"),
	ETerrainType("lava")
};

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
	
ETerrainType::ETerrainType(int _type)
{
	type = ETerrainTypePredefined[_type];
}
	
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

std::vector<ETerrainType> & ETerrainType::terrains()
{
	static std::vector<ETerrainType> _terrains(ETerrainTypePredefined.begin(), ETerrainTypePredefined.end());
	return _terrains;
}
	
int ETerrainType::id() const
{
	auto iter = std::find(ETerrainType::terrains().begin(), ETerrainType::terrains().end(), *this);
	if(iter != ETerrainType::terrains().end())
		return iter - ETerrainType::terrains().begin();
	
	ETerrainType::terrains().push_back(*this);
	return ETerrainType::terrains().size() - 1;
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
