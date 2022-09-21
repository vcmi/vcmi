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

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

TerrainTypeHandler::TerrainTypeHandler()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), "core");

	std::vector<std::function<void()>> resolveLater;

	objects.resize(Terrain::ORIGINAL_TERRAIN_COUNT, nullptr); //make space for original terrains

	for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourceID("config/terrains.json")))
			continue;
		
		JsonNode terrs(mod, ResourceID("config/terrains.json"));
		for(auto & terr : terrs.Struct())
		{
			auto * info = new TerrainType(terr.first); //set name

			info->moveCost = terr.second["moveCost"].Integer();
			const JsonVector &unblockedVec = terr.second["minimapUnblocked"].Vector();
			info->minimapUnblocked =
			{
				ui8(unblockedVec[0].Float()),
				ui8(unblockedVec[1].Float()),
				ui8(unblockedVec[2].Float())
			};
			
			const JsonVector &blockedVec = terr.second["minimapBlocked"].Vector();
			info->minimapBlocked =
			{
				ui8(blockedVec[0].Float()),
				ui8(blockedVec[1].Float()),
				ui8(blockedVec[2].Float())
			};
			info->musicFilename = terr.second["music"].String();
			info->tilesFilename = terr.second["tiles"].String();
			
			if(terr.second["type"].isNull())
			{
				info->passabilityType = TerrainType::PassabilityType::LAND;
			}
			else
			{
				auto s = terr.second["type"].String();
				if(s == "LAND") info->passabilityType = TerrainType::PassabilityType::LAND;
				if(s == "WATER") info->passabilityType = TerrainType::PassabilityType::WATER;
				if(s == "ROCK") info->passabilityType = TerrainType::PassabilityType::ROCK;
				if(s == "SUB") info->passabilityType = TerrainType::PassabilityType::SUBTERRANEAN;
			}
			
			if(terr.second["rockTerrain"].isNull())
			{
				info->rockTerrain = Terrain::ROCK;
			}
			else
			{
				auto rockTerrainType = terr.second["rockTerrain"].String();
				resolveLater.push_back([this, rockTerrainType, info]()
				{
						info->rockTerrain = getInfoByName(rockTerrainType)->id;
				});
			}
			
			if(terr.second["river"].isNull())
			{
				info->river = RIVER_NAMES[0];
			}
			else
			{
				info->river = terr.second["river"].String();
			}
			
			if(terr.second["horseSoundId"].isNull())
			{
				info->horseSoundId = 9; //rock sound as default
			}
			else
			{
				info->horseSoundId = terr.second["horseSoundId"].Float();
			}
			
			if(!terr.second["text"].isNull())
			{
				info->terrainText = terr.second["text"].String();
			}
			
			if(terr.second["code"].isNull())
			{
				info->typeCode = terr.first.substr(0, 2);
			}
			else
			{
				info->typeCode = terr.second["code"].String();
				assert(info->typeCode.length() == 2);
			}
			
			if(!terr.second["battleFields"].isNull())
			{
				for(auto & t : terr.second["battleFields"].Vector())
				{
					info->battleFields.emplace_back(t.String());
				}
			}
			
			if(!terr.second["prohibitTransitions"].isNull())
			{
				for(auto & t : terr.second["prohibitTransitions"].Vector())
				{
					std::string prohibitedTerrainName = t.String();
					resolveLater.push_back([this, prohibitedTerrainName, info]()
					{
						info->prohibitTransitions.emplace_back(getInfoByName(prohibitedTerrainName)->id);
					});
				}
			}
			
			info->transitionRequired = false;
			if(!terr.second["transitionRequired"].isNull())
			{
				info->transitionRequired = terr.second["transitionRequired"].Bool();
			}
			
			info->terrainViewPatterns = "normal";
			if(!terr.second["terrainViewPatterns"].isNull())
			{
				info->terrainViewPatterns = terr.second["terrainViewPatterns"].String();
			}

			TTerrain id = Terrain::WRONG;
			if(!terr.second["originalTerrainId"].isNull())
			{
				//place in reserved slot
				id = (TTerrain)(terr.second["originalTerrainId"].Float());
				objects[id] = info;
			}
			else
			{
				//append at the end
				id = objects.size();
				objects.push_back(info);
			}
		}
	}

	for (size_t i = Terrain::FIRST_REGULAR_TERRAIN; i < Terrain::ORIGINAL_TERRAIN_COUNT; i++)
	{
		//Make sure that original terrains are loaded
		assert(objects(i));
	}

	//TODO: add ids to resolve
	for (auto& functor : resolveLater)
	{
		functor();
	}
}

void TerrainTypeHandler::recreateTerrainMaps()
{
	for (const TerrainType * terrainInfo : objects)
	{
		terrainInfoByName[terrainInfo->name] = terrainInfo;
		terrainInfoByCode[terrainInfo->typeCode] = terrainInfo;
		terrainInfoById[terrainInfo->id] = terrainInfo;
	}
}

const std::vector<TerrainType *> & TerrainTypeHandler::terrains() const
{
	return objects;
}

const TerrainType* TerrainTypeHandler::getInfoByName(const std::string& terrainName) const
{
	return terrainInfoByName.at(terrainName);
}

const TerrainType* TerrainTypeHandler::getInfoByCode(const std::string& terrainCode) const
{
	return terrainInfoByCode.at(terrainCode);
}

const TerrainType* TerrainTypeHandler::getInfoById(TTerrain id) const
{
	return terrainInfoById.at(id);
}

std::ostream & operator<<(std::ostream & os, const TerrainType & terrainType)
{
	return os << static_cast<const std::string &>(terrainType);
}
	
TerrainType::operator std::string() const
{
	return name;
}
	
TerrainType::TerrainType(const std::string& _name):
	name(name),
	id(Terrain::WRONG),
	rockTerrain(Terrain::ROCK),
	moveCost(100),
	horseSoundId(0),
	passabilityType(PassabilityType::LAND),
	transitionRequired(false)
{
}

TerrainType& TerrainType::operator=(const TerrainType & other)
{
	//TODO
	name = other.name;
	return *this;
}
	
bool TerrainType::operator==(const TerrainType& other)
{
	return id == other.id;
}

bool TerrainType::operator!=(const TerrainType& other)
{
	return id != other.id;
}

bool TerrainType::operator<(const TerrainType& other)
{
	return id < other.id;
}
	
bool TerrainType::isLand() const
{
	return !isWater();
}

bool TerrainType::isWater() const
{
	return passabilityType == PassabilityType::WATER;
}

bool TerrainType::isPassable() const
{
	return passabilityType != PassabilityType::ROCK;
}

bool TerrainType::isUnderground() const
{
	return passabilityType != PassabilityType::SUBTERRANEAN;
}

bool TerrainType::isTransitionRequired() const
{
	return transitionRequired;
}
