/*
 * Terrain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Terrain.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

const Terrain Terrain::ANY("ANY");

Terrain Terrain::createTerrainTypeH3M(int tId)
{
	static std::array<std::string, 10> terrainsH3M
	{
		"dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
	};
	return Terrain(terrainsH3M.at(tId));
}

Terrain Terrain::createTerrainByCode(const std::string & typeCode)
{
	for(const auto & terrain : Manager::terrains())
	{
		if(Manager::getInfo(terrain).typeCode == typeCode)
			return terrain;
	}
	return Terrain::ANY;
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
			Terrain::Info info;
			info.moveCost = terr.second["moveCost"].Integer();
			const JsonVector &unblockedVec = terr.second["minimapUnblocked"].Vector();
			info.minimapUnblocked =
			{
				ui8(unblockedVec[0].Float()),
				ui8(unblockedVec[1].Float()),
				ui8(unblockedVec[2].Float())
			};
			
			const JsonVector &blockedVec = terr.second["minimapBlocked"].Vector();
			info.minimapBlocked =
			{
				ui8(blockedVec[0].Float()),
				ui8(blockedVec[1].Float()),
				ui8(blockedVec[2].Float())
			};
			info.musicFilename = terr.second["music"].String();
			info.tilesFilename = terr.second["tiles"].String();
			
			if(terr.second["type"].isNull())
			{
				info.type = Terrain::Info::Type::Land;
			}
			else
			{
				auto s = terr.second["type"].String();
				if(s == "LAND") info.type = Terrain::Info::Type::Land;
				if(s == "WATER") info.type = Terrain::Info::Type::Water;
				if(s == "ROCK") info.type = Terrain::Info::Type::Rock;
				if(s == "SUB") info.type = Terrain::Info::Type::Subterranean;
			}
			
			if(terr.second["rockTerrain"].isNull())
			{
				info.rockTerrain = "rock";
			}
			else
			{
				info.rockTerrain = terr.second["rockTerrain"].String();
			}
			
			if(terr.second["river"].isNull())
			{
				info.river = RIVER_NAMES[0];
			}
			else
			{
				info.river = terr.second["river"].String();
			}
			
			if(terr.second["horseSoundId"].isNull())
			{
				info.horseSoundId = 9; //rock sound as default
			}
			else
			{
				info.horseSoundId = terr.second["horseSoundId"].Integer();
			}
			
			if(!terr.second["text"].isNull())
			{
				info.terrainText = terr.second["text"].String();
			}
			
			if(terr.second["code"].isNull())
			{
				info.typeCode = terr.first.substr(0, 2);
			}
			else
			{
				info.typeCode = terr.second["code"].String();
				assert(info.typeCode.length() == 2);
			}
			
			if(!terr.second["battleFields"].isNull())
			{
				for(auto & t : terr.second["battleFields"].Vector())
				{
					info.battleFields.emplace_back(t.String());
				}
			}
			
			if(!terr.second["prohibitTransitions"].isNull())
			{
				for(auto & t : terr.second["prohibitTransitions"].Vector())
				{
					info.prohibitTransitions.emplace_back(t.String());
				}
			}
			
			info.transitionRequired = false;
			if(!terr.second["transitionRequired"].isNull())
			{
				info.transitionRequired = terr.second["transitionRequired"].Bool();
			}
			
			info.terrainViewPatterns = "normal";
			if(!terr.second["terrainViewPatterns"].isNull())
			{
				info.terrainViewPatterns = terr.second["terrainViewPatterns"].String();
			}
			
			terrainInfo[terr.first] = info;
			if(!terrainId.count(terr.first))
			{
				terrainId[terr.first] = terrainVault.size();
				terrainVault.push_back(terr.first);
			}
		}
	}
}

Terrain::Manager & Terrain::Manager::get()
{
	static Terrain::Manager manager;
	return manager;
}

const std::vector<Terrain> & Terrain::Manager::terrains()
{
	return Terrain::Manager::get().terrainVault;
}

int Terrain::Manager::id(const Terrain & terrain)
{
	if(terrain.name == "ANY") return -3;
	if(terrain.name == "WRONG") return -2;
	if(terrain.name == "BORDER") return -1;
	
	return Terrain::Manager::get().terrainId.at(terrain);
}

const Terrain::Info & Terrain::Manager::getInfo(const Terrain & terrain)
{
	return Terrain::Manager::get().terrainInfo.at(static_cast<std::string>(terrain));
}

std::ostream & operator<<(std::ostream & os, const Terrain terrainType)
{
	return os << static_cast<const std::string &>(terrainType);
}
	
Terrain::operator std::string() const
{
	return name;
}

Terrain::Terrain(const std::string & _name) : name(_name)
{}
	
Terrain& Terrain::operator=(const std::string & _name)
{
	name = _name;
	return *this;
}

bool operator==(const Terrain & l, const Terrain & r)
{
	return l.name == r.name;
}

bool operator!=(const Terrain & l, const Terrain & r)
{
	return l.name != r.name;
}
	
bool operator<(const Terrain & l, const Terrain & r)
{
	return l.name < r.name;
}
	
int Terrain::id() const
{
	return Terrain::Manager::id(*this);
}
	
bool Terrain::isLand() const
{
	return !isWater();
}
bool Terrain::isWater() const
{
	return Terrain::Manager::getInfo(*this).type == Terrain::Info::Type::Water;
}
bool Terrain::isPassable() const
{
	return Terrain::Manager::getInfo(*this).type != Terrain::Info::Type::Rock;
}
bool Terrain::isUnderground() const
{
	return Terrain::Manager::getInfo(*this).type == Terrain::Info::Type::Subterranean;
}
bool Terrain::isNative() const
{
	return name.empty();
}
bool Terrain::isTransitionRequired() const
{
	return Terrain::Manager::getInfo(*this).transitionRequired;
}

VCMI_LIB_NAMESPACE_END
