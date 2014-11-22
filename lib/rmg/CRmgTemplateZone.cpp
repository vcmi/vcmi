
/*
 * CRmgTemplateZone.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRmgTemplateZone.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"

#include "../VCMI_Lib.h"
#include "../CTownHandler.h"
#include "../CCreatureHandler.h"
#include "../CSpellHandler.h" //for choosing random spells

#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../mapObjects/CGPandoraBox.h"
#include "../mapObjects/CRewardableObject.h"

class CMap;
class CMapEditManager;
//class CGObjectInstance;

CRmgTemplateZone::CTownInfo::CTownInfo() : townCount(0), castleCount(0), townDensity(0), castleDensity(0)
{

}

int CRmgTemplateZone::CTownInfo::getTownCount() const
{
	return townCount;
}

void CRmgTemplateZone::CTownInfo::setTownCount(int value)
{
	if(value < 0)
		throw rmgException("Negative value for town count not allowed.");
	townCount = value;
}

int CRmgTemplateZone::CTownInfo::getCastleCount() const
{
	return castleCount;
}

void CRmgTemplateZone::CTownInfo::setCastleCount(int value)
{
	if(value < 0)
		throw rmgException("Negative value for castle count not allowed.");
	castleCount = value;
}

int CRmgTemplateZone::CTownInfo::getTownDensity() const
{
	return townDensity;
}

void CRmgTemplateZone::CTownInfo::setTownDensity(int value)
{
	if(value < 0)
		throw rmgException("Negative value for town density not allowed.");
	townDensity = value;
}

int CRmgTemplateZone::CTownInfo::getCastleDensity() const
{
	return castleDensity;
}

void CRmgTemplateZone::CTownInfo::setCastleDensity(int value)
{
	if(value < 0)
		throw rmgException("Negative value for castle density not allowed.");
	castleDensity = value;
}

CTileInfo::CTileInfo():nearestObjectDistance(INT_MAX), terrain(ETerrainType::WRONG) 
{
	occupied = ETileType::POSSIBLE; //all tiles are initially possible to place objects or passages
}

float CTileInfo::getNearestObjectDistance() const
{
	return nearestObjectDistance;
}

void CTileInfo::setNearestObjectDistance(float value)
{
	nearestObjectDistance = std::max<float>(0, value); //never negative (or unitialized)
}
bool CTileInfo::shouldBeBlocked() const
{
	return occupied == ETileType::BLOCKED;
}
bool CTileInfo::isBlocked() const
{
	return occupied == ETileType::BLOCKED || occupied == ETileType::USED;
}
bool CTileInfo::isPossible() const
{
	return occupied == ETileType::POSSIBLE;
}
bool CTileInfo::isFree() const
{
	return occupied == ETileType::FREE;
}
bool CTileInfo::isUsed() const
{
	return occupied == ETileType::USED;
}
void CTileInfo::setOccupied(ETileType::ETileType value)
{
	occupied = value;
}

ETerrainType CTileInfo::getTerrainType() const
{
	return terrain;
}

void CTileInfo::setTerrainType(ETerrainType value)
{
	terrain = value;
}

CRmgTemplateZone::CRmgTemplateZone() :
	id(0),
	type(ETemplateZoneType::PLAYER_START),
	size(1),
	townsAreSameType(false),
	matchTerrainToTown(true),
	townType(ETownType::NEUTRAL),
	terrainType (ETerrainType::GRASS),
	zoneMonsterStrength(EMonsterStrength::ZONE_NORMAL),
	totalDensity(0)
{
	terrainTypes = getDefaultTerrainTypes();
}

TRmgTemplateZoneId CRmgTemplateZone::getId() const
{
	return id;
}

void CRmgTemplateZone::setId(TRmgTemplateZoneId value)
{
	if(value <= 0)
		throw rmgException(boost::to_string(boost::format("Zone %d id should be greater than 0.") %id));
	id = value;
}

ETemplateZoneType::ETemplateZoneType CRmgTemplateZone::getType() const
{
	return type;
}
void CRmgTemplateZone::setType(ETemplateZoneType::ETemplateZoneType value)
{
	type = value;
}

int CRmgTemplateZone::getSize() const
{
	return size;
}

void CRmgTemplateZone::setSize(int value)
{
	if(value <= 0)
		throw rmgException(boost::to_string(boost::format("Zone %d size needs to be greater than 0.") % id));
	size = value;
}

boost::optional<int> CRmgTemplateZone::getOwner() const
{
	return owner;
}

void CRmgTemplateZone::setOwner(boost::optional<int> value)
{
	if(!(*value >= 0 && *value <= PlayerColor::PLAYER_LIMIT_I))
		throw rmgException(boost::to_string(boost::format ("Owner of zone %d has to be in range 0 to max player count.") %id));
	owner = value;
}

const CRmgTemplateZone::CTownInfo & CRmgTemplateZone::getPlayerTowns() const
{
	return playerTowns;
}

void CRmgTemplateZone::setPlayerTowns(const CTownInfo & value)
{
	playerTowns = value;
}

const CRmgTemplateZone::CTownInfo & CRmgTemplateZone::getNeutralTowns() const
{
	return neutralTowns;
}

void CRmgTemplateZone::setNeutralTowns(const CTownInfo & value)
{
	neutralTowns = value;
}

bool CRmgTemplateZone::getTownsAreSameType() const
{
	return townsAreSameType;
}

void CRmgTemplateZone::setTownsAreSameType(bool value)
{
	townsAreSameType = value;
}

const std::set<TFaction> & CRmgTemplateZone::getTownTypes() const
{
	return townTypes;
}

void CRmgTemplateZone::setTownTypes(const std::set<TFaction> & value)
{
	townTypes = value;
}
void CRmgTemplateZone::setMonsterTypes(const std::set<TFaction> & value)
{
	monsterTypes = value;
}

std::set<TFaction> CRmgTemplateZone::getDefaultTownTypes() const
{
	std::set<TFaction> defaultTowns;
	auto towns = VLC->townh->getDefaultAllowed();
	for(int i = 0; i < towns.size(); ++i)
	{
		if(towns[i]) defaultTowns.insert(i);
	}
	return defaultTowns;
}

bool CRmgTemplateZone::getMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

void CRmgTemplateZone::setMatchTerrainToTown(bool value)
{
	matchTerrainToTown = value;
}

const std::set<ETerrainType> & CRmgTemplateZone::getTerrainTypes() const
{
	return terrainTypes;
}

void CRmgTemplateZone::setTerrainTypes(const std::set<ETerrainType> & value)
{
	assert(value.find(ETerrainType::WRONG) == value.end() && value.find(ETerrainType::BORDER) == value.end() &&
		   value.find(ETerrainType::WATER) == value.end() && value.find(ETerrainType::ROCK) == value.end());
	terrainTypes = value;
}

std::set<ETerrainType> CRmgTemplateZone::getDefaultTerrainTypes() const
{
	std::set<ETerrainType> terTypes;
	static const ETerrainType::EETerrainType allowedTerTypes[] = {ETerrainType::DIRT, ETerrainType::SAND, ETerrainType::GRASS, ETerrainType::SNOW,
												   ETerrainType::SWAMP, ETerrainType::ROUGH, ETerrainType::SUBTERRANEAN, ETerrainType::LAVA};
	for (auto & allowedTerType : allowedTerTypes)
		terTypes.insert(allowedTerType);

	return terTypes;
}

void CRmgTemplateZone::setMinesAmount (TResource res, ui16 amount)
{
	mines[res] = amount;
}

std::map<TResource, ui16> CRmgTemplateZone::getMinesInfo() const
{
	return mines;
}

void CRmgTemplateZone::addConnection(TRmgTemplateZoneId otherZone)
{
	connections.push_back (otherZone);
}

std::vector<TRmgTemplateZoneId> CRmgTemplateZone::getConnections() const
{
	return connections;
}

void CRmgTemplateZone::setMonsterStrength (EMonsterStrength::EMonsterStrength val)
{
	assert (vstd::iswithin(val, EMonsterStrength::ZONE_WEAK, EMonsterStrength::ZONE_STRONG));
	zoneMonsterStrength = val;
}

void CRmgTemplateZone::setTotalDensity (ui16 val)
{
	totalDensity = val;
}

ui16 CRmgTemplateZone::getTotalDensity () const
{
	return totalDensity;
}

void CRmgTemplateZone::addTreasureInfo(CTreasureInfo & info)
{
	treasureInfo.push_back(info);
}

std::vector<CTreasureInfo> CRmgTemplateZone::getTreasureInfo()
{
	return treasureInfo;
}

std::set<int3>* CRmgTemplateZone::getFreePaths()
{
	return &freePaths;
}

float3 CRmgTemplateZone::getCenter() const
{
	return center;
}
void CRmgTemplateZone::setCenter(const float3 &f)
{
	//limit boundaries to (0,1) square
	center = float3 (std::min(std::max(f.x, 0.f), 1.f), std::min(std::max(f.y, 0.f), 1.f), f.z);
}


bool CRmgTemplateZone::pointIsIn(int x, int y)
{
	return true;
}

int3 CRmgTemplateZone::getPos() const
{
	return pos;
}
void CRmgTemplateZone::setPos(const int3 &Pos)
{
	pos = Pos;
}

void CRmgTemplateZone::addTile (const int3 &pos)
{
	tileinfo.insert(pos);
}

std::set<int3> CRmgTemplateZone::getTileInfo () const
{
	return tileinfo;
}

void CRmgTemplateZone::discardDistantTiles (CMapGenerator* gen, float distance)
{
	//TODO: mark tiles beyond zone as unavailable, but allow to connect with adjacent zones

	//for (auto tile : tileinfo)
	//{
	//	if (tile.dist2d(this->pos) > distance)
	//	{
	//		gen->setOccupied(tile, ETileType::USED);
	//		//gen->setOccupied(tile, ETileType::BLOCKED); //fixme: crash at rendering?
	//	}
	//}
	vstd::erase_if (tileinfo, [distance, this](const int3 &tile) -> bool
	{
		return tile.dist2d(this->pos) > distance;
	});
}

void CRmgTemplateZone::initFreeTiles (CMapGenerator* gen)
{
	vstd::copy_if (tileinfo, vstd::set_inserter(possibleTiles), [gen](const int3 &tile) -> bool
	{
		return gen->isPossible(tile);
	});
}

void CRmgTemplateZone::createBorder(CMapGenerator* gen)
{
	for (auto tile : tileinfo)
	{
		gen->foreach_neighbour (tile, [this, gen](int3 &pos)
		{
			if (!vstd::contains(this->tileinfo, pos))
			{
				gen->foreach_neighbour (pos, [this, gen](int3 &pos)
				{
					if (gen->isPossible(pos))
						gen->setOccupied (pos, ETileType::BLOCKED);
				});
			}
		});
	}
}

void CRmgTemplateZone::fractalize(CMapGenerator* gen)
{
	for (auto tile : tileinfo)
	{
		if (gen->isFree(tile))
			freePaths.insert(tile);
	}
	std::vector<int3> clearedTiles (freePaths.begin(), freePaths.end());
	std::set<int3> possibleTiles;
	std::set<int3> tilesToClear; //will be set clear
	std::set<int3> tilesToIgnore; //will be erased in this iteration

	//the more treasure density, the greater distance between paths. Scaling is experimental.
	const float minDistance = std::sqrt(totalDensity * 3);
	for (auto tile : tileinfo)
	{
		if (gen->isFree(tile))
			clearedTiles.push_back(tile);
		else if (gen->isPossible(tile))
			possibleTiles.insert(tile);
	}
	assert (clearedTiles.size()); //this should come from zone connections

	while (possibleTiles.size())
	{
		//link tiles in random order
		std::vector<int3> tilesToMakePath(possibleTiles.begin(), possibleTiles.end());
		RandomGeneratorUtil::randomShuffle(tilesToMakePath, gen->rand);

		for (auto tileToMakePath : tilesToMakePath)
		{
			//find closest free tile
			float currentDistance = 1e10;
			int3 closestTile (-1,-1,-1);

			for (auto clearTile : clearedTiles)
			{
				float distance = tileToMakePath.dist2d(clearTile);
				
				if (distance < currentDistance)
				{
					currentDistance = distance;
					closestTile = clearTile;
				}
				if (currentDistance <= minDistance)
				{
					//this tile is close enough. Forget about it and check next one
					tilesToIgnore.insert (tileToMakePath);
					break;
				}
			}
			//if tiles is not close enough, make path to it
			if (currentDistance > minDistance)
			{
				crunchPath (gen, tileToMakePath, closestTile, id, &tilesToClear);
				break; //next iteration - use already cleared tiles
			}
		}

		for (auto tileToClear : tilesToClear)
		{
			//move cleared tiles from one set to another
			clearedTiles.push_back(tileToClear);
			vstd::erase_if_present(possibleTiles, tileToClear);
		}
		for (auto tileToClear : tilesToIgnore)
		{
			//these tiles are already connected, ignore them
			vstd::erase_if_present(possibleTiles, tileToClear);
		}
		if (tilesToClear.empty()) //nothing else can be done (?)
			break;
		tilesToClear.clear(); //empty this container
		tilesToIgnore.clear();
	}

	for (auto tile : clearedTiles)
	{
		freePaths.insert(tile);
	}

	

	if (0) //enable to debug
	{
		std::ofstream out(boost::to_string(boost::format("zone %d") % id));
		int levels = gen->map->twoLevel ? 2 : 1;
		int width =  gen->map->width;
		int height = gen->map->height;
		for (int k = 0; k < levels; k++)
		{
			for(int j=0; j<height; j++)
			{
				for (int i=0; i<width; i++)
				{
					out << (int)vstd::contains(freePaths, int3(i,j,k));
				}
				out << std::endl;
			}
			out << std::endl;
		}
	}

	//logGlobal->infoStream() << boost::format ("Zone %d subdivided fractally") %id;
}

bool CRmgTemplateZone::crunchPath (CMapGenerator* gen, const int3 &src, const int3 &dst, TRmgTemplateZoneId zone, std::set<int3>* clearedTiles)
{
/*
make shortest path with free tiles, reachning dst or closest already free tile. Avoid blocks.
do not leave zone border
*/
	bool result = false;
	bool end = false;

	int3 currentPos = src;
	float distance = currentPos.dist2dSQ (dst);

	while (!end)
	{
		if (currentPos == dst)
		{
			result = true;
			break;
		}

		auto lastDistance = distance;
		gen->foreach_neighbour (currentPos, [this, gen, &currentPos, dst, &distance, &result, &end, clearedTiles](int3 &pos)
		{
			if (!result) //not sure if lambda is worth it...
			{
				if (pos == dst)
				{
					result = true;
					end = true;
				}
				if (pos.dist2dSQ (dst) < distance)
				{
					if (!gen->isBlocked(pos))
					{
						if (vstd::contains (tileinfo, pos))
						{
							if (gen->isPossible(pos))
							{
								gen->setOccupied (pos, ETileType::FREE);
								if (clearedTiles)
									clearedTiles->insert(pos);
								currentPos = pos;
								distance = currentPos.dist2dSQ (dst);
							}
							else if (gen->isFree(pos))
							{
								end = true;
								result = true;
							}
						}
					}
				}
			}
		});

		int3 anotherPos(-1, -1, -1);

		if (!(result || distance < lastDistance)) //we do not advance, use more advaced pathfinding algorithm?
		{
			//try any nearby tiles, even if its not closer than current
			float lastDistance = 2 * distance; //start with significantly larger value
			gen->foreach_neighbour(currentPos, [this, gen, &currentPos, dst, &lastDistance, &anotherPos, &end, clearedTiles](int3 &pos)
			{
				if (currentPos.dist2dSQ(dst) < lastDistance) //try closest tiles from all surrounding unused tiles
				{
					if (vstd::contains(tileinfo, pos))
					{
						if (gen->isPossible(pos))
						{
							if (clearedTiles)
								clearedTiles->insert(pos);
							anotherPos = pos;
							lastDistance = currentPos.dist2dSQ(dst);
						}
					}
				}
			});
			if (anotherPos.valid())
			{
				if (clearedTiles)
					clearedTiles->insert(anotherPos);
				gen->setOccupied(anotherPos, ETileType::FREE);
				currentPos = anotherPos;
			}
		}
		if (!(result || distance < lastDistance || anotherPos.valid()))
		{
			logGlobal->warnStream() << boost::format("No tile closer than %s found on path from %s to %s") % currentPos %src %dst;
			break;
		}
	}

	return result;
}

void CRmgTemplateZone::addRequiredObject(CGObjectInstance * obj, si32 strength)
{
	requiredObjects.push_back(std::make_pair(obj, strength));
}

bool CRmgTemplateZone::addMonster(CMapGenerator* gen, int3 &pos, si32 strength, bool clearSurroundingTiles, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426

	int mapMonsterStrength = gen->mapGenOptions->getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};

	int strength1 = std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]); 
	int strength2 = std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]); 

	strength = strength1 + strength2;
	if (strength < 2000)
		return false; //no guard at all

	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for (auto cre : VLC->creh->creatures)
	{
		if (cre->special)
			continue;
		if (!vstd::contains(monsterTypes, cre->faction))
			continue;
		if ((cre->AIValue * (cre->ammMin + cre->ammMax) / 2 < strength) && (strength < cre->AIValue * 100)) //at least one full monster. size between minimum size of given stack and 100
		{
			possibleCreatures.push_back(cre->idNumber);
		}
	}
	if (possibleCreatures.size())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, gen->rand);
		amount = strength / VLC->creh->creatures[creId]->AIValue;
		if (amount >= 4)
			amount *= gen->rand.nextDouble(0.75, 1.25);
	}
	else //just pick any available creature
	{
		creId = CreatureID(132); //Azure Dragon
		amount = strength / VLC->creh->creatures[creId]->AIValue;
	}


	auto guard = new CGCreature();
	guard->ID = Obj::MONSTER;
	guard->subID = creId;
	guard->character = 1; //MUST be initialized or switch will diverge
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);

	//logGlobal->traceStream() << boost::format ("Adding stack of %d %s. Map monster strenght %d, zone monster strength %d, base monster value %d")
	//	% amount % VLC->creh->creatures[creId]->namePl % mapMonsterStrength % zoneMonsterStrength % strength;

	placeObject(gen, guard, pos);

	if (clearSurroundingTiles)
	{
		//do not spawn anything near monster
		gen->foreach_neighbour (pos, [gen](int3 pos)
		{
			if (gen->isPossible(pos))
				gen->setOccupied(pos, ETileType::FREE);
		});
	}

	return true;
}

bool CRmgTemplateZone::createTreasurePile (CMapGenerator* gen, int3 &pos, float minDistance)
{
	CTreasurePileInfo info;

	std::map<int3, CGObjectInstance *> treasures;
	std::set<int3> boundary;
	int3 guardPos (-1,-1,-1);
	info.nextTreasurePos = pos;

	//default values
	int maxValue = 5000;
	int minValue = 1500;

	if (treasureInfo.size())
	{
		//roulette wheel
		int r = gen->rand.nextInt (1, totalDensity);

		for (auto t : treasureInfo)
		{
			if (r <= t.threshold)
			{
				maxValue = t.max;
				minValue = t.min;
				break;
			}
		}
	}
	ui32 desiredValue = gen->rand.nextInt(minValue, maxValue);
	//quantize value to let objects with value equal to max spawn too

	int currentValue = 0;
	CGObjectInstance * object = nullptr;
	while (currentValue < desiredValue)
	{
		treasures[info.nextTreasurePos] = nullptr;

		for (auto treasurePos : treasures)
		{
			gen->foreach_neighbour (treasurePos.first, [gen, &boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}
		for (auto treasurePos : treasures)
		{
			//leaving only boundary around objects
			vstd::erase_if_present (boundary, treasurePos.first);
		}

		for (auto tile : boundary)
		{
			//we can't extend boundary anymore
			if (!(gen->isBlocked(tile) || gen->isPossible(tile)))
				break;
		}

		ObjectInfo oi = getRandomObject(gen, info, desiredValue, maxValue, currentValue);
		if (!oi.value) //0 value indicates no object
		{
			vstd::erase_if_present(treasures, info.nextTreasurePos);
			break;
		}
		else
		{
			object = oi.generateObject();

			//remove from possible objects
			auto oiptr = std::find(possibleObjects.begin(), possibleObjects.end(), oi);
			assert (oiptr != possibleObjects.end());
			oiptr->maxPerZone--;
			if (!oiptr->maxPerZone)
				possibleObjects.erase(oiptr);
			//TODO

			//update treasure pile area
			int3 visitablePos = info.nextTreasurePos;

			//TODO: actually we need to check is object is either !blockVisit or removable after visit - this means object below can be accessed
			if (oi.templ.isVisitableFromTop())
				info.visitableFromTopPositions.insert(visitablePos); //can be accessed from any direction
			else
				info.visitableFromBottomPositions.insert(visitablePos); //can be accessed only from bottom or side

			for (auto blockedOffset : oi.templ.getBlockedOffsets())
			{
				int3 blockPos = info.nextTreasurePos + blockedOffset + oi.templ.getVisitableOffset(); //object will be moved to align vistable pos to treasure pos
				info.occupiedPositions.insert(blockPos);
				info.blockedPositions.insert(blockPos);
			}
			info.occupiedPositions.insert(visitablePos);

			currentValue += oi.value;
		
			treasures[info.nextTreasurePos] = object;

			//now find place for next object
			int3 placeFound(-1,-1,-1);

			//randomize next position from among possible ones
			std::vector<int3> boundaryCopy (boundary.begin(), boundary.end());
			//RandomGeneratorUtil::randomShuffle(boundaryCopy, gen->rand);
			auto chooseTopTile = [](const int3 & lhs, const int3 & rhs) -> bool
			{
				return lhs.y < rhs.y;
			};
			boost::sort(boundaryCopy, chooseTopTile); //start from top tiles to allow objects accessible from bottom

			for (auto tile : boundaryCopy)
			{
				if (gen->isPossible(tile)) //we can place new treasure only on possible tile
				{
					bool here = true;
					gen->foreach_neighbour (tile, [gen, &here, minDistance](int3 pos)
					{
						if (!(gen->isBlocked(pos) || gen->isPossible(pos)) || gen->getNearestObjectDistance(pos) < minDistance)
							here = false;
					});
					if (here)
					{
						placeFound = tile;
						break;
					}
				}
			}
			if (placeFound.valid())
				info.nextTreasurePos = placeFound;
			}
	}

	if (treasures.size())
	{
		//find object closest to zone center, then connect it to the middle of the zone
		int3 closestFreeTile (-1,-1,-1);
		if (info.visitableFromBottomPositions.size()) //get random treasure tile, starting from objects accessible only from bottom
			closestFreeTile = findClosestTile (freePaths, *RandomGeneratorUtil::nextItem(info.visitableFromBottomPositions, gen->rand));
		else
			closestFreeTile = findClosestTile (freePaths, *RandomGeneratorUtil::nextItem(info.visitableFromTopPositions, gen->rand));

		int3 closestTile = int3(-1,-1,-1);
		float minDistance = 1e10;
		for (auto visitablePos : info.visitableFromBottomPositions) //objects that are not visitable from top must be accessible from bottom or side
		{
			if (closestFreeTile.dist2d(visitablePos) < minDistance)
			{
				closestTile = visitablePos + int3 (0, 1, 0); //start below object (y+1), possibly even outside the map (?)
				minDistance = closestFreeTile.dist2d(visitablePos);
			}
		}
		if (!closestTile.valid())
		{
			for (auto visitablePos : info.visitableFromTopPositions) //all objects are accessible from any direction
			{
				if (closestFreeTile.dist2d(visitablePos) < minDistance)
				{
					closestTile = visitablePos;
					minDistance = closestFreeTile.dist2d(visitablePos);
				}
			}
		}
		assert (closestTile.valid());

		for (auto tile : info.occupiedPositions)
		{
			if (gen->map->isInTheMap(tile)) //pile boundary may reach map border
				gen->setOccupied(tile, ETileType::BLOCKED); //so that crunch path doesn't cut through objects
		}

		if (!crunchPath (gen, closestTile, closestFreeTile, id))
		{
			//we can't connect this pile, just block it off and start over
			for (auto treasure : treasures)
			{
				if (gen->isPossible(treasure.first))
					gen->setOccupied (treasure.first, ETileType::BLOCKED);
			}
			return true;
		}

		//update boundary around our objects, including knowledge about objects visitable from bottom

		boundary.clear();
		for (auto tile : info.visitableFromBottomPositions)
		{
			gen->foreach_neighbour (tile, [tile, &boundary](int3 pos)
			{
				if (pos.y >= tile.y) //don't block these objects from above
					boundary.insert(pos);
			});
		}
		for (auto tile : info.visitableFromTopPositions)
		{
			gen->foreach_neighbour (tile, [&boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}

		for (auto tile : boundary) //guard must be standing there
		{
			if (gen->isFree(tile)) //this tile could be already blocked, don't place a monster here
			{
				guardPos = tile;
				break;
			}
		}

		if (guardPos.valid())
		{
			for (auto treasure : treasures)
			{
				int3 visitableOffset = treasure.second->getVisitableOffset();
				placeObject(gen, treasure.second, treasure.first + visitableOffset);
			}
			if (addMonster(gen, guardPos, currentValue, false))
			{//block only if the object is guarded
				for (auto tile : boundary)
				{
					if (gen->isPossible(tile))
						gen->setOccupied (tile, ETileType::BLOCKED);
				}
				//do not spawn anything near monster
				gen->foreach_neighbour (guardPos, [gen](int3 pos)
				{
					if (gen->isPossible(pos))
						gen->setOccupied(pos, ETileType::FREE);
				});
			}
		}
		else //we couldn't make a connection to this location, block it
		{
			for (auto treasure : treasures)
			{
				if (gen->isPossible(treasure.first))
					gen->setOccupied (treasure.first, ETileType::BLOCKED);

				delete treasure.second;
			}
		}

		return true;
	}
	else //we did not place eveyrthing successfully
		return false;
}
void CRmgTemplateZone::initTownType (CMapGenerator* gen)
{
	//FIXME: handle case that this player is not present -> towns should be set to neutral
	int totalTowns = 0;

	auto cutPathAroundTown = [gen, this](const CGTownInstance * town)
	{
		//cut contour around town in case it was placed in a middle of path. TODO: find better solution
		for (auto tile : town->getBlockedPos())
		{
			gen->foreach_neighbour(tile, [gen, &tile](int3& pos)
			{
				if (gen->isPossible(pos))
				{
					gen->setOccupied(pos, ETileType::FREE);
				}
			});
		}
	};

	auto addNewTowns = [&totalTowns, gen, this, &cutPathAroundTown](int count, bool hasFort, PlayerColor player)
	{
		for (int i = 0; i < count; i++)
		{
			auto town = new CGTownInstance();
			town->ID = Obj::TOWN;

			if (this->townsAreSameType)
				town->subID = townType;
			else
			{
				if (townTypes.size())
					town->subID = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
				else
					town->subID = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed
			}

			town->tempOwner = player;
			if (hasFort)
				town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);

			for (auto spell : VLC->spellh->objects) //add all regular spells to town
			{
				if (!spell->isSpecialSpell() && !spell->isCreatureAbility())
					town->possibleSpells.push_back(spell->id);
			}

			if (!totalTowns) 
			{
				//first town in zone sets the facton of entire zone
				town->subID = townType;
				//register MAIN town of zone
				gen->registerZone(town->subID);
				//first town in zone goes in the middle
				placeAndGuardObject(gen, town, getPos() + town->getVisitableOffset(), 0);
				cutPathAroundTown(town);
			}
			else
				addRequiredObject (town);
			totalTowns++;
		}
	};


	if ((type == ETemplateZoneType::CPU_START) || (type == ETemplateZoneType::PLAYER_START))
	{
		//set zone types to player faction, generate main town
		logGlobal->infoStream() << "Preparing playing zone";
		int player_id = *owner - 1;
		auto & playerInfo = gen->map->players[player_id];
		if (playerInfo.canAnyonePlay())
		{
			PlayerColor player(player_id);
			townType = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if (townType == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
			{
				if (townTypes.size())
					townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
				else
					townType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed
			}
			
			auto  town = new CGTownInstance();
			town->ID = Obj::TOWN;

			town->subID = townType;
			town->tempOwner = player;
			town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);

			for (auto spell : VLC->spellh->objects) //add all regular spells to town
			{
				if (!spell->isSpecialSpell() && !spell->isCreatureAbility())
					town->possibleSpells.push_back(spell->id);
			}
			//towns are big objects and should be centered around visitable position
			placeAndGuardObject(gen, town, getPos() + town->getVisitableOffset(), 0); //generate no guards, but free path to entrance
			cutPathAroundTown(town);

			totalTowns++;
			//register MAIN town of zone only
			gen->registerZone (town->subID);

			logGlobal->traceStream() << "Fill player info " << player_id;

			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert (townType);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos - town->getVisitableOffset();
			playerInfo.generateHeroAtMainTown = true;

			//now create actual towns
			addNewTowns (playerTowns.getCastleCount() - 1, true, player);
			addNewTowns (playerTowns.getTownCount(), false, player);

			//requiredObjects.push_back(town);
		}
		else
		{			
			type = ETemplateZoneType::TREASURE;
			if (townTypes.size())
				townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
			else
				townType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed
		}
	}
	else //no player
	{
		if (townTypes.size())
			townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
		else
			townType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed
	}

	addNewTowns (neutralTowns.getCastleCount(), true, PlayerColor::NEUTRAL);
	addNewTowns (neutralTowns.getTownCount(), false, PlayerColor::NEUTRAL);

	if (!totalTowns) //if there's no town present, get random faction for dwellings and pandoras
	{
		//25% chance for neutral
		if (gen->rand.nextInt(1, 100) <= 25)
		{
			townType = ETownType::NEUTRAL;
		}
		else
		{
			if (townTypes.size())
				townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
			else if (monsterTypes.size())
				townType = *RandomGeneratorUtil::nextItem(monsterTypes, gen->rand); //this happens in Clash of Dragons in treasure zones, where all towns are banned
		}

	}
}

void CRmgTemplateZone::initTerrainType (CMapGenerator* gen)
{

	if (matchTerrainToTown && townType != ETownType::NEUTRAL)
		terrainType = VLC->townh->factions[townType]->nativeTerrain;
	else
		terrainType = *RandomGeneratorUtil::nextItem(terrainTypes, gen->rand);

	//TODO: allow new types of terrain?
	if (pos.z)
	{
		if (terrainType != ETerrainType::LAVA)
			terrainType = ETerrainType::SUBTERRANEAN;
	}
	else
	{
		if (terrainType == ETerrainType::SUBTERRANEAN)
			terrainType = ETerrainType::DIRT;
	}

	paintZoneTerrain (gen, terrainType);
}

void CRmgTemplateZone::paintZoneTerrain (CMapGenerator* gen, ETerrainType terrainType)
{
	std::vector<int3> tiles;
	for (auto tile : tileinfo)
	{
		tiles.push_back (tile);
	}
	gen->editManager->getTerrainSelection().setSelection(tiles);
	gen->editManager->drawTerrain(terrainType, &gen->rand);
}

bool CRmgTemplateZone::placeMines (CMapGenerator* gen)
{
	std::vector<Res::ERes> required_mines;
	required_mines.push_back(Res::ERes::WOOD);
	required_mines.push_back(Res::ERes::ORE);

	static const Res::ERes woodOre[] = {Res::ERes::WOOD, Res::ERes::ORE};
	static const Res::ERes preciousResources[] = {Res::ERes::GEMS, Res::ERes::CRYSTAL, Res::ERes::MERCURY, Res::ERes::SULFUR};


	//TODO: factory / copy constructor?
	for (const auto & res : woodOre)
	{
		//TODO: these 2 should be close to town (within 12 tiles radius)
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = new CGMine();
			mine->ID = Obj::MINE;
			mine->subID = static_cast<si32>(res);
			mine->producedResource = res;
			mine->producedQuantity = mine->defaultResProduction();
			addRequiredObject(mine, 1500);
		}
	}
	for (const auto & res : preciousResources)
	{
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = new CGMine();
			mine->ID = Obj::MINE;
			mine->subID = static_cast<si32>(res);
			mine->producedResource = res;
			mine->producedQuantity = mine->defaultResProduction();
			addRequiredObject(mine, 3500);
		}
	}
	for (int i = 0; i < mines[Res::GOLD]; i++)
	{
		auto mine = new CGMine();
		mine->ID = Obj::MINE;
		mine->subID = static_cast<si32>(Res::GOLD);
		mine->producedResource = Res::GOLD;
		mine->producedQuantity = mine->defaultResProduction();
		addRequiredObject(mine, 7000);
	}

	return true;
}

bool CRmgTemplateZone::createRequiredObjects(CMapGenerator* gen)
{
	logGlobal->traceStream() << "Creating required objects";
	for(const auto &obj : requiredObjects)
	{
		int3 pos;
		if ( ! findPlaceForObject(gen, obj.first, 3, pos))		
		{
			logGlobal->errorStream() << boost::format("Failed to fill zone %d due to lack of space") %id;
			//TODO CLEANUP!
			return false;
		}

		placeObject (gen, obj.first, pos);
		guardObject (gen, obj.first, obj.second, (obj.first->ID == Obj::MONOLITH_TWO_WAY), true);
		//paths to required objects constitute main paths of zone. otherwise they just may lead to middle and create dead zones
	}
	return true;
}

void CRmgTemplateZone::createTreasures(CMapGenerator* gen)
{
	//treasure density is proportional to map siz,e but must be scaled bakc to map size
	//also, normalize it to zone count - higher count means relative smaller zones

	//this is squared distance for optimization purposes
	const double minDistance = std::max<float>((600.f * size * size * gen->getZones().size()) /
		(gen->mapGenOptions->getWidth() * gen->mapGenOptions->getHeight() * totalDensity * (gen->map->twoLevel ? 2 : 1)), 2);
	//distance lower than 2 causes objects to overlap and crash

	do {
		
		//optimization - don't check tiles which are not allowed
		vstd::erase_if (possibleTiles, [gen](const int3 &tile) -> bool
		{
			return !gen->isPossible(tile);
		});

		int3 pos;
		if ( ! findPlaceForTreasurePile(gen, minDistance, pos))		
		{
			break;
		}
		createTreasurePile (gen, pos, minDistance);

	} while(true);
}

void CRmgTemplateZone::createObstacles(CMapGenerator* gen)
{
	//tighten obstacles to improve visuals

	for (int i = 0; i < 3; ++i)
	{
		int blockedTiles = 0;
		int freeTiles = 0;

		for (auto tile : tileinfo)
		{
			if (!gen->isPossible(tile)) //only possible tiles can change
				continue;

			int blockedNeighbours = 0;
			int freeNeighbours = 0;
			gen->foreach_neighbour(tile, [gen, &blockedNeighbours, &freeNeighbours](int3 &pos)
			{
				if (gen->isBlocked(pos))
					blockedNeighbours++;
				if (gen->isFree(pos))
					freeNeighbours++;
			});
			if (blockedNeighbours > 4)
			{
				gen->setOccupied(tile, ETileType::BLOCKED);
				blockedTiles++;
			}
			else if (freeNeighbours > 4)
			{
				gen->setOccupied(tile, ETileType::FREE);
				freeTiles++;
			}
		}
		logGlobal->traceStream() << boost::format("Set %d tiles to BLOCKED and %d tiles to FREE") % blockedTiles % freeTiles;
	}

	#define MAKE_COOL_UNDERGROUND_TUNNELS false
	if (pos.z && MAKE_COOL_UNDERGROUND_TUNNELS) //underground
	{
		std::vector<int3> rockTiles;

		for (auto tile : tileinfo)
		{	
			if (gen->shouldBeBlocked(tile))
			{
				bool placeRock = true;
				gen->foreach_neighbour (tile, [gen, &placeRock](int3 &pos)
				{
					if (!(gen->shouldBeBlocked(pos) || gen->isPossible(pos)))
						placeRock = false;
				});
				if (placeRock)
				{
					rockTiles.push_back(tile);
				}
			}
		}
		gen->editManager->getTerrainSelection().setSelection(rockTiles);
		gen->editManager->drawTerrain(ETerrainType::ROCK, &gen->rand);
		for (auto tile : rockTiles)
		{
			gen->setOccupied (tile, ETileType::USED); //don't place obstacles in a rock
			//gen->foreach_neighbour (tile, [gen](int3 &pos)
			//{
			//	if (!gen->isUsed(pos))
			//		gen->setOccupied (pos, ETileType::BLOCKED);
			//});
		}
	}
	typedef std::vector<ObjectTemplate> obstacleVector;
	//obstacleVector possibleObstacles;

	std::map <ui8, obstacleVector> obstaclesBySize;
	typedef std::pair <ui8, obstacleVector> obstaclePair;
	std::vector<obstaclePair> possibleObstacles;

	//get all possible obstacles for this terrain
	for (auto primaryID : VLC->objtypeh->knownObjects()) 
	{ 
		for (auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID)) 
		{ 
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID); 
			if (handler->isStaticObject())
			{
				for (auto temp : handler->getTemplates())
				{
					if (temp.canBePlacedAt(terrainType) && temp.getBlockMapOffset().valid())
						obstaclesBySize[temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		} 
	}
	for (auto o : obstaclesBySize)
	{
		possibleObstacles.push_back (std::make_pair(o.first, o.second));
	}
	boost::sort (possibleObstacles, [](const obstaclePair &p1, const obstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});

	auto sel = gen->editManager->getTerrainSelection();
	sel.clearSelection();

	auto tryToPlaceObstacleHere = [this, gen, &possibleObstacles](int3& tile, int index)-> bool
	{
		auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles[index].second, gen->rand);
		int3 obstaclePos = tile - temp.getBlockMapOffset();
		if (canObstacleBePlacedHere(gen, temp, obstaclePos)) //can be placed here
		{
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			placeObject(gen, obj, obstaclePos, false);
			return true;
		}
		return false;
	};

	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	for (auto tile : boost::adaptors::reverse(tileinfo))
	{
		//fill tiles that should be blocked with obstacles or are just possible (with some probability)
		if (gen->shouldBeBlocked(tile) || (gen->isPossible(tile) && gen->rand.nextInt(1,100) < 60))
		{
			//start from biggets obstacles
			for (int i = 0; i < possibleObstacles.size(); i++)
			{
				if (tryToPlaceObstacleHere(tile, i))
					break;
			}
		}
	}
}

bool CRmgTemplateZone::fill(CMapGenerator* gen)
{
	initTerrainType(gen);

	freePaths.insert(pos); //zone center should be always clear to allow other tiles to connect

	addAllPossibleObjects (gen);

	placeMines(gen);
	createRequiredObjects(gen);
	fractalize(gen); //after required objects are created and linked with their own paths
	createTreasures(gen);
	createObstacles(gen);

	logGlobal->infoStream() << boost::format ("Zone %d filled successfully") %id;
	return true;
}

bool CRmgTemplateZone::findPlaceForTreasurePile(CMapGenerator* gen, float min_dist, int3 &pos)
{
	//si32 min_dist = sqrt(tileinfo.size()/density);
	float best_distance = 0;
	bool result = false;

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;
	for(auto tile : possibleTiles)
	{
		auto dist = gen->getNearestObjectDistance(tile);

		if ((dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			gen->foreach_neighbour (tile, [&gen, &allTilesAvailable](int3 neighbour)
			{
				if (!(gen->isPossible(neighbour) || gen->shouldBeBlocked(neighbour)))
				{
					allTilesAvailable = false; //all present tiles must be already blocked or ready for new objects
				}
			});
			if (allTilesAvailable)
			{
				best_distance = dist;
				pos = tile;
				result = true;
			}
		}
	}
	if (result)
	{
		gen->setOccupied(pos, ETileType::BLOCKED); //block that tile
	}
	return result;
}

bool CRmgTemplateZone::canObstacleBePlacedHere(CMapGenerator* gen, ObjectTemplate &temp, int3 &pos)
{
	if (!gen->map->isInTheMap(pos)) //blockmap may fit in the map, but botom-right corner does not
		return false;

	auto tilesBlockedByObject = temp.getBlockedOffsets();

	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = pos + blockingTile;
		if (!gen->map->isInTheMap(t) || !(gen->isPossible(t) || gen->shouldBeBlocked(t)))
		{
			return false; //if at least one tile is not possible, object can't be placed here
		}
	}
	return true;
}

bool CRmgTemplateZone::isAccessibleFromAnywhere (CMapGenerator* gen, ObjectTemplate &appearance,  int3 &tile, const std::set<int3> &tilesBlockedByObject) const
{
	bool accessible = false;
	for (int x = -1; x < 2; x++)
	{
		for (int y = -1; y <2; y++)
		{
			if (x && y) //check only if object is visitable from another tile
			{
				int3 offset = appearance.getVisitableOffset() + int3(x, y, 0);
				if (!vstd::contains(tilesBlockedByObject, offset))
				{
					int3 nearbyPos = tile + offset;
					if (gen->map->isInTheMap(nearbyPos))
					{
						if (appearance.isVisitableFrom(x, y) && !gen->isBlocked(nearbyPos))
							accessible = true;
					}
				}
			}
		};
	}
	return accessible;
}

bool CRmgTemplateZone::findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tiles
	if (obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID)->getTemplates(gen->map->getTile(getPos()).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %obj->ID %obj->subID %pos));
	
		obj->appearance = templates.front();
	}

	//si32 min_dist = sqrt(tileinfo.size()/density);
	int best_distance = 0;
	bool result = false;
	//si32 w = gen->map->width;
	//si32 h = gen->map->height;

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;

	auto tilesBlockedByObject = obj->getBlockedOffsets();

	for (auto tile : tileinfo)
	{
		//object must be accessible from at least one surounding tile
		if (!isAccessibleFromAnywhere(gen, obj->appearance, tile, tilesBlockedByObject))
			continue;

		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			for (auto blockingTile : tilesBlockedByObject)
			{
				int3 t = tile + blockingTile;
				if (!gen->map->isInTheMap(t) || !gen->isPossible(t))
				{
					allTilesAvailable = false; //if at least one tile is not possible, object can't be placed here
					break;
				}
			}
			if (allTilesAvailable)
			{
				best_distance = dist;
				pos = tile;
				result = true;
			}
		}
	}
	if (result)
	{
		gen->setOccupied(pos, ETileType::BLOCKED); //block that tile
	}
	return result;
}

void CRmgTemplateZone::checkAndPlaceObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos)
{
	if (!gen->map->isInTheMap(pos))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % object->id % pos));
	object->pos = pos;

	if (object->isVisitable() && !gen->map->isInTheMap(object->visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % object->visitablePos() % object->id % object->pos()));
	for (auto tile : object->getBlockedPos())
	{
		if (!gen->map->isInTheMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile() % object->id % object->pos()));
	}

	if (object->appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = gen->map->getTile(pos).terType;
		auto templates = VLC->objtypeh->getHandlerFor(object->ID, object->subID)->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") %object->ID %object->subID %pos %terrainType));
	
		object->appearance = templates.front();
	}

	gen->editManager->insertObject(object, pos);
	//logGlobal->traceStream() << boost::format ("Successfully inserted object (%d,%d) at pos %s") %object->ID %object->subID %pos();
}

void CRmgTemplateZone::placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos, bool updateDistance)
{
	//logGlobal->traceStream() << boost::format("Inserting object at %d %d") % pos.x % pos.y;

	checkAndPlaceObject (gen, object, pos);

	auto points = object->getBlockedPos();
	if (object->isVisitable())
		points.insert(pos + object->getVisitableOffset());
	points.insert(pos);
	for(auto p : points)
	{		
		if (gen->map->isInTheMap(p))
		{
			gen->setOccupied(p, ETileType::USED);
		}
	}
	if (updateDistance)
	{
		for(auto tile : possibleTiles) //don't need to mark distance for not possible tiles
		{		
			si32 d = pos.dist2dSQ(tile); //optimization, only relative distance is interesting
			gen->setNearestObjectDistance(tile, std::min<float>(d, gen->getNearestObjectDistance(tile)));
		}
	}
}

void CRmgTemplateZone::placeAndGuardObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos, si32 str, bool zoneGuard)
{
	placeObject(gen, object, pos);
	guardObject(gen, object, str, zoneGuard);
}

std::vector<int3> CRmgTemplateZone::getAccessibleOffsets (CMapGenerator* gen, CGObjectInstance* object)
{
	//get all tiles from which this object can be accessed
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;

	auto tilesBlockedByObject = object->getBlockedPos(); //absolue value, as object is already placed

	gen->foreach_neighbour(visitable, [&](int3& pos) 
	{
		if (gen->isPossible(pos))
		{
			if (!vstd::contains(tilesBlockedByObject, pos))
			{
				if (object->appearance.isVisitableFrom(pos.x - visitable.x, pos.y - visitable.y) && !gen->isBlocked(pos)) //TODO: refactor - info about visitability from absolute coordinates
				{
					tiles.push_back(pos);
				}
			}

		};
	});

	return tiles;
}

bool CRmgTemplateZone::guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str, bool zoneGuard, bool addToFreePaths)
{
	logGlobal->traceStream() << boost::format("Guard object at %s") % object->pos();

	std::vector<int3> tiles = getAccessibleOffsets (gen, object);

	int3 guardTile(-1,-1,-1);

	for (auto tile : tiles)
	{
		//crunching path may fail if center of the zone is directly over wide object
		//make sure object is accessible before surrounding it with blocked tiles
		if (crunchPath (gen, tile, findClosestTile(freePaths, tile), id, addToFreePaths ? &freePaths : nullptr))
		{
			guardTile = tile;
			break;
		}
	}
	if (!guardTile.valid())
	{
		logGlobal->errorStream() << boost::format("Failed to crunch path to object at %s") % object->pos();
		return false;
	}

	if (addMonster (gen, guardTile, str, false, zoneGuard)) //do not place obstacles around unguarded object
	{
		for (auto pos : tiles)
		{
			if (!gen->isFree(pos))
				gen->setOccupied(pos, ETileType::BLOCKED);
		}
		gen->foreach_neighbour (guardTile, [&](int3& pos) 
		{
			if (gen->isPossible(pos))
				gen->setOccupied (pos, ETileType::FREE);
		});

		gen->setOccupied (guardTile, ETileType::USED);
	}
	else //allow no guard or other object in front of this object
	{
		for (auto tile : tiles)
			if (gen->isPossible(tile))
				gen->setOccupied (tile, ETileType::FREE);
	}

	return true;
}

ObjectInfo CRmgTemplateZone::getRandomObject(CMapGenerator* gen, CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue)
{
	//int objectsVisitableFromBottom = 0; //for debug

	std::vector<std::pair<ui32, ObjectInfo>> tresholds;
	ui32 total = 0;

	//calculate actual treasure value range based on remaining value
	ui32 maxVal = maxValue - currentValue;
	ui32 minValue = 0.25f * (desiredValue - currentValue);

	//roulette wheel
	for (ObjectInfo &oi : possibleObjects) //copy constructor turned out to be costly
	{
		if (oi.value >= minValue && oi.value <= maxVal && oi.maxPerZone > 0)
		{
			int3 newVisitableOffset = oi.templ.getVisitableOffset(); //visitablePos assumes object will be shifter by visitableOffset
			int3 newVisitablePos = info.nextTreasurePos;

			if (!oi.templ.isVisitableFromTop())
			{
				//objectsVisitableFromBottom++;
				//there must be free tiles under object
				auto blockedOffsets = oi.templ.getBlockedOffsets();
				if (!isAccessibleFromAnywhere(gen, oi.templ, newVisitablePos, blockedOffsets))
					continue;
			}

			//NOTE: y coordinate grows downwards
			if (info.visitableFromBottomPositions.size() + info.visitableFromTopPositions.size()) //do not try to match first object in zone
			{
				bool fitsHere = false;

				if (oi.templ.isVisitableFromTop()) //new can be accessed from any direction
				{
					for (auto tile : info.visitableFromTopPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if (newVisitablePos.areNeighbours(actualTile)) //we access other removable object from any position
						{
							fitsHere = true;
							break;
						}
					}
					for (auto tile : info.visitableFromBottomPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if (newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y >= actualTile.y) //we access existing static object from side or bottom only
						{
							fitsHere = true;
							break;
						}
					}
				}
				else //if new object is not visitable from top, it must be accessible from below or side
				{
					for (auto tile : info.visitableFromTopPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if (newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y <= actualTile.y) //we access existing removable object from top or side only
						{
							fitsHere = true;
							break;
						}
					}
					for (auto tile : info.visitableFromBottomPositions)
					{
						int3 actualTile = tile + newVisitableOffset;
						if (newVisitablePos.areNeighbours(actualTile) && newVisitablePos.y == actualTile.y) //we access other static object from side only
						{
							fitsHere = true;
							break;
						}
					}
				}
				if (!fitsHere)
					continue;
			}

			//now check blockmap, including our already reserved pile area

			bool fitsBlockmap = true;


			std::set<int3> blockedOffsets = oi.templ.getBlockedOffsets();
			blockedOffsets.insert (newVisitableOffset);
			for (auto blockingTile : blockedOffsets)
			{
				int3 t = info.nextTreasurePos + newVisitableOffset + blockingTile;
				if (!gen->map->isInTheMap(t) || vstd::contains(info.occupiedPositions, t))
				{
					fitsBlockmap = false; //if at least one tile is not possible, object can't be placed here
					break;
				}
				if (!(gen->isPossible(t) || gen->isBlocked(t))) //blocked tiles of object may cover blocked tiles, but not used or free tiles
				{
					fitsBlockmap = false;
					break;
				}
			}
			if (!fitsBlockmap)
				continue;

			total += oi.probability;
			//assert (oi.value > 0);
			tresholds.push_back (std::make_pair (total, oi));
		}
	}

	//logGlobal->infoStream() << boost::format ("Number of objects visitable  from bottom: %d") % objectsVisitableFromBottom;

	if (tresholds.empty())
	{
		ObjectInfo oi;
		//Generate pandora Box with gold if the value is extremely high
		if (minValue > 20000) //we don't have object valuable enough
		{
			oi.generateObject = [minValue]() -> CGObjectInstance *
			{
				auto obj = new CGPandoraBox();
				obj->ID = Obj::PANDORAS_BOX;
				obj->subID = 0;
				obj->resources[Res::GOLD] = minValue;
				return obj;
			};
			oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
			oi.value = minValue;
			oi.probability = 0;
		}
		else //generate empty object with 0 value if the value if we can't spawn anything
		{
			oi.generateObject = [gen]() -> CGObjectInstance *
			{
				return nullptr;
			};
			oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType); //TODO: null template or something? should be never used, but hell knows
			oi.value = 0; // this field is checked to determine no object
			oi.probability = 0;
		}
		return oi;
	}
	else
	{
		int r = gen->rand.nextInt (1, total);

		for (auto t : tresholds)
		{
			if (r <= t.first)
				return t.second;
		}
		assert (0); //we should never be here
	}
	//FIXME: control reaches end of non-void function. Missing return?
}

void CRmgTemplateZone::addAllPossibleObjects (CMapGenerator* gen)
{
	ObjectInfo oi;
	oi.maxPerMap = std::numeric_limits<ui32>().max();

	int numZones = gen->getZones().size();

	for (auto primaryID : VLC->objtypeh->knownObjects()) 
	{ 
		for (auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID)) 
		{ 
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID); 
			if (!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				for (auto temp : handler->getTemplates())
				{
					if (temp.canBePlacedAt(terrainType))
					{
						oi.generateObject = [gen, temp]() -> CGObjectInstance *
						{
							return VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
						};
						auto rmgInfo = handler->getRMGInfo();
						oi.value = rmgInfo.value;
						oi.probability = rmgInfo.rarity;
						oi.templ = temp;
						oi.maxPerZone = rmgInfo.zoneLimit;
						vstd::amin (oi.maxPerZone, rmgInfo.mapLimit / numZones); //simple, but should distribute objects evenly on large maps
						possibleObjects.push_back (oi);
					}
				}
			}
		} 
	}

	//prisons
	//levels 1, 5, 10, 20, 30
    static int prisonExp[] = {0, 5000, 15000, 90000, 500000};
	static int prisonValues[] = {2500, 5000, 10000, 20000, 30000};

	for (int i = 0; i < 5; i++)
	{
		oi.generateObject = [i, gen, this]() -> CGObjectInstance *
		{
			auto obj = new CGHeroInstance;
			obj->ID = Obj::PRISON;

			std::vector<ui32> possibleHeroes;
			for (int j = 0; j < gen->map->allowedHeroes.size(); j++)
			{
				if (gen->map->allowedHeroes[j])
					possibleHeroes.push_back(j);
			}

			auto hid = *RandomGeneratorUtil::nextItem(possibleHeroes, gen->rand);
			obj->subID = hid; //will be initialized later
			obj->exp = prisonExp[i]; //game crashes at hero level up
			//obj->exp = 0;
			obj->setOwner(PlayerColor::NEUTRAL);
			gen->map->allowedHeroes[hid] = false; //ban this hero
			gen->decreasePrisonsRemaining();
			obj->appearance = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(terrainType).front(); //can't init template with hero subID

			return obj;
		};
		oi.setTemplate (Obj::PRISON, 0, terrainType);
		oi.value = prisonValues[i];
		oi.probability = 30;
		oi.maxPerZone = gen->getPrisonsRemaning() / 5; //probably not perfect, but we can't generate more prisons than hereos.
		possibleObjects.push_back (oi);
	}

	//all following objects are unlimited
	oi.maxPerZone = std::numeric_limits<ui32>().max();

	//dwellings

	auto subObjects = VLC->objtypeh->knownSubObjects(Obj::CREATURE_GENERATOR1);

	//don't spawn original "neutral" dwellings that got replaced by Conflux dwellings in AB
	static int elementalConfluxROE[] = {7, 13, 16, 47};
	for (int i = 0; i < 4; i++)
		vstd::erase_if_present(subObjects, elementalConfluxROE[i]);

	for (auto secondaryID : subObjects)
	{
		auto dwellingHandler = dynamic_cast<const CDwellingInstanceConstructor*>(VLC->objtypeh->getHandlerFor(Obj::CREATURE_GENERATOR1, secondaryID).get());
		auto creatures = dwellingHandler->getProducedCreatures();
		if (creatures.empty())
			continue;

		auto cre = creatures.front();
		if (cre->faction == townType)
		{
			float nativeZonesCount = gen->getZoneCount(cre->faction);
			oi.value = cre->AIValue * cre->growth * (1 + (nativeZonesCount / gen->getTotalZoneCount()) + (nativeZonesCount / 2));
			oi.probability = 40;

			for (auto temp : dwellingHandler->getTemplates())
			{
				if (temp.canBePlacedAt(terrainType))
				{
					oi.generateObject = [gen, temp, secondaryID, dwellingHandler]() -> CGObjectInstance *
					{
						auto obj = VLC->objtypeh->getHandlerFor(Obj::CREATURE_GENERATOR1, secondaryID)->create(temp);
						//dwellingHandler->configureObject(obj, gen->rand);
						obj->tempOwner = PlayerColor::NEUTRAL;
						return obj;
					};

					oi.templ = temp;
					possibleObjects.push_back (oi);
				}
			}
		}
	}

	static const int scrollValues[] = {500, 2000, 3000, 4000, 5000};

	for (int i = 0; i < 5; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::SPELL_SCROLL;
			obj->subID = 0;
			std::vector<SpellID> out;

			//TODO: unify with cb->getAllowedSpells?
			for (ui32 i = 0; i < gen->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
			{
				const CSpell *spell = SpellID(i).toSpell();
				if (gen->map->allowedSpell[spell->id] && spell->level == i+1)
				{
					out.push_back(spell->id);
				}
			}
			auto a = CArtifactInstance::createScroll(RandomGeneratorUtil::nextItem(out, gen->rand)->toSpell());
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			return obj;
		};
		oi.setTemplate (Obj::SPELL_SCROLL, 0, terrainType);
		oi.value = scrollValues[i];
		oi.probability = 30;
		possibleObjects.push_back (oi);
	}

	//pandora box with gold
	for (int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;
			obj->resources[Res::GOLD] = i * 5000;
			return obj;
		};
		oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 5000;;
		oi.probability = 5;
		possibleObjects.push_back (oi);
	}

	//pandora box with experience
	for (int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;
			obj->gainedExp = i * 5000;
			return obj;
		};
		oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 6000;;
		oi.probability = 20;
		possibleObjects.push_back (oi);
	}

	//pandora box with creatures
	static const int tierValues[] = {5000, 7000, 9000, 12000, 16000, 21000, 27000};

	for (auto creature : VLC->creh->creatures)
	{
		if (!creature->special && creature->faction == townType)
		{
			int actualTier = creature->level > 7 ? 6 : creature->level-1;
			float creaturesAmount = tierValues[actualTier] / creature->AIValue;
			if (creaturesAmount <= 5)
			{
				creaturesAmount = boost::math::round(creaturesAmount); //allow single monsters
				if (creaturesAmount < 1)
					continue;
			}
			else if (creaturesAmount <= 12)
			{
				(creaturesAmount /= 2) *= 2;
			}
			else if (creaturesAmount <= 50)
			{
				creaturesAmount = boost::math::round(creaturesAmount / 5) * 5;
			}
			else
			{
				creaturesAmount = boost::math::round(creaturesAmount / 10) * 10;
			}

			oi.generateObject = [creature, creaturesAmount]() -> CGObjectInstance *
			{
				auto obj = new CGPandoraBox();
				obj->ID = Obj::PANDORAS_BOX;
				obj->subID = 0;
				auto stack = new CStackInstance(creature, creaturesAmount);
				obj->creatures.putStack(SlotID(0), stack);
				return obj;
			};
			oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
			oi.value = (2 * (creature->AIValue) * creaturesAmount * (1 + (float)(gen->getZoneCount(creature->faction)) / gen->getTotalZoneCount()))/3; //TODO: count number of towns on the map
			oi.probability = 3;
			possibleObjects.push_back (oi);
		}
	}

	//Pandora with 12 spells of certain level
	for (int i = 1; i <= GameConstants::SPELL_LEVELS; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;

			std::vector <CSpell *> spells;
			for (auto spell : VLC->spellh->objects)
			{
				if (!spell->isSpecialSpell() && spell->level == i)
					spells.push_back(spell);
			}

			RandomGeneratorUtil::randomShuffle(spells, gen->rand);
			for (int j = 0; j < std::min<int>(12, spells.size()); j++)
			{
				obj->spells.push_back (spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = (i + 1) * 2500; //5000 - 15000
		oi.probability = 2;
		possibleObjects.push_back (oi);
	}

	//Pandora with 15 spells of certain school
	for (int i = 1; i <= 4; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;

			std::vector <CSpell *> spells;
			for (auto spell : VLC->spellh->objects)
			{
				if (!spell->isSpecialSpell())
				{
					bool school = false; //TODO: we could have better interface for iterating schools
					switch (i)
					{
						case 1:
							school = spell->air;
							break;
						case 2:
							school = spell->earth;
							break;
						case 3:
							school = spell->fire;
							break;
						case 4:
							school = spell->water;
							break;
					}
					if (school)
						spells.push_back(spell);
				}
			}

			RandomGeneratorUtil::randomShuffle(spells, gen->rand);
			for (int j = 0; j < std::min<int>(15, spells.size()); j++)
			{
				obj->spells.push_back (spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = 15000;
		oi.probability = 2;
		possibleObjects.push_back (oi);
	}

	// Pandora box with 60 random spells

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGPandoraBox();
		obj->ID = Obj::PANDORAS_BOX;
		obj->subID = 0;

		std::vector <CSpell *> spells;
		for (auto spell : VLC->spellh->objects)
		{
			if (!spell->isSpecialSpell())
				spells.push_back(spell);
		}

		RandomGeneratorUtil::randomShuffle(spells, gen->rand);
		for (int j = 0; j < std::min<int>(60, spells.size()); j++)
		{
			obj->spells.push_back (spells[j]->id);
		}

		return obj;
	};
	oi.setTemplate (Obj::PANDORAS_BOX, 0, terrainType);
	oi.value = 3000;
	oi.probability = 2;
	possibleObjects.push_back (oi);
}

void ObjectInfo::setTemplate (si32 type, si32 subtype, ETerrainType terrainType)
{
	templ = VLC->objtypeh->getHandlerFor(type, subtype)->getTemplates(terrainType).front();
}
