
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
#include "../spells/CSpellHandler.h" //for choosing random spells

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

void CRmgTemplateZone::addRoadNode(const int3& node)
{
	roadNodes.insert(node);
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

CTileInfo::CTileInfo():nearestObjectDistance(INT_MAX), terrain(ETerrainType::WRONG),roadType(ERoadType::NO_ROAD) 
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

bool CTileInfo::isRoad() const
{
	return roadType != ERoadType::NO_ROAD;
}

bool CTileInfo::isUsed() const
{
	return occupied == ETileType::USED;
}
void CTileInfo::setOccupied(ETileType::ETileType value)
{
	occupied = value;
}

ETileType::ETileType CTileInfo::getTileType() const
{
	return occupied;
}

ETerrainType CTileInfo::getTerrainType() const
{
	return terrain;
}

void CTileInfo::setTerrainType(ETerrainType value)
{
	terrain = value;
}

void CTileInfo::setRoadType(ERoadType::ERoadType value)
{
	roadType = value;
//	setOccupied(ETileType::FREE);
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
	minGuardedValue(0),
	questArtZone(nullptr)
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

void CRmgTemplateZone::setQuestArtZone(CRmgTemplateZone * otherZone)
{
	questArtZone = otherZone;
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

void CRmgTemplateZone::clearTiles()
{
	tileinfo.clear();
}

void CRmgTemplateZone::initFreeTiles (CMapGenerator* gen)
{
	vstd::copy_if (tileinfo, vstd::set_inserter(possibleTiles), [gen](const int3 &tile) -> bool
	{
		return gen->isPossible(tile);
	});
	if (freePaths.empty())
		freePaths.insert(pos); //zone must have at least one free tile where other paths go - for instance in the center
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
	std::set<int3> tilesToIgnore; //will be erased in this iteration

	//the more treasure density, the greater distance between paths. Scaling is experimental.
	int totalDensity = 0;
	for (auto ti : treasureInfo)
		totalDensity += ti.density;
	const float minDistance = 10 * 10; //squared

	for (auto tile : tileinfo)
	{
		if (gen->isFree(tile))
			clearedTiles.push_back(tile);
		else if (gen->isPossible(tile))
			possibleTiles.insert(tile);
	}
	assert (clearedTiles.size()); //this should come from zone connections

	std::vector<int3> nodes; //connect them with a grid

	if (type != ETemplateZoneType::JUNCTION)
	{
		//junction is not fractalized, has only one straight path
		//everything else remains blocked
		while (possibleTiles.size())
		{
			//link tiles in random order
			std::vector<int3> tilesToMakePath(possibleTiles.begin(), possibleTiles.end());
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, gen->rand);

			int3 nodeFound(-1, -1, -1);

			for (auto tileToMakePath : tilesToMakePath)
			{
				//find closest free tile
				float currentDistance = 1e10;
				int3 closestTile(-1, -1, -1);

				for (auto clearTile : clearedTiles)
				{
					float distance = tileToMakePath.dist2dSQ(clearTile);

					if (distance < currentDistance)
					{
						currentDistance = distance;
						closestTile = clearTile;
					}
					if (currentDistance <= minDistance)
					{
						//this tile is close enough. Forget about it and check next one
						tilesToIgnore.insert(tileToMakePath);
						break;
					}
				}
				//if tiles is not close enough, make path to it
				if (currentDistance > minDistance)
				{
					nodeFound = tileToMakePath;
					nodes.push_back(nodeFound);
					clearedTiles.push_back(nodeFound); //from now on nearby tiles will be considered handled
					break; //next iteration - use already cleared tiles
				}
			}

			for (auto tileToClear : tilesToIgnore)
			{
				//these tiles are already connected, ignore them
				vstd::erase_if_present(possibleTiles, tileToClear);
			}
			if (!nodeFound.valid()) //nothing else can be done (?)
				break;
			tilesToIgnore.clear();
		}
	}

	for (auto node : nodes)
	{
		boost::sort(nodes, [&node](const int3& ourNode, const int3& otherNode) -> bool
		{
			return node.dist2dSQ(ourNode) < node.dist2dSQ(otherNode);
		}
		);

		std::vector <int3> nearbyNodes;
		if (nodes.size() >= 2)
		{
			nearbyNodes.push_back(nodes[1]); //node[0] is our node we want to connect
		}
		if (nodes.size() >= 3)
		{
			nearbyNodes.push_back(nodes[2]);
		}

		//connect with all the paths
		crunchPath(gen, node, findClosestTile(freePaths, node), true, &freePaths);
		//connect with nearby nodes
		for (auto nearbyNode : nearbyNodes)
		{
			crunchPath(gen, node, nearbyNode, true, &freePaths);
		}
	}
	for (auto node : nodes)
		gen->setOccupied(node, ETileType::FREE); //make sure they are clear

	//now block most distant tiles away from passages

	float blockDistance = minDistance * 0.25f;

	for (auto tile : tileinfo)
	{
		if (!gen->isPossible(tile))
			continue;

		bool closeTileFound = false;

		for (auto clearTile : freePaths)
		{
			float distance = tile.dist2dSQ(clearTile);

			if (distance < blockDistance)
			{
				closeTileFound = true;
				break;
			}
		}
		if (!closeTileFound) //this tile is far enough from passages
			gen->setOccupied(tile, ETileType::BLOCKED);
	}

	#define PRINT_FRACTALIZED_MAP false
	if (PRINT_FRACTALIZED_MAP) //enable to debug
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
					char t = '?';
					switch (gen->getTile(int3(i, j, k)).getTileType())
					{
						case ETileType::FREE:
							t = ' '; break;
						case ETileType::BLOCKED:
							t = '#'; break;
						case ETileType::POSSIBLE:
							t = '-'; break;
						case ETileType::USED:
							t = 'O'; break;
					}

					out << t;
				}
				out << std::endl;
			}
			out << std::endl;
		}
		out << std::endl;
	}

	//logGlobal->infoStream() << boost::format ("Zone %d subdivided fractally") %id;
}

void CRmgTemplateZone::connectLater(CMapGenerator* gen)
{
	for (const int3 node : tilesToConnectLater)
	{
		if (!connectWithCenter(gen, node, true))
			logGlobal->errorStream() << boost::format("Failed to connect node %s with center of the zone") % node;
	}
}

bool CRmgTemplateZone::crunchPath(CMapGenerator* gen, const int3 &src, const int3 &dst, bool onlyStraight, std::set<int3>* clearedTiles)
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
			
		auto processNeighbours = [this, gen, &currentPos, dst, &distance, &result, &end, clearedTiles](int3 &pos)
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
		};
		
		if (onlyStraight)
			gen->foreachDirectNeighbour (currentPos, processNeighbours);
		else
			gen->foreach_neighbour (currentPos,processNeighbours);
						
		int3 anotherPos(-1, -1, -1);

		if (!(result || distance < lastDistance)) //we do not advance, use more advanced pathfinding algorithm?
		{
			//try any nearby tiles, even if its not closer than current
			float lastDistance = 2 * distance; //start with significantly larger value
			
			auto processNeighbours2 = [this, gen, &currentPos, dst, &lastDistance, &anotherPos, &end, clearedTiles](int3 &pos)
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
			};			
			if (onlyStraight)
				gen->foreachDirectNeighbour(currentPos, processNeighbours2);
			else
				gen->foreach_neighbour(currentPos, processNeighbours2);
						
			
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
			//FIXME: seemingly this condition is messed up, tells nothing
			//logGlobal->warnStream() << boost::format("No tile closer than %s found on path from %s to %s") % currentPos %src %dst;
			break;
		}
	}

	return result;
}

bool CRmgTemplateZone::createRoad(CMapGenerator* gen, const int3& src, const int3& dst)
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	std::set<int3> open{src};    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	//int3 currentNode = src;
	gen->setRoad (src, ERoadType::NO_ROAD); //just in case zone guard already has road under it. Road under nodes will be added at very end

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	// Cost from start along best known path.
	// Estimated total cost from start to goal through y.

	while (open.size())
	{
		int3 currentNode = *boost::min_element(open, [&distances](const int3 &pos1, const int3 &pos2) -> bool
		{
			return distances[pos1] < distances[pos2];
		});

		vstd::erase_if_present (open, currentNode);
		closed.insert (currentNode);

		if (currentNode == dst || gen->isRoad(currentNode))
		{
			// The goal node was reached. Trace the path using
			// the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				// add node to path
				roads.insert (backTracking);
				gen->setRoad (backTracking, ERoadType::COBBLESTONE_ROAD);
				//logGlobal->traceStream() << boost::format("Setting road at tile %s") % backTracking;
				// do the same for the predecessor
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			bool directNeighbourFound = false;
			float movementCost = 1;

			auto foo = [gen, this, &open, &closed, &cameFrom, &currentNode, &distances, &dst, &directNeighbourFound, movementCost](int3& pos) -> void
			{
				float distance = distances[currentNode] + movementCost;
				int bestDistanceSoFar = 1e6; //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (distance < bestDistanceSoFar || !vstd::contains(closed, pos))
				{
					auto obj = gen->map->getTile(pos).topVisitableObj();
					//if (gen->map->checkForVisitableDir(currentNode, &gen->map->getTile(pos), pos)) //TODO: why it has no effect?
					if (gen->isFree(pos) || pos == dst || (obj && obj->ID == Obj::MONSTER))
					{
						if (vstd::contains(this->tileinfo, pos) || pos == dst) //otherwise guard position may appear already connected to other zone.
						{
							cameFrom[pos] = currentNode;
							open.insert(pos);
							distances[pos] = distance;
							directNeighbourFound = true;
							//logGlobal->traceStream() << boost::format("Found connection between node %s and %s, current distance %d") % currentNode % pos % distance;
						}
					}
				}
			};

			gen->foreachDirectNeighbour (currentNode, foo); // roads cannot be rendered correctly for diagonal directions
			if (!directNeighbourFound)
			{
				movementCost = 2.1f; //moving diagonally is penalized over moving two tiles straight
				gen->foreach_neighbour(currentNode, foo);
			}
		}

	}
	logGlobal->warnStream() << boost::format("Failed to create road from %s to %s") % src %dst;
	return false;

}

bool CRmgTemplateZone::connectPath(CMapGenerator* gen, const int3& src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	std::set<int3> open{ src };    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	//int3 currentNode = src;

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	// Cost from start along best known path.
	// Estimated total cost from start to goal through y.

	while (open.size())
	{
		int3 currentNode = *boost::min_element(open, [&distances](const int3 &pos1, const int3 &pos2) -> bool
		{
			return distances[pos1] < distances[pos2];
		});

		vstd::erase_if_present(open, currentNode);
		closed.insert(currentNode);

		if (gen->isFree(currentNode)) //we reached free paths, stop
		{
			// Trace the path using the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				gen->setOccupied(backTracking, ETileType::FREE);
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			auto foo = [gen, this, &open, &closed, &cameFrom, &currentNode, &distances](int3& pos) -> void
			{
				int distance = distances[currentNode] + 1;
				int bestDistanceSoFar = 1e6; //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (gen->isBlocked(pos)) //no paths through blocked or occupied tiles
					return;
				if (distance < bestDistanceSoFar || !vstd::contains(closed, pos))
				{
					//auto obj = gen->map->getTile(pos).topVisitableObj();
					if (vstd::contains(this->tileinfo, pos))
					{
						cameFrom[pos] = currentNode;
						open.insert(pos);
						distances[pos] = distance;
					}
				}
			};

			if (onlyStraight)
				gen->foreachDirectNeighbour(currentNode, foo);
			else 
				gen->foreach_neighbour(currentNode, foo);
		}

	}
	for (auto tile : closed) //these tiles are sealed off and can't be connected anymore
	{
		//TODO: refactor, unify?
		gen->setOccupied (tile, ETileType::BLOCKED);
		vstd::erase_if_present(possibleTiles, tile);
	}
	return false;
}

bool CRmgTemplateZone::connectWithCenter(CMapGenerator* gen, const int3& src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	std::set<int3> open{ src };    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	//int3 currentNode = src;

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	// Cost from start along best known path.
	// Estimated total cost from start to goal through y.

	while (open.size())
	{
		int3 currentNode = *boost::min_element(open, [&distances](const int3 &pos1, const int3 &pos2) -> bool
		{
			return distances[pos1] < distances[pos2];
		});

		vstd::erase_if_present(open, currentNode);
		closed.insert(currentNode);

		if (currentNode == pos) //we reached center of the zone, stop
		{
			// Trace the path using the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				gen->setOccupied(backTracking, ETileType::FREE);
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			auto foo = [gen, this, &open, &closed, &cameFrom, &currentNode, &distances](int3& pos) -> void
			{
				float movementCost = 0;
				if (gen->isFree(pos))
					movementCost = 1;
				else if (gen->isPossible(pos))
					movementCost = 2;
				else
					return;

				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = 1e6; //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (distance < bestDistanceSoFar || !vstd::contains(closed, pos))
				{
					//auto obj = gen->map->getTile(pos).topVisitableObj();
					if (vstd::contains(this->tileinfo, pos))
					{
						cameFrom[pos] = currentNode;
						open.insert(pos);
						distances[pos] = distance;
					}
				}
			};

			if (onlyStraight)
				gen->foreachDirectNeighbour(currentNode, foo);
			else
				gen->foreach_neighbour(currentNode, foo);
		}

	}
	return false;
}


void CRmgTemplateZone::addRequiredObject(CGObjectInstance * obj, si32 strength)
{
	requiredObjects.push_back(std::make_pair(obj, strength));
}
void CRmgTemplateZone::addCloseObject(CGObjectInstance * obj, si32 strength)
{
	closeObjects.push_back(std::make_pair(obj, strength));
}

void CRmgTemplateZone::addToConnectLater(const int3& src)
{
	tilesToConnectLater.insert(src);
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
		if ((cre->AIValue * (cre->ammMin + cre->ammMax) / 2 < strength) && (strength < cre->AIValue * 100)) //at least one full monster. size between average size of given stack and 100
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
	guard->character = CGCreature::HOSTILE;
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

bool CRmgTemplateZone::createTreasurePile(CMapGenerator* gen, int3 &pos, float minDistance, const CTreasureInfo& treasureInfo)
{
	CTreasurePileInfo info;

	std::map<int3, CGObjectInstance *> treasures;
	std::set<int3> boundary;
	int3 guardPos (-1,-1,-1);
	info.nextTreasurePos = pos;

	int maxValue = treasureInfo.max;
	int minValue = treasureInfo.min;

	ui32 desiredValue = (gen->rand.nextInt(minValue, maxValue));

	int currentValue = 0;
	CGObjectInstance * object = nullptr;
	while (currentValue <= desiredValue - 100) //no objects with value below 100 are avaiable
	{
		treasures[info.nextTreasurePos] = nullptr;

		for (auto treasurePos : treasures)
		{
			gen->foreach_neighbour(treasurePos.first, [gen, &boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}
		for (auto treasurePos : treasures)
		{
			//leaving only boundary around objects
			vstd::erase_if_present(boundary, treasurePos.first);
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

			//update treasure pile area
			int3 visitablePos = info.nextTreasurePos;

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
			else
				break; //no more place to add any objects
		}
	}

	if (treasures.size())
	{
		//find object closest to free path, then connect it to the middle of the zone

		int3 closestTile = int3(-1,-1,-1);
		float minDistance = 1e10;

		for (auto visitablePos : info.visitableFromBottomPositions) //objects that are not visitable from top must be accessible from bottom or side
		{
			int3 closestFreeTile = findClosestTile(freePaths, visitablePos);
			if (closestFreeTile.dist2d(visitablePos) < minDistance)
			{
				closestTile = visitablePos + int3 (0, 1, 0); //start below object (y+1), possibly even outside the map, to not make path up through it
				minDistance = closestFreeTile.dist2d(visitablePos);
			}
		}
		for (auto visitablePos : info.visitableFromTopPositions) //all objects are accessible from any direction
		{
			int3 closestFreeTile = findClosestTile(freePaths, visitablePos);
			if (closestFreeTile.dist2d(visitablePos) < minDistance)
			{
				closestTile = visitablePos;
				minDistance = closestFreeTile.dist2d(visitablePos);
			}
		}
		assert (closestTile.valid());

		for (auto tile : info.occupiedPositions)
		{
			if (gen->map->isInTheMap(tile)) //pile boundary may reach map border
				gen->setOccupied(tile, ETileType::BLOCKED); //so that crunch path doesn't cut through objects
		}

		if (!connectPath (gen, closestTile, false)) //this place is sealed off, need to find new position
		{
			return false;
		}

		//update boundary around our objects, including knowledge about objects visitable from bottom
		boundary.clear();

		for (auto tile : info.visitableFromBottomPositions)
		{
			gen->foreach_neighbour(tile, [tile, &boundary](int3 pos)
			{
				if (pos.y >= tile.y) //don't block these objects from above
					boundary.insert(pos);
			});
		}
		for (auto tile : info.visitableFromTopPositions)
		{
			gen->foreach_neighbour(tile, [&boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}

		bool isPileGuarded = currentValue >= minGuardedValue;

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
				if (treasure.second->ID == Obj::SEER_HUT) //FIXME: find generic solution or figure out why Seer Hut doesn't behave correctly
					visitableOffset.x += 1;
				placeObject(gen, treasure.second, treasure.first + visitableOffset);
			}
			if (addMonster(gen, guardPos, currentValue, false))
			{//block only if the object is guarded
				for (auto tile : boundary)
				{
					if (gen->isPossible(tile))
						gen->setOccupied(tile, ETileType::BLOCKED);
				}
				//do not spawn anything near monster
				gen->foreach_neighbour(guardPos, [gen](int3 pos)
				{
					if (gen->isPossible(pos))
						gen->setOccupied(pos, ETileType::FREE);
				});
			}
			else //mo monster in this pile, make some free space (needed?)
			{
				for (auto tile : boundary)
					if (gen->isPossible(tile))
						gen->setOccupied(tile, ETileType::FREE);
			}
		}
		else if (isPileGuarded)//we couldn't make a connection to this location, block it
		{
			for (auto treasure : treasures)
			{
				if (gen->isPossible(treasure.first))
					gen->setOccupied(treasure.first, ETileType::BLOCKED);

				delete treasure.second;
			}
		}

		return true;
	}
	else //we did not place eveyrthing successfully
	{
		gen->setOccupied(pos, ETileType::BLOCKED); //TODO: refactor stop condition
		vstd::erase_if_present(possibleTiles, pos);
		return false;
	}
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
				setPos(town->visitablePos() + int3(0, 1, 0)); //new center of zone that paths connect to
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
		PlayerColor player(player_id);
		if (playerInfo.canAnyonePlay())
		{
			player = PlayerColor(player_id);
			townType = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if (townType == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				randomizeTownType(gen);
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			randomizeTownType(gen);
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
		setPos(town->visitablePos() + int3(0, 1, 0)); //new center of zone that paths connect to

		totalTowns++;
		//register MAIN town of zone only
		gen->registerZone (town->subID);

		if (playerInfo.canAnyonePlay()) //configure info for owning player
		{
			logGlobal->traceStream() << "Fill player info " << player_id;

			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(townType);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos - town->getVisitableOffset();
			playerInfo.generateHeroAtMainTown = true;

			//now create actual towns
			addNewTowns(playerTowns.getCastleCount() - 1, true, player);
			addNewTowns(playerTowns.getTownCount(), false, player);
		}
		else
		{
			addNewTowns(playerTowns.getCastleCount() - 1, true, PlayerColor::NEUTRAL);
			addNewTowns(playerTowns.getTownCount(), false, PlayerColor::NEUTRAL);
		}
	}
	else //randomize town types for any other zones as well
	{
		randomizeTownType(gen);
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
			else //just in any case
				randomizeTownType(gen);
		}
	}
}

void CRmgTemplateZone::randomizeTownType (CMapGenerator* gen)
{
	if (townTypes.size())
		townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
	else
		townType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed, we still need some
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

	for (const auto & res : woodOre)
	{
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = new CGMine();
			mine->ID = Obj::MINE;
			mine->subID = static_cast<si32>(res);
			mine->producedResource = res;
			mine->producedQuantity = mine->defaultResProduction();
			if (!i)
				addCloseObject(mine, 1500); //only firts one is close
			else
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
	
	for(const auto &object : requiredObjects)
	{
		auto obj = object.first;
		int3 pos;
		int3 accessibleOffset;
		while (true)
		{
			if (!findPlaceForObject(gen, obj, 3, pos))
			{
				logGlobal->errorStream() << boost::format("Failed to fill zone %d due to lack of space") % id;
				return false;
			}

			//check if we can find a path around this object. Tiles will be set to "USED" after object is successfully placed.
			obj->pos = pos;
			gen->setOccupied (obj->visitablePos(), ETileType::BLOCKED);
			for (auto tile : obj->getBlockedPos())
			{
				if (gen->map->isInTheMap(tile))
					gen->setOccupied(tile, ETileType::BLOCKED);
			}
			accessibleOffset = getAccessibleOffset(gen, obj->appearance, pos);
			if (!accessibleOffset.valid())
			{
				logGlobal->warnStream() << boost::format("Cannot access required object at position %s, retrying") % pos;
				continue;
			}
			if (!connectPath(gen, accessibleOffset, true))
			{
				logGlobal->warnStream() << boost::format("Failed to create path to required object at position %s, retrying") % pos;
				continue;
			}
			else
				break;
		}

	
		placeObject(gen, obj, pos);
		guardObject (gen, obj, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY), true);
		//paths to required objects constitute main paths of zone. otherwise they just may lead to middle and create dead zones	
	}

	for (const auto &obj : closeObjects)
	{
		std::vector<int3> tiles(possibleTiles.begin(), possibleTiles.end()); //new tiles vector after each object has been placed
		
		// smallest distance to zone center, greatest distance to nearest object
		auto isCloser = [this, gen](const int3 & lhs, const int3 & rhs) -> bool
		{
			float lDist = this->pos.dist2d(lhs);
			float rDist = this->pos.dist2d(rhs);
			lDist *= (lDist > 12) ? 10 : 1; //objects within 12 tile radius are preferred (smaller distance rating)
			rDist *= (rDist > 12) ? 10 : 1;

			return (lDist * 0.5f - std::sqrt(gen->getNearestObjectDistance(lhs))) < (rDist * 0.5f - std::sqrt(gen->getNearestObjectDistance(rhs)));
		};

		boost::sort (tiles, isCloser);

		setTemplateForObject(gen, obj.first);
		auto tilesBlockedByObject = obj.first->getBlockedOffsets();
		bool result = false;

		for (auto tile : tiles)
		{
			//object must be accessible from at least one surounding tile
			if (!isAccessibleFromAnywhere(gen, obj.first->appearance, tile))
				continue;

			//avoid borders
			if (gen->isPossible(tile))
			{
				if (areAllTilesAvailable(gen, obj.first, tile, tilesBlockedByObject))
				{
					placeObject(gen, obj.first, tile);
					guardObject(gen, obj.first, obj.second, (obj.first->ID == Obj::MONOLITH_TWO_WAY), true);
					result = true;
					break;
				}
			}
		}
		if (!result)
		{
			logGlobal->errorStream() << boost::format("Failed to fill zone %d due to lack of space") % id;
			//TODO CLEANUP!
			return false;
		}
	}

	return true;
}

void CRmgTemplateZone::createTreasures(CMapGenerator* gen)
{
	int mapMonsterStrength = gen->mapGenOptions->getMonsterStrength();
	int monsterStrength = zoneMonsterStrength + mapMonsterStrength - 1; //array index from 0 to 4

	static int minGuardedValues[] = { 6500, 4167, 3000, 1833, 1333 };
	minGuardedValue = minGuardedValues[monsterStrength];

	auto valueComparator = [](const CTreasureInfo & lhs, const CTreasureInfo & rhs) -> bool
	{
		return lhs.max > rhs.max;
	};

	//place biggest treasures first at large distance, place smaller ones inbetween
	boost::sort(treasureInfo, valueComparator);

	int totalDensity = 0;
	for (auto t : treasureInfo)
	{
		//discard objects with too high value to be ever placed
		vstd::erase_if(possibleObjects, [t](const ObjectInfo& oi) -> bool
		{
			return oi.value > t.max;
		});

		totalDensity += t.density;

		//treasure density is inversely proportional to zone size but must be scaled back to map size
		//also, normalize it to zone count - higher count means relatively smaller zones

		//this is squared distance for optimization purposes
		const double minDistance = std::max<float>((125.f / totalDensity), 2);
		//distance lower than 2 causes objects to overlap and crash

		bool stop = false;
		do {
			//optimization - don't check tiles which are not allowed
			vstd::erase_if(possibleTiles, [gen](const int3 &tile) -> bool
			{
				return !gen->isPossible(tile);
			});

			
			int3 treasureTilePos;
			//If we are able to place at least one object with value lower than minGuardedValue, it's ok
			do
			{
				if (!findPlaceForTreasurePile(gen, minDistance, treasureTilePos, t.min))
				{
					stop = true;
					break;
				}
			}
			while (!createTreasurePile(gen, treasureTilePos, minDistance, t)); //failed creation - position was wrong, cannot connect it

		} while (!stop);
	}
}

void CRmgTemplateZone::createObstacles1(CMapGenerator * gen)
{
	if (pos.z) //underground
	{
		//now make sure all accessible tiles have no additional rock on them

		std::vector<int3> accessibleTiles;
		for (auto tile : tileinfo)
		{
			if (gen->isFree(tile) || gen->isUsed(tile))
			{
				accessibleTiles.push_back(tile);
			}
		}
		gen->editManager->getTerrainSelection().setSelection(accessibleTiles);
		gen->editManager->drawTerrain(terrainType, &gen->rand);
	}
}

void CRmgTemplateZone::createObstacles2(CMapGenerator* gen)
{

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
		int3 obstaclePos = tile + temp.getBlockMapOffset();
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
	//cleanup - remove unused possible tiles to make space for roads
	for (auto tile : tileinfo)
	{
		if (gen->isPossible(tile))
		{
			gen->setOccupied (tile, ETileType::FREE);
		}
	}
}

void CRmgTemplateZone::connectRoads(CMapGenerator* gen)
{
	logGlobal->debug("Started building roads");
	
	std::set<int3> roadNodesCopy(roadNodes);
	std::set<int3> processed;
	
	while(!roadNodesCopy.empty())
	{
		int3 node = *roadNodesCopy.begin(); 
		roadNodesCopy.erase(node);
		int3 cross(-1, -1, -1);

		auto comparator = [=](int3 lhs, int3 rhs) { return node.dist2dSQ(lhs)  < node.dist2dSQ(rhs); };

		if (processed.size()) //connect with already existing network
		{
			cross = *boost::range::min_element(processed, comparator); //find another remaining node
		}
		else if (roadNodesCopy.size()) //connect with any other unconnected node
		{
			cross = *boost::range::min_element(roadNodesCopy, comparator); //find another remaining node
		}
		else //no other nodes left, for example single road node in this zone
			break;

		logGlobal->debugStream() << "Building road from " << node << " to " << cross;
		if (createRoad(gen, node, cross))
		{
			processed.insert(cross); //don't draw road starting at end point which is already connected
			vstd::erase_if_present(roadNodesCopy, cross);
		}
		
		processed.insert(node); 
	}

	drawRoads(gen);
	
	logGlobal->debug("Finished building roads");	
}

void CRmgTemplateZone::drawRoads(CMapGenerator* gen)
{
	std::vector<int3> tiles;
	for (auto tile : roads)
	{
		if(gen->map->isInTheMap(tile))	
			tiles.push_back (tile);
	}
	for (auto tile : roadNodes)
	{
		if (vstd::contains(tileinfo, tile)) //mark roads for our nodes, but not for zone guards in other zones
			tiles.push_back(tile);
	}

	gen->editManager->getTerrainSelection().setSelection(tiles);	
	gen->editManager->drawRoad(ERoadType::COBBLESTONE_ROAD, &gen->rand);	
}


bool CRmgTemplateZone::fill(CMapGenerator* gen)
{
	initTerrainType(gen);

	//zone center should be always clear to allow other tiles to connect
	gen->setOccupied(this->getPos(), ETileType::FREE);
	freePaths.insert(pos); 

	addAllPossibleObjects (gen);

	connectLater(gen); //ideally this should work after fractalize, but fails
	fractalize(gen);
	placeMines(gen);
	createRequiredObjects(gen);
	createTreasures(gen);
	
	logGlobal->infoStream() << boost::format ("Zone %d filled successfully") %id;
	return true;
}

bool CRmgTemplateZone::findPlaceForTreasurePile(CMapGenerator* gen, float min_dist, int3 &pos, int value)
{
	float best_distance = 0;
	bool result = false;

	bool needsGuard = value > minGuardedValue;

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;
	for(auto tile : possibleTiles)
	{
		auto dist = gen->getNearestObjectDistance(tile);

		if ((dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			gen->foreach_neighbour (tile, [&gen, &allTilesAvailable, needsGuard](int3 neighbour)
			{
				if (!(gen->isPossible(neighbour) || gen->shouldBeBlocked(neighbour) || (!needsGuard && gen->isFree(neighbour))))
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
		gen->setOccupied(pos, ETileType::BLOCKED); //block that tile //FIXME: why?
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

bool CRmgTemplateZone::isAccessibleFromAnywhere (CMapGenerator* gen, ObjectTemplate &appearance,  int3 &tile) const
{
	return getAccessibleOffset(gen, appearance, tile).valid();
}

int3 CRmgTemplateZone::getAccessibleOffset(CMapGenerator* gen, ObjectTemplate &appearance, int3 &tile) const
{
	auto tilesBlockedByObject = appearance.getBlockedOffsets();

	int3 ret(-1, -1, -1);
	for (int x = -1; x < 2; x++)
	{
		for (int y = -1; y <2; y++)
		{
			if (x && y) //check only if object is visitable from another tile
			{
				int3 offset = int3(x, y, 0) - appearance.getVisitableOffset();
				if (!vstd::contains(tilesBlockedByObject, offset))
				{
					int3 nearbyPos = tile + offset;
					if (gen->map->isInTheMap(nearbyPos))
					{
						if (appearance.isVisitableFrom(x, y) && !gen->isBlocked(nearbyPos))
							ret = nearbyPos;
					}
				}
			}
		};
	}
	return ret;
}

void CRmgTemplateZone::setTemplateForObject(CMapGenerator* gen, CGObjectInstance* obj)
{
	if (obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID)->getTemplates(gen->map->getTile(getPos()).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") % obj->ID %obj->subID %pos));

		obj->appearance = templates.front();
	}
}

bool CRmgTemplateZone::areAllTilesAvailable(CMapGenerator* gen, CGObjectInstance* obj, int3& tile, std::set<int3>& tilesBlockedByObject) const
{
	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = tile + blockingTile;
		if (!gen->map->isInTheMap(t) || !gen->isPossible(t))
		{
			//if at least one tile is not possible, object can't be placed here
			return false;
		}
	}
	return true;
}

bool CRmgTemplateZone::findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tile
	setTemplateForObject(gen, obj);

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
		if (!isAccessibleFromAnywhere(gen, obj->appearance, tile))
			continue;

		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			if (areAllTilesAvailable(gen, obj, tile, tilesBlockedByObject))
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
	if (object->ID == Obj::SEER_HUT) //debug
	{
		CGSeerHut * sh = dynamic_cast<CGSeerHut *>(object);
		auto artid = sh->quest->m5arts.front();
		logGlobal->warnStream() << boost::format("Placed Seer Hut at %s, quest artifact %d is %s") % object->pos % artid % VLC->arth->artifacts[artid]->Name();
	}

	
	switch (object->ID)
	{
	case Obj::TOWN:
	case Obj::RANDOM_TOWN:
	case Obj::MONOLITH_TWO_WAY:
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::SUBTERRANEAN_GATE:
		{
			addRoadNode(object->visitablePos());
		}
		break;
	
	default:
		break;
	}		
}

void CRmgTemplateZone::placeAndGuardObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos, si32 str, bool zoneGuard)
{
	placeObject(gen, object, pos);
	guardObject(gen, object, str, zoneGuard);
}

void CRmgTemplateZone::placeSubterraneanGate(CMapGenerator* gen, int3 pos, si32 guardStrength)
{
	auto gate = new CGSubterraneanGate;
	gate->ID = Obj::SUBTERRANEAN_GATE;
	gate->subID = 0;
	placeObject (gen, gate, pos, true);
	addToConnectLater (getAccessibleOffset (gen, gate->appearance, pos)); //guard will be placed on accessibleOffset
	guardObject (gen, gate, guardStrength, true);
}

std::vector<int3> CRmgTemplateZone::getAccessibleOffsets (CMapGenerator* gen, CGObjectInstance* object)
{
	//get all tiles from which this object can be accessed
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;

	auto tilesBlockedByObject = object->getBlockedPos(); //absolue value, as object is already placed

	gen->foreach_neighbour(visitable, [&](int3& pos) 
	{
		if (gen->isPossible(pos) || gen->isFree(pos))
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
	std::vector<int3> tiles = getAccessibleOffsets(gen, object);

	int3 guardTile(-1, -1, -1);

	if (tiles.size())
	{
		//guardTile = tiles.front();
		guardTile = getAccessibleOffset(gen, object->appearance, object->pos);
		logGlobal->traceStream() << boost::format("Guard object at %s") % object->pos();
	}
	else
	{
		logGlobal->errorStream() << boost::format("Failed to guard object at %s") % object->pos();
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
	ui32 maxVal = desiredValue - currentValue;
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
				if (!isAccessibleFromAnywhere(gen, oi.templ, newVisitablePos))
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

	return ObjectInfo(); // unreachable
}

void CRmgTemplateZone::addAllPossibleObjects(CMapGenerator* gen)
{
	ObjectInfo oi;
	oi.maxPerMap = std::numeric_limits<ui32>().max();

	int numZones = gen->getZones().size();

	std::vector<CCreature *> creatures; //native creatures for this zone
	for (auto cre : VLC->creh->creatures)
	{
		if (!cre->special && cre->faction == townType)
		{
			creatures.push_back(cre);
		}
	}

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
						vstd::amin(oi.maxPerZone, rmgInfo.mapLimit / numZones); //simple, but should distribute objects evenly on large maps
						possibleObjects.push_back(oi);
					}
				}
			}
		}
	}

	//prisons
	//levels 1, 5, 10, 20, 30
	static int prisonExp[] = { 0, 5000, 15000, 90000, 500000 };
	static int prisonValues[] = { 2500, 5000, 10000, 20000, 30000 };

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
			obj->exp = prisonExp[i];
			obj->setOwner(PlayerColor::NEUTRAL);
			gen->map->allowedHeroes[hid] = false; //ban this hero
			gen->decreasePrisonsRemaining();
			obj->appearance = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(terrainType).front(); //can't init template with hero subID

			return obj;
		};
		oi.setTemplate(Obj::PRISON, 0, terrainType);
		oi.value = prisonValues[i];
		oi.probability = 30;
		oi.maxPerZone = gen->getPrisonsRemaning() / 5; //probably not perfect, but we can't generate more prisons than hereos.
		possibleObjects.push_back(oi);
	}

	//all following objects are unlimited
	oi.maxPerZone = std::numeric_limits<ui32>().max();

	//dwellings

	auto subObjects = VLC->objtypeh->knownSubObjects(Obj::CREATURE_GENERATOR1);

	//don't spawn original "neutral" dwellings that got replaced by Conflux dwellings in AB
	static int elementalConfluxROE[] = { 7, 13, 16, 47 };
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
					possibleObjects.push_back(oi);
				}
			}
		}
	}

	static const int scrollValues[] = { 500, 2000, 3000, 4000, 5000 };

	for (int i = 0; i < 5; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::SPELL_SCROLL;
			obj->subID = 0;
			std::vector<SpellID> out;

			for (auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
			{
				if (gen->isAllowedSpell(spell->id) && spell->level == i + 1)
				{
					out.push_back(spell->id);
				}
			}
			auto a = CArtifactInstance::createScroll(RandomGeneratorUtil::nextItem(out, gen->rand)->toSpell());
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			return obj;
		};
		oi.setTemplate(Obj::SPELL_SCROLL, 0, terrainType);
		oi.value = scrollValues[i];
		oi.probability = 30;
		possibleObjects.push_back(oi);
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
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 5000;;
		oi.probability = 5;
		possibleObjects.push_back(oi);
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
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 6000;;
		oi.probability = 20;
		possibleObjects.push_back(oi);
	}

	//pandora box with creatures
	static const int tierValues[] = { 5000, 7000, 9000, 12000, 16000, 21000, 27000 };

	auto creatureToCount = [](CCreature * creature) -> int
	{
		int actualTier = creature->level > 7 ? 6 : creature->level - 1;
		float creaturesAmount = ((float)tierValues[actualTier]) / creature->AIValue;
		if (creaturesAmount <= 5)
		{
			creaturesAmount = boost::math::round(creaturesAmount); //allow single monsters
			if (creaturesAmount < 1)
				return 0;
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
		return creaturesAmount;
	};

	for (auto creature : creatures)
	{
		int creaturesAmount = creatureToCount(creature);
		if (!creaturesAmount)
			continue;

		oi.generateObject = [creature, creaturesAmount]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;
			auto stack = new CStackInstance(creature, creaturesAmount);
			obj->creatures.putStack(SlotID(0), stack);
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = (2 * (creature->AIValue) * creaturesAmount * (1 + (float)(gen->getZoneCount(creature->faction)) / gen->getTotalZoneCount())) / 3;
		oi.probability = 3;
		possibleObjects.push_back(oi);
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
				if (gen->isAllowedSpell(spell->id) && spell->level == i)
					spells.push_back(spell);
			}

			RandomGeneratorUtil::randomShuffle(spells, gen->rand);
			for (int j = 0; j < std::min<int>(12, spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = (i + 1) * 2500; //5000 - 15000
		oi.probability = 2;
		possibleObjects.push_back(oi);
	}

	//Pandora with 15 spells of certain school
	for (int i = 0; i < 4; i++)
	{
		oi.generateObject = [i,gen]() -> CGObjectInstance *
		{
			auto obj = new CGPandoraBox();
			obj->ID = Obj::PANDORAS_BOX;
			obj->subID = 0;

			std::vector <CSpell *> spells;
			for (auto spell : VLC->spellh->objects)
			{

				if (gen->isAllowedSpell(spell->id) && spell->school[(ESpellSchool)i])
					spells.push_back(spell);
			}

			RandomGeneratorUtil::randomShuffle(spells, gen->rand);
			for (int j = 0; j < std::min<int>(15, spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = 15000;
		oi.probability = 2;
		possibleObjects.push_back(oi);
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
			if (gen->isAllowedSpell(spell->id))
				spells.push_back(spell);
		}

		RandomGeneratorUtil::randomShuffle(spells, gen->rand);
		for (int j = 0; j < std::min<int>(60, spells.size()); j++)
		{
			obj->spells.push_back(spells[j]->id);
		}

		return obj;
	};
	oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
	oi.value = 30000;
	oi.probability = 2;
	possibleObjects.push_back(oi);

	//seer huts with creatures or generic rewards

	if (questArtZone) //we won't be placing seer huts if there is no zone left to place arties
	{
		static const int genericSeerHuts = 8;
		int seerHutsPerType = 0;
		const int questArtsRemaining = gen->getQuestArtsRemaning().size();

		//general issue is that not many artifact types are available for quests

		if (questArtsRemaining >= genericSeerHuts + creatures.size())
		{
			seerHutsPerType = questArtsRemaining / (genericSeerHuts + creatures.size());
		}
		else if (questArtsRemaining >= genericSeerHuts)
		{
			seerHutsPerType = 1;
		}
		oi.maxPerZone = seerHutsPerType;

		RandomGeneratorUtil::randomShuffle(creatures, gen->rand);

		auto generateArtInfo = [this](ArtifactID id) -> ObjectInfo
		{
			ObjectInfo artInfo;
			artInfo.probability = std::numeric_limits<ui16>::max(); //99,9% to spawn that art in first treasure pile
			artInfo.maxPerZone = 1;
			artInfo.value = 2000; //treasure art
			artInfo.setTemplate(Obj::ARTIFACT, id, this->terrainType);
			artInfo.generateObject = [id]() -> CGObjectInstance *
			{
				auto handler = VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, id);
				return handler->create(handler->getTemplates().front());
			};
			return artInfo;
		};

		for (int i = 0; i < std::min<int>(creatures.size(), questArtsRemaining - genericSeerHuts); i++)
		{
			auto creature = creatures[i];
			int creaturesAmount = creatureToCount(creature);

			if (!creaturesAmount)
				continue;

			int randomAppearance = *RandomGeneratorUtil::nextItem(VLC->objtypeh->knownSubObjects(Obj::SEER_HUT), gen->rand);

			oi.generateObject = [creature, creaturesAmount, randomAppearance, gen, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto obj = new CGSeerHut();
				obj->ID = Obj::SEER_HUT;
				obj->subID = randomAppearance;
				obj->rewardType = CGSeerHut::CREATURE;
				obj->rID = creature->idNumber;
				obj->rVal = creaturesAmount;

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);
				gen->map->addQuest(obj);

				this->questArtZone->possibleObjects.push_back (generateArtInfo(artid));

				return obj;
			};
			oi.setTemplate(Obj::SEER_HUT, randomAppearance, terrainType);
			oi.value = ((2 * (creature->AIValue) * creaturesAmount * (1 + (float)(gen->getZoneCount(creature->faction)) / gen->getTotalZoneCount())) - 4000) / 3;
			oi.probability = 3;
			possibleObjects.push_back(oi);
		}

		static int seerExpGold[] = { 5000, 10000, 15000, 20000 };
		static int seerValues[] = { 2000, 5333, 8666, 12000 };

		for (int i = 0; i < 4; i++) //seems that code for exp and gold reward is similiar
		{
			int randomAppearance = *RandomGeneratorUtil::nextItem(VLC->objtypeh->knownSubObjects(Obj::SEER_HUT), gen->rand);

			oi.setTemplate(Obj::SEER_HUT, randomAppearance, terrainType);
			oi.value = seerValues[i];
			oi.probability = 10;

			oi.generateObject = [i, randomAppearance, gen, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto obj = new CGSeerHut();
				obj->ID = Obj::SEER_HUT;
				obj->subID = randomAppearance;
				obj->rewardType = CGSeerHut::EXPERIENCE;
				obj->rID = 0; //unitialized?
				obj->rVal = seerExpGold[i];

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);
				gen->map->addQuest(obj);

				this->questArtZone->possibleObjects.push_back(generateArtInfo(artid));

				return obj;
			};

			possibleObjects.push_back(oi);

			oi.generateObject = [i, randomAppearance, gen, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto obj = new CGSeerHut();
				obj->ID = Obj::SEER_HUT;
				obj->subID = randomAppearance;
				obj->rewardType = CGSeerHut::RESOURCES;
				obj->rID = Res::GOLD;
				obj->rVal = seerExpGold[i];

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);
				gen->map->addQuest(obj);

				this->questArtZone->possibleObjects.push_back(generateArtInfo(artid));

				return obj;
			};

			possibleObjects.push_back(oi);
		}
	}
}

void ObjectInfo::setTemplate (si32 type, si32 subtype, ETerrainType terrainType)
{
	templ = VLC->objtypeh->getHandlerFor(type, subtype)->getTemplates(terrainType).front();
}
