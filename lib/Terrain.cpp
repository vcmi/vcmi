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
	initRivers();
	initRoads();

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
				info->passabilityType = TerrainType::PassabilityType::LAND | TerrainType::PassabilityType::SURFACE;
			}
			else if (terr.second["type"].getType() == JsonNode::JsonType::DATA_VECTOR)
			{
				for (const auto& node : terr.second["type"].Vector())
				{
					//Set bits
					auto s = node.String();
					if (s == "LAND") info->passabilityType |= TerrainType::PassabilityType::LAND;
					if (s == "WATER") info->passabilityType |= TerrainType::PassabilityType::WATER;
					if (s == "ROCK") info->passabilityType |= TerrainType::PassabilityType::ROCK;
					if (s == "SURFACE") info->passabilityType |= TerrainType::PassabilityType::SURFACE;
					if (s == "SUB") info->passabilityType |= TerrainType::PassabilityType::SUBTERRANEAN;
				}
			}
			else  //should be string - one option only
			{
				auto s = terr.second["type"].String();
				if (s == "LAND") info->passabilityType = TerrainType::PassabilityType::LAND;
				if (s == "WATER") info->passabilityType = TerrainType::PassabilityType::WATER;
				if (s == "ROCK") info->passabilityType = TerrainType::PassabilityType::ROCK;
				if (s == "SURFACE") info->passabilityType = TerrainType::PassabilityType::SURFACE;
				if (s == "SUB") info->passabilityType = TerrainType::PassabilityType::SUBTERRANEAN;
			}
			
			if(terr.second["rockTerrain"].isNull())
			{
				info->rockTerrain = Terrain::ROCK;
			}
			else
			{
				auto rockTerrainName = terr.second["rockTerrain"].String();
				resolveLater.push_back([this, rockTerrainName, info]()
				{
						info->rockTerrain = getInfoByName(rockTerrainName)->id;
				});
			}
			
			if(terr.second["river"].isNull())
			{
				info->river = River::NO_RIVER;
			}
			else
			{
				info->river = getRiverByCode(terr.second["river"].String())->id;
			}
			
			if(terr.second["horseSoundId"].isNull())
			{
				info->horseSoundId = Terrain::ROCK; //rock sound as default
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
			info->id = id;
		}
	}

	for (size_t i = Terrain::FIRST_REGULAR_TERRAIN; i < Terrain::ORIGINAL_TERRAIN_COUNT; i++)
	{
		//Make sure that original terrains are loaded
		assert(objects(i));
	}

	recreateTerrainMaps();

	for (auto& functor : resolveLater)
	{
		functor();
	}
}

void TerrainTypeHandler::initRivers()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), "core");

	riverTypes.resize(River::ORIGINAL_RIVER_COUNT, nullptr); //make space for original rivers
	riverTypes[River::NO_RIVER] = new RiverType(); //default

	for (auto & mod : allConfigs)
	{
		if (!CResourceHandler::get(mod)->existsResource(ResourceID("config/rivers.json")))
			continue;

		JsonNode rivs(mod, ResourceID("config/rivers.json"));
		for (auto & river : rivs.Struct())
		{
			auto * info = new RiverType();

			info->fileName = river.second["animation"].String();
			info->code = river.second["code"].String();
			info->deltaName = river.second["delta"].String();
			//info->movementCost = river.second["moveCost"].Integer();

			if (!river.second["originalRiverId"].isNull())
			{
				info->id = static_cast<TRiver>(river.second["originalRiverId"].Float());
				riverTypes[info->id] = info;
			}
			else
			{
				info->id = riverTypes.size();
				riverTypes.push_back(info);
			}
		}
	}

	recreateRiverMaps();
}

void TerrainTypeHandler::initRoads()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), "core");

	roadTypes.resize(Road::ORIGINAL_ROAD_COUNT, nullptr); //make space for original rivers
	roadTypes[Road::NO_ROAD] = new RoadType(); //default

	for (auto & mod : allConfigs)
	{
		if (!CResourceHandler::get(mod)->existsResource(ResourceID("config/roads.json")))
			continue;

		JsonNode rds(mod, ResourceID("config/roads.json"));
		for (auto & road : rds.Struct())
		{
			auto * info = new RoadType();

			info->fileName = road.second["animation"].String();
			info->code = road.second["code"].String();
			info->movementCost = static_cast<ui8>(road.second["moveCost"].Float());

			if (!road.second["originalRoadId"].isNull())
			{
				info->id = static_cast<TRoad>(road.second["originalRoadId"].Float());
				roadTypes[info->id] = info;
			}
			else
			{
				info->id = roadTypes.size();
				roadTypes.push_back(info);
			}
		}
	}

	recreateRoadMaps();
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

void TerrainTypeHandler::recreateRiverMaps()
{
	for (const RiverType * riverInfo : riverTypes)
	{
		if (riverInfo->id == River::NO_RIVER)
			continue;

		riverInfoByName[riverInfo->fileName] = riverInfo;
		riverInfoByCode[riverInfo->code] = riverInfo;
		riverInfoById[riverInfo->id] = riverInfo;
	}
}

void TerrainTypeHandler::recreateRoadMaps()
{
	for (const RoadType * roadInfo : roadTypes)
	{
		if (roadInfo->id == Road::NO_ROAD)
			continue;

		roadInfoByName[roadInfo->fileName] = roadInfo;
		roadInfoByCode[roadInfo->code] = roadInfo;
		roadInfoById[roadInfo->id] = roadInfo;
	}
}

const std::vector<TerrainType *> & TerrainTypeHandler::terrains() const
{
	return objects;
}

const std::vector<RiverType*>& TerrainTypeHandler::rivers() const
{
	return riverTypes;
}

const std::vector<RoadType*>& TerrainTypeHandler::roads() const
{
	return roadTypes;
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

const RiverType* TerrainTypeHandler::getRiverByName(const std::string& riverName) const
{
	return riverInfoByName.at(riverName);
}

const RiverType* TerrainTypeHandler::getRiverByCode(const std::string& riverCode) const
{
	return riverInfoByCode.at(riverCode);
}

const RiverType* TerrainTypeHandler::getRiverById(TRiver id) const
{
	return riverInfoById.at(id);
}

const RoadType* TerrainTypeHandler::getRoadByName(const std::string& roadName) const
{
	return roadInfoByName.at(roadName);
}

const RoadType* TerrainTypeHandler::getRoadByCode(const std::string& roadCode) const
{
	return roadInfoByCode.at(roadCode);
}

const RoadType* TerrainTypeHandler::getRoadById(TRoad id) const
{
	return roadInfoById.at(id);
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
	name(_name),
	id(Terrain::WRONG),
	rockTerrain(Terrain::ROCK),
	moveCost(100),
	horseSoundId(0),
	passabilityType(0),
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
	return passabilityType & PassabilityType::WATER;
}

bool TerrainType::isPassable() const
{
	return !(passabilityType & PassabilityType::ROCK);
}

bool TerrainType::isSurface() const
{
	return passabilityType & PassabilityType::SURFACE;
}

bool TerrainType::isUnderground() const
{
	return passabilityType & PassabilityType::SUBTERRANEAN;
}

bool TerrainType::isTransitionRequired() const
{
	return transitionRequired;
}

RiverType::RiverType(const std::string & fileName, const std::string & code, TRiver id):
	fileName(fileName),
	code(code),
	id(id)
{
}

RoadType::RoadType(const std::string& fileName, const std::string& code, TRoad id):
	fileName(fileName),
	code(code),
	id(id),
	movementCost(GameConstants::BASE_MOVEMENT_COST)
{
}