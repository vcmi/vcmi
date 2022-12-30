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

TerrainTypeHandler::TerrainTypeHandler()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), CModHandler::scopeBuiltin());

	initRivers(allConfigs);
	recreateRiverMaps();
	initRoads(allConfigs);
	recreateRoadMaps();
	initTerrains(allConfigs); //maps will be populated inside
}

void TerrainTypeHandler::initTerrains(const std::vector<std::string> & allConfigs)
{
	std::vector<std::function<void()>> resolveLater;

	objects.resize(Terrain::ORIGINAL_TERRAIN_COUNT); //make space for original terrains

	for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourceID("config/terrains.json")))
			continue;
		
		JsonNode terrs(mod, ResourceID("config/terrains.json"));
		for(auto & terr : terrs.Struct())
		{
			TerrainType info(terr.first); //set name

			info.moveCost = static_cast<int>(terr.second["moveCost"].Integer());
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
				info.passabilityType = TerrainType::PassabilityType::LAND | TerrainType::PassabilityType::SURFACE;
			}
			else if (terr.second["type"].getType() == JsonNode::JsonType::DATA_VECTOR)
			{
				for(const auto& node : terr.second["type"].Vector())
				{
					//Set bits
					auto s = node.String();
					if (s == "LAND") info.passabilityType |= TerrainType::PassabilityType::LAND;
					if (s == "WATER") info.passabilityType |= TerrainType::PassabilityType::WATER;
					if (s == "ROCK") info.passabilityType |= TerrainType::PassabilityType::ROCK;
					if (s == "SURFACE") info.passabilityType |= TerrainType::PassabilityType::SURFACE;
					if (s == "SUB") info.passabilityType |= TerrainType::PassabilityType::SUBTERRANEAN;
				}
			}
			else  //should be string - one option only
			{
				auto s = terr.second["type"].String();
				if (s == "LAND") info.passabilityType = TerrainType::PassabilityType::LAND;
				if (s == "WATER") info.passabilityType = TerrainType::PassabilityType::WATER;
				if (s == "ROCK") info.passabilityType = TerrainType::PassabilityType::ROCK;
				if (s == "SURFACE") info.passabilityType = TerrainType::PassabilityType::SURFACE;
				if (s == "SUB") info.passabilityType = TerrainType::PassabilityType::SUBTERRANEAN;
			}
			
			if(terr.second["river"].isNull())
			{
				info.river = River::NO_RIVER;
			}
			else
			{
				info.river = getRiverByCode(terr.second["river"].String())->id;
			}
			
			if(terr.second["horseSoundId"].isNull())
			{
				info.horseSoundId = Terrain::ROCK; //rock sound as default
			}
			else
			{
				info.horseSoundId = static_cast<int>(terr.second["horseSoundId"].Float());
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

			if(!terr.second["originalTerrainId"].isNull())
			{
				//place in reserved slot
				info.id = (TerrainId)(terr.second["originalTerrainId"].Float());
				objects[info.id] = info;
			}
			else
			{
				//append at the end
				info.id = static_cast<TerrainId>(objects.size());
				objects.push_back(info);
			}
			TerrainId id = info.id;

			//Update terrain with this id in the future, after all terrain types are populated

			if(!terr.second["prohibitTransitions"].isNull())
			{
				for(auto & t : terr.second["prohibitTransitions"].Vector())
				{
					std::string prohibitedTerrainName = t.String();
					resolveLater.push_back([this, prohibitedTerrainName, id]()
					{
						//FIXME: is that reference to the element in vector?
						objects[id].prohibitTransitions.emplace_back(getInfoByName(prohibitedTerrainName)->id);
					});
				}
			}

			if(terr.second["rockTerrain"].isNull())
			{
				objects[id].rockTerrain = Terrain::ROCK;
			}
			else
			{
				auto rockTerrainName = terr.second["rockTerrain"].String();
				resolveLater.push_back([this, rockTerrainName, id]()
				{
					//FIXME: is that reference to the element in vector?
						objects[id].rockTerrain = getInfoByName(rockTerrainName)->id;
				});
			}
		}
	}

	for(size_t i = Terrain::FIRST_REGULAR_TERRAIN; i < Terrain::ORIGINAL_TERRAIN_COUNT; i++)
	{
		//Make sure that original terrains are loaded
		assert(objects[i].id != Terrain::WRONG);
	}

	recreateTerrainMaps();

	for(auto& functor : resolveLater)
	{
		functor();
	}
}

void TerrainTypeHandler::initRivers(const std::vector<std::string> & allConfigs)
{
	riverTypes.resize(River::ORIGINAL_RIVER_COUNT); //make space for original rivers
	//First object will be default NO_RIVER

	for(auto & mod : allConfigs)
	{
		if (!CResourceHandler::get(mod)->existsResource(ResourceID("config/rivers.json")))
			continue;

		JsonNode rivs(mod, ResourceID("config/rivers.json"));
		for(auto & river : rivs.Struct())
		{
			RiverType info;

			info.name = river.first;
			info.fileName = river.second["animation"].String();
			info.code = river.second["code"].String();
			info.deltaName = river.second["delta"].String();

			if (!river.second["originalRiverId"].isNull())
			{
				info.id = static_cast<RiverId>(river.second["originalRiverId"].Float());
				riverTypes[info.id] = info;
			}
			else
			{
				info.id = static_cast<RiverId>(riverTypes.size());
				riverTypes.push_back(info);
			}
		}
	}

	recreateRiverMaps();
}

void TerrainTypeHandler::initRoads(const std::vector<std::string> & allConfigs)
{
	roadTypes.resize(Road::ORIGINAL_ROAD_COUNT); //make space for original rivers
	//first object will be default NO_ROAD

	for(auto & mod : allConfigs)
	{
		if (!CResourceHandler::get(mod)->existsResource(ResourceID("config/roads.json")))
			continue;

		JsonNode rds(mod, ResourceID("config/roads.json"));
		for(auto & road : rds.Struct())
		{
			RoadType info;

			info.name = road.first;
			info.fileName = road.second["animation"].String();
			info.code = road.second["code"].String();
			info.movementCost = static_cast<ui8>(road.second["moveCost"].Float());

			if (!road.second["originalRoadId"].isNull())
			{
				info.id = static_cast<RoadId>(road.second["originalRoadId"].Float());
				roadTypes[info.id] = info;
			}
			else
			{
				info.id = static_cast<RoadId>(roadTypes.size());
				roadTypes.push_back(info);
			}
		}
	}

	recreateRoadMaps();
}

void TerrainTypeHandler::recreateTerrainMaps()
{
	//This assumes the vector will never be updated or reallocated in the future

	for(size_t i = 0; i < objects.size(); i++)
	{
		const auto * terrainInfo = &objects[i];

		terrainInfoByName[terrainInfo->name] = terrainInfo;
		terrainInfoByCode[terrainInfo->typeCode] = terrainInfo;
		terrainInfoById[terrainInfo->id] = terrainInfo;
	}
}

void TerrainTypeHandler::recreateRiverMaps()
{
	for(size_t i = River::FIRST_REGULAR_RIVER ; i < riverTypes.size(); i++)
	{
		const auto * riverInfo = &riverTypes[i];

		riverInfoByName[riverInfo->name] = riverInfo;
		riverInfoByCode[riverInfo->code] = riverInfo;
		riverInfoById[riverInfo->id] = riverInfo;
	}
}

void TerrainTypeHandler::recreateRoadMaps()
{
	for(size_t i = Road::FIRST_REGULAR_ROAD ; i < roadTypes.size(); i++)
	{
		const auto * roadInfo = &roadTypes[i];

		roadInfoByName[roadInfo->name] = roadInfo;
		roadInfoByCode[roadInfo->code] = roadInfo;
		roadInfoById[roadInfo->id] = roadInfo;
	}
}

const std::vector<TerrainType> & TerrainTypeHandler::terrains() const
{
	//FIXME: somehow make it non-copyable? Pointers must point to original data and not its copy
	return objects;
}

const std::vector<RiverType>& TerrainTypeHandler::rivers() const
{
	return riverTypes;
}

const std::vector<RoadType>& TerrainTypeHandler::roads() const
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

const TerrainType* TerrainTypeHandler::getInfoById(TerrainId id) const
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

const RiverType* TerrainTypeHandler::getRiverById(RiverId id) const
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

const RoadType* TerrainTypeHandler::getRoadById(RoadId id) const
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
	minimapBlocked({0,0,0}), //black
	minimapUnblocked({ 128,128,128 }), //grey
	name(_name),
	river(River::NO_RIVER),
	id(Terrain::WRONG),
	rockTerrain(Terrain::ROCK),
	moveCost(GameConstants::BASE_MOVEMENT_COST),
	horseSoundId(0),
	passabilityType(0),
	transitionRequired(false)
{
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

bool TerrainType::isSurfaceCartographerCompatible() const
{
	return isLand() && isPassable() && id != Terrain::SUBTERRANEAN;
}

bool TerrainType::isUndergroundCartographerCompatible() const
{
	return isLand() && isPassable() && id == Terrain::SUBTERRANEAN;
}

bool TerrainType::isTransitionRequired() const
{
	return transitionRequired;
}

RiverType::RiverType(const std::string & fileName, const std::string & code, RiverId id):
	fileName(fileName),
	code(code),
	id(id)
{
}

RoadType::RoadType(const std::string& fileName, const std::string& code, RoadId id):
	fileName(fileName),
	code(code),
	id(id),
	movementCost(GameConstants::BASE_MOVEMENT_COST)
{
}

VCMI_LIB_NAMESPACE_END
