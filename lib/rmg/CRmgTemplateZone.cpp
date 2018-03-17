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

using namespace rmg; //TODO: move all to namespace



void CRmgTemplateZone::addRoadNode(const int3& node)
{
	roadNodes.insert(node);
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


CRmgTemplateZone::CRmgTemplateZone()
	: ZoneOptions(),
	townType(ETownType::NEUTRAL),
	terrainType (ETerrainType::GRASS),
	minGuardedValue(0),
	questArtZone(),
	gen(nullptr)
{

}

void CRmgTemplateZone::setOptions(const ZoneOptions * options)
{
	ZoneOptions::operator=(*options);
}

void CRmgTemplateZone::setGenPtr(CMapGenerator * Gen)
{
	gen = Gen;
}

void CRmgTemplateZone::setQuestArtZone(std::shared_ptr<CRmgTemplateZone> otherZone)
{
	questArtZone = otherZone;
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

	//alternate solution - wrap zone around unitary square. If it doesn't fit on one side, will come out on the opposite side
	center = f;

	center.x = std::fmod(center.x, 1);
	center.y = std::fmod(center.y, 1);

	if (center.x < 0) //fmod seems to work only for positive numbers? we want to stay positive
		center.x = 1 - std::abs(center.x);
	if (center.y < 0)
		center.y = 1 - std::abs(center.y);
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
std::set<int3> CRmgTemplateZone::getPossibleTiles() const
{
	return possibleTiles;
}

void CRmgTemplateZone::discardDistantTiles (float distance)
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

void CRmgTemplateZone::initFreeTiles ()
{
	vstd::copy_if (tileinfo, vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return gen->isPossible(tile);
	});
	if (freePaths.empty())
	{
		gen->setOccupied(pos, ETileType::FREE);
		freePaths.insert(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

void CRmgTemplateZone::createBorder()
{
	for (auto tile : tileinfo)
	{
		bool edge = false;
		gen->foreach_neighbour(tile, [this, &edge](int3 &pos)
		{
			if (edge)
				return; //optimization - do it only once
			if (gen->getZoneID(pos) != id) //optimization - better than set search
			{
				//we are edge if at least one tile does not belong to zone
				//mark all nearby tiles blocked and we're done
				gen->foreach_neighbour (pos, [this](int3 &nearbyPos)
				{
					if (gen->isPossible(nearbyPos))
						gen->setOccupied(nearbyPos, ETileType::BLOCKED);
				});
				edge = true;
			}
		});
	}
}

void CRmgTemplateZone::fractalize()
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

	//cut straight paths towards the center. A* is too slow for that.
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
		crunchPath(node, findClosestTile(freePaths, node), true, &freePaths);
		//connect with nearby nodes
		for (auto nearbyNode : nearbyNodes)
		{
			crunchPath(node, nearbyNode, true, &freePaths);
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
}

void CRmgTemplateZone::connectLater()
{
	for (const int3 node : tilesToConnectLater)
	{
		if (!connectWithCenter(node, true))
			logGlobal->error("Failed to connect node %s with center of the zone", node.toString());
	}
}

bool CRmgTemplateZone::crunchPath(const int3 &src, const int3 &dst, bool onlyStraight, std::set<int3>* clearedTiles)
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

		auto processNeighbours = [this, &currentPos, dst, &distance, &result, &end, clearedTiles](int3 &pos)
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
						if (gen->getZoneID(pos) == id)
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

			auto processNeighbours2 = [this, &currentPos, dst, &lastDistance, &anotherPos, clearedTiles](int3 &pos)
			{
				if (currentPos.dist2dSQ(dst) < lastDistance) //try closest tiles from all surrounding unused tiles
				{
					if (gen->getZoneID(pos) == id)
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
			//logGlobal->warn("No tile closer than %s found on path from %s to %s", currentPos, src , dst);
			break;
		}
	}

	return result;
}
boost::heap::priority_queue<CRmgTemplateZone::TDistance, boost::heap::compare<CRmgTemplateZone::NodeComparer>> CRmgTemplateZone::createPiorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}

bool CRmgTemplateZone::createRoad(const int3& src, const int3& dst)
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	auto pq = createPiorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	gen->setRoad (src, ERoadType::NO_ROAD); //just in case zone guard already has road under it. Road under nodes will be added at very end

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	pq.push(std::make_pair(src, 0.f));
	distances[src] = 0.f;
	// Cost from start along best known path.

	while (!pq.empty())
	{
		auto node = pq.top();
		pq.pop(); //remove top element
		int3 currentNode = node.first;
		closed.insert (currentNode);
		auto currentTile = &gen->map->getTile(currentNode);

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
				//logGlobal->trace("Setting road at tile %s", backTracking);
				// do the same for the predecessor
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			bool directNeighbourFound = false;
			float movementCost = 1;

			auto foo = [this, &pq, &distances, &closed, &cameFrom, &currentNode, &currentTile, &node, &dst, &directNeighbourFound, &movementCost](int3& pos) -> void
			{
				if (vstd::contains(closed, pos)) //we already visited that node
					return;
				float distance = node.second + movementCost;
				float bestDistanceSoFar = std::numeric_limits<float>::max();
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (distance < bestDistanceSoFar)
				{
					auto tile = &gen->map->getTile(pos);
					bool canMoveBetween = gen->map->canMoveBetween(currentNode, pos);

					if ((gen->isFree(pos) && gen->isFree(currentNode)) //empty path
						|| ((tile->visitable || currentTile->visitable) && canMoveBetween) //moving from or to visitable object
						|| pos == dst) //we already compledted the path
					{
						if (gen->getZoneID(pos) == id || pos == dst) //otherwise guard position may appear already connected to other zone.
						{
							cameFrom[pos] = currentNode;
							distances[pos] = distance;
							pq.push(std::make_pair(pos, distance));
							directNeighbourFound = true;
						}
					}
				}
			};

			gen->foreachDirectNeighbour (currentNode, foo); // roads cannot be rendered correctly for diagonal directions
			if (!directNeighbourFound)
			{
				movementCost = 2.1f; //moving diagonally is penalized over moving two tiles straight
				gen->foreachDiagonaltNeighbour(currentNode, foo);
			}
		}

	}
	logGlobal->warn("Failed to create road from %s to %s", src.toString(), dst.toString());
	return false;

}

bool CRmgTemplateZone::connectPath(const int3& src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	auto open = createPiorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	//int3 currentNode = src;

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0.f;
	open.push(std::make_pair(src, 0.f));
	// Cost from start along best known path.
	// Estimated total cost from start to goal through y.

	while (!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;

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
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances](int3& pos) -> void
			{
				if (vstd::contains(closed, pos))
					return;

				//no paths through blocked or occupied tiles, stay within zone
				if (gen->isBlocked(pos) || gen->getZoneID(pos) != id)
					return;

				int distance = distances[currentNode] + 1;
				int bestDistanceSoFar = std::numeric_limits<int>::max();
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, distance));
					distances[pos] = distance;
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
		gen->setOccupied (tile, ETileType::BLOCKED);
		vstd::erase_if_present(possibleTiles, tile);
	}
	return false;
}

bool CRmgTemplateZone::connectWithCenter(const int3& src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	auto open = createPiorityQueue(); // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;

	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	open.push(std::make_pair(src, 0.f));
	// Cost from start along best known path.

	while (!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;

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
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances](int3& pos) -> void
			{
				if (vstd::contains(closed, pos))
					return;

				if (gen->getZoneID(pos) != id)
					return;

				float movementCost = 0;
				if (gen->isFree(pos))
					movementCost = 1;
				else if (gen->isPossible(pos))
					movementCost = 2;
				else
					return;

				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = std::numeric_limits<int>::max(); //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;

				if (distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, distance));
					distances[pos] = distance;
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

bool CRmgTemplateZone::addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles, bool zoneGuard)
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
		if (!cre->AIValue) //bug #2681
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

	auto guardFactory = VLC->objtypeh->getHandlerFor(Obj::MONSTER, creId);

	auto guard = (CGCreature *) guardFactory->create(ObjectTemplate());
	guard->character = CGCreature::HOSTILE;
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);

	placeObject(guard, pos);

	if (clearSurroundingTiles)
	{
		//do not spawn anything near monster
		gen->foreach_neighbour (pos, [this](int3 pos)
		{
			if (gen->isPossible(pos))
				gen->setOccupied(pos, ETileType::FREE);
		});
	}

	return true;
}

bool CRmgTemplateZone::createTreasurePile(int3 &pos, float minDistance, const CTreasureInfo& treasureInfo)
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
			gen->foreach_neighbour(treasurePos.first, [&boundary](int3 pos)
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

		ObjectInfo oi = getRandomObject(info, desiredValue, maxValue, currentValue);
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
			info.occupiedPositions.insert(visitablePos + oi.templ.getVisitableOffset());

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
					gen->foreach_neighbour (tile, [this, &here, minDistance](int3 pos)
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
		float minTreasureDistance = 1e10;

		for (auto visitablePos : info.visitableFromBottomPositions) //objects that are not visitable from top must be accessible from bottom or side
		{
			int3 closestFreeTile = findClosestTile(freePaths, visitablePos);
			if (closestFreeTile.dist2d(visitablePos) < minTreasureDistance)
			{
				closestTile = visitablePos + int3 (0, 1, 0); //start below object (y+1), possibly even outside the map, to not make path up through it
				minTreasureDistance = closestFreeTile.dist2d(visitablePos);
			}
		}
		for (auto visitablePos : info.visitableFromTopPositions) //all objects are accessible from any direction
		{
			int3 closestFreeTile = findClosestTile(freePaths, visitablePos);
			if (closestFreeTile.dist2d(visitablePos) < minTreasureDistance)
			{
				closestTile = visitablePos;
				minTreasureDistance = closestFreeTile.dist2d(visitablePos);
			}
		}
		assert (closestTile.valid());

		for (auto tile : info.occupiedPositions)
		{
			if (gen->map->isInTheMap(tile)) //pile boundary may reach map border
				gen->setOccupied(tile, ETileType::BLOCKED); //so that crunch path doesn't cut through objects
		}

		if (!connectPath (closestTile, false)) //this place is sealed off, need to find new position
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
				placeObject(treasure.second, treasure.first + visitableOffset);
			}
			if (addMonster(guardPos, currentValue, false))
			{//block only if the object is guarded
				for (auto tile : boundary)
				{
					if (gen->isPossible(tile))
						gen->setOccupied(tile, ETileType::BLOCKED);
				}
				//do not spawn anything near monster
				gen->foreach_neighbour(guardPos, [this](int3 pos)
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
void CRmgTemplateZone::initTownType ()
{
	//FIXME: handle case that this player is not present -> towns should be set to neutral
	int totalTowns = 0;

	//cut a ring around town to ensure crunchPath always hits it.
	auto cutPathAroundTown = [this](const CGTownInstance * town)
	{
		for (auto blockedTile : town->getBlockedPos())
		{
			gen->foreach_neighbour(blockedTile, [this](const int3 & pos)
			{
				if (gen->isPossible(pos))
					gen->setOccupied(pos, ETileType::FREE);
			});
		}
	};

	auto addNewTowns = [&totalTowns, this, &cutPathAroundTown](int count, bool hasFort, PlayerColor player)
	{
		for (int i = 0; i < count; i++)
		{
			si32 subType = townType;

			if(totalTowns>0)
			{
				if(!this->townsAreSameType)
				{
					if (townTypes.size())
						subType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
					else
						subType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed
				}
			}

			auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, subType);
			auto town = (CGTownInstance *) townFactory->create(ObjectTemplate());
			town->ID = Obj::TOWN;

			town->tempOwner = player;
			if (hasFort)
				town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);

			for(auto spell : VLC->spellh->objects) //add all regular spells to town
			{
				if(!spell->isSpecial() && !spell->isCreatureAbility())
					town->possibleSpells.push_back(spell->id);
			}

			if (totalTowns <= 0)
			{
				//register MAIN town of zone
				gen->registerZone(town->subID);
				//first town in zone goes in the middle
				placeObject(town, getPos() + town->getVisitableOffset(), true);
				cutPathAroundTown(town);
				setPos(town->visitablePos()); //roads lead to mian town
			}
			else
				addRequiredObject (town);
			totalTowns++;
		}
	};


	if ((type == ETemplateZoneType::CPU_START) || (type == ETemplateZoneType::PLAYER_START))
	{
		//set zone types to player faction, generate main town
		logGlobal->info("Preparing playing zone");
		int player_id = *owner - 1;
		auto & playerInfo = gen->map->players[player_id];
		PlayerColor player(player_id);
		if (playerInfo.canAnyonePlay())
		{
			player = PlayerColor(player_id);
			townType = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if (townType == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				randomizeTownType();
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			randomizeTownType();
		}

		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, townType);

		CGTownInstance * town = (CGTownInstance *) townFactory->create(ObjectTemplate());
		town->tempOwner = player;
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);

		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		//towns are big objects and should be centered around visitable position
		placeObject(town, getPos() + town->getVisitableOffset(), true);
		cutPathAroundTown(town);
		setPos(town->visitablePos()); //roads lead to mian town

		totalTowns++;
		//register MAIN town of zone only
		gen->registerZone (town->subID);

		if (playerInfo.canAnyonePlay()) //configure info for owning player
		{
			logGlobal->trace("Fill player info %d", player_id);

			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(townType);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos;
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
		randomizeTownType();
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
				randomizeTownType();
		}
	}
}

void CRmgTemplateZone::randomizeTownType ()
{
	if (townTypes.size())
		townType = *RandomGeneratorUtil::nextItem(townTypes, gen->rand);
	else
		townType = *RandomGeneratorUtil::nextItem(getDefaultTownTypes(), gen->rand); //it is possible to have zone with no towns allowed, we still need some
}

void CRmgTemplateZone::initTerrainType ()
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

	paintZoneTerrain (terrainType);
}

void CRmgTemplateZone::paintZoneTerrain (ETerrainType terrainType)
{
	std::vector<int3> tiles(tileinfo.begin(), tileinfo.end());
	gen->editManager->getTerrainSelection().setSelection(tiles);
	gen->editManager->drawTerrain(terrainType, &gen->rand);
}

bool CRmgTemplateZone::placeMines ()
{
	static const Res::ERes woodOre[] = {Res::ERes::WOOD, Res::ERes::ORE};
	static const Res::ERes preciousResources[] = {Res::ERes::GEMS, Res::ERes::CRYSTAL, Res::ERes::MERCURY, Res::ERes::SULFUR};

	std::array<TObjectTypeHandler, 7> factory =
	{
		VLC->objtypeh->getHandlerFor(Obj::MINE, 0),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 1),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 2),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 3),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 4),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 5),
		VLC->objtypeh->getHandlerFor(Obj::MINE, 6)
	};

	for (const auto & res : woodOre)
	{
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = (CGMine *) factory.at(static_cast<si32>(res))->create(ObjectTemplate());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
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
			auto mine = (CGMine *) factory.at(static_cast<si32>(res))->create(ObjectTemplate());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			addRequiredObject(mine, 3500);
		}
	}
	for (int i = 0; i < mines[Res::GOLD]; i++)
	{
		auto mine = (CGMine *) factory.at(Res::GOLD)->create(ObjectTemplate());
		mine->producedResource = Res::GOLD;
		mine->tempOwner = PlayerColor::NEUTRAL;
		mine->producedQuantity = mine->defaultResProduction();
		addRequiredObject(mine, 7000);
	}

	return true;
}

EObjectPlacingResult::EObjectPlacingResult CRmgTemplateZone::tryToPlaceObjectAndConnectToPath(CGObjectInstance *obj, int3 &pos)
{
	//check if we can find a path around this object. Tiles will be set to "USED" after object is successfully placed.
	obj->pos = pos;
	gen->setOccupied(obj->visitablePos(), ETileType::BLOCKED);
	for (auto tile : obj->getBlockedPos())
	{
		if (gen->map->isInTheMap(tile))
			gen->setOccupied(tile, ETileType::BLOCKED);
	}
	int3 accessibleOffset = getAccessibleOffset(obj->appearance, pos);
	if (!accessibleOffset.valid())
	{
		logGlobal->warn("Cannot access required object at position %s, retrying", pos.toString());
		return EObjectPlacingResult::CANNOT_FIT;
	}
	if (!connectPath(accessibleOffset, true))
	{
		logGlobal->trace("Failed to create path to required object at position %s, retrying", pos.toString());
		return EObjectPlacingResult::SEALED_OFF;
	}
	else
		return EObjectPlacingResult::SUCCESS;
}

bool CRmgTemplateZone::createRequiredObjects()
{
	logGlobal->trace("Creating required objects");

	for(const auto &object : requiredObjects)
	{
		auto obj = object.first;
		int3 pos;
		while (true)
		{
			if (!findPlaceForObject(obj, 3, pos))
			{
				logGlobal->error("Failed to fill zone %d due to lack of space", id);
				return false;
			}
			if (tryToPlaceObjectAndConnectToPath(obj, pos) == EObjectPlacingResult::SUCCESS)
			{
				//paths to required objects constitute main paths of zone. otherwise they just may lead to middle and create dead zones
				placeObject(obj, pos);
				guardObject(obj, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY), true);
				break;
			}
		}
	}

	for (const auto &obj : closeObjects)
	{
		setTemplateForObject(obj.first);
		auto tilesBlockedByObject = obj.first->getBlockedOffsets();

		bool finished = false;
		while (!finished)
		{
			std::vector<int3> tiles(possibleTiles.begin(), possibleTiles.end());
			//new tiles vector after each object has been placed, OR misplaced area has been sealed off

			boost::remove_if(tiles, [obj, this](int3 &tile)-> bool
			{
				//object must be accessible from at least one surounding tile
				return !this->isAccessibleFromAnywhere(obj.first->appearance, tile);
			});

			// smallest distance to zone center, greatest distance to nearest object
			auto isCloser = [this](const int3 & lhs, const int3 & rhs) -> bool
			{
				float lDist = this->pos.dist2d(lhs);
				float rDist = this->pos.dist2d(rhs);
				lDist *= (lDist > 12) ? 10 : 1; //objects within 12 tile radius are preferred (smaller distance rating)
				rDist *= (rDist > 12) ? 10 : 1;

				return (lDist * 0.5f - std::sqrt(gen->getNearestObjectDistance(lhs))) < (rDist * 0.5f - std::sqrt(gen->getNearestObjectDistance(rhs)));
			};

			boost::sort(tiles, isCloser);

			if (tiles.empty())
			{
				logGlobal->error("Failed to fill zone %d due to lack of space", id);
				return false;
			}
			for (auto tile : tiles)
			{
				//code partially adapted from findPlaceForObject()

				if (areAllTilesAvailable(obj.first, tile, tilesBlockedByObject))
					gen->setOccupied(pos, ETileType::BLOCKED); //why?
				else
					continue;

				EObjectPlacingResult::EObjectPlacingResult result = tryToPlaceObjectAndConnectToPath(obj.first, tile);
				if (result == EObjectPlacingResult::SUCCESS)
				{
					placeObject(obj.first, tile);
					guardObject(obj.first, obj.second, (obj.first->ID == Obj::MONOLITH_TWO_WAY), true);
					finished = true;
					break;
				}
				else if (result == EObjectPlacingResult::CANNOT_FIT)
					continue; // next tile
				else if (result == EObjectPlacingResult::SEALED_OFF)
				{
					break; //tiles expired, pick new ones
				}
				else
					throw (rmgException("Wrong result of tryToPlaceObjectAndConnectToPath()"));
			}
		}
	}

	return true;
}

void CRmgTemplateZone::createTreasures()
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

	//sort treasures by ascending value so we can stop checking treasures with too high value
	boost::sort(possibleObjects, [](const ObjectInfo& oi1, const ObjectInfo& oi2) -> bool
	{
		return oi1.value < oi2.value;
	});

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
			vstd::erase_if(possibleTiles, [this](const int3 &tile) -> bool
			{
				return !gen->isPossible(tile);
			});


			int3 treasureTilePos;
			//If we are able to place at least one object with value lower than minGuardedValue, it's ok
			do
			{
				if (!findPlaceForTreasurePile(minDistance, treasureTilePos, t.min))
				{
					stop = true;
					break;
				}
			}
			while (!createTreasurePile(treasureTilePos, minDistance, t)); //failed creation - position was wrong, cannot connect it

		} while (!stop);
	}
}

void CRmgTemplateZone::createObstacles1()
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

void CRmgTemplateZone::createObstacles2()
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

	auto tryToPlaceObstacleHere = [this, &possibleObstacles](int3& tile, int index)-> bool
	{
		auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles[index].second, gen->rand);
		int3 obstaclePos = tile + temp.getBlockMapOffset();
		if (canObstacleBePlacedHere(temp, obstaclePos)) //can be placed here
		{
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			placeObject(obj, obstaclePos, false);
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

void CRmgTemplateZone::connectRoads()
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

		logGlobal->debug("Building road from %s to %s", node.toString(), cross.toString());
		if (createRoad(node, cross))
		{
			processed.insert(cross); //don't draw road starting at end point which is already connected
			vstd::erase_if_present(roadNodesCopy, cross);
		}

		processed.insert(node);
	}

	drawRoads();

	logGlobal->debug("Finished building roads");
}

void CRmgTemplateZone::drawRoads()
{
	std::vector<int3> tiles;
	for (auto tile : roads)
	{
		if(gen->map->isInTheMap(tile))
			tiles.push_back (tile);
	}
	for (auto tile : roadNodes)
	{
		if (gen->getZoneID(tile) == id) //mark roads for our nodes, but not for zone guards in other zones
			tiles.push_back(tile);
	}

	gen->editManager->getTerrainSelection().setSelection(tiles);
	gen->editManager->drawRoad(ERoadType::COBBLESTONE_ROAD, &gen->rand);
}


bool CRmgTemplateZone::fill()
{
	initTerrainType();

	//zone center should be always clear to allow other tiles to connect
	gen->setOccupied(pos, ETileType::FREE);
	freePaths.insert(pos);

	addAllPossibleObjects ();

	connectLater(); //ideally this should work after fractalize, but fails
	fractalize();
	placeMines();
	createRequiredObjects();
	createTreasures();

	logGlobal->info("Zone %d filled successfully", id);
	return true;
}

bool CRmgTemplateZone::findPlaceForTreasurePile(float min_dist, int3 &pos, int value)
{
	float best_distance = 0;
	bool result = false;

	bool needsGuard = value > minGuardedValue;

	//logGlobal->info("Min dist for density %f is %d", density, min_dist);
	for(auto tile : possibleTiles)
	{
		auto dist = gen->getNearestObjectDistance(tile);

		if ((dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			gen->foreach_neighbour (tile, [this, &allTilesAvailable, needsGuard](int3 neighbour)
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

bool CRmgTemplateZone::canObstacleBePlacedHere(ObjectTemplate &temp, int3 &pos)
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

bool CRmgTemplateZone::isAccessibleFromAnywhere (ObjectTemplate &appearance,  int3 &tile) const
{
	return getAccessibleOffset(appearance, tile).valid();
}

int3 CRmgTemplateZone::getAccessibleOffset(ObjectTemplate &appearance, int3 &tile) const
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
		}
	}
	return ret;
}

void CRmgTemplateZone::setTemplateForObject(CGObjectInstance* obj)
{
	if (obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID)->getTemplates(gen->map->getTile(getPos()).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") % obj->ID % obj->subID % pos.toString()));

		obj->appearance = templates.front();
	}
}

bool CRmgTemplateZone::areAllTilesAvailable(CGObjectInstance* obj, int3& tile, std::set<int3>& tilesBlockedByObject) const
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

bool CRmgTemplateZone::findPlaceForObject(CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tile
	setTemplateForObject(obj);

	int best_distance = 0;
	bool result = false;

	auto tilesBlockedByObject = obj->getBlockedOffsets();

	for (auto tile : tileinfo)
	{
		//object must be accessible from at least one surounding tile
		if (!isAccessibleFromAnywhere(obj->appearance, tile))
			continue;

		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			if (areAllTilesAvailable(obj, tile, tilesBlockedByObject))
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

void CRmgTemplateZone::checkAndPlaceObject(CGObjectInstance* object, const int3 &pos)
{
	if (!gen->map->isInTheMap(pos))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % object->id % pos.toString()));
	object->pos = pos;

	if (object->isVisitable() && !gen->map->isInTheMap(object->visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % object->visitablePos().toString() % object->id % object->pos.toString()));
	for (auto tile : object->getBlockedPos())
	{
		if (!gen->map->isInTheMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % object->id % object->pos.toString()));
	}

	if (object->appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = gen->map->getTile(pos).terType;
		auto templates = VLC->objtypeh->getHandlerFor(object->ID, object->subID)->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % object->ID % object->subID % pos.toString() % terrainType));

		object->appearance = templates.front();
	}

	gen->editManager->insertObject(object);
}

void CRmgTemplateZone::placeObject(CGObjectInstance* object, const int3 &pos, bool updateDistance)
{
	checkAndPlaceObject (object, pos);

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
		updateDistances(pos);

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

void CRmgTemplateZone::updateDistances(const int3 & pos)
{
	for (auto tile : possibleTiles) //don't need to mark distance for not possible tiles
	{
		ui32 d = pos.dist2dSQ(tile); //optimization, only relative distance is interesting
		gen->setNearestObjectDistance(tile, std::min<float>(d, gen->getNearestObjectDistance(tile)));
	}
}

void CRmgTemplateZone::placeAndGuardObject(CGObjectInstance* object, const int3 &pos, si32 str, bool zoneGuard)
{
	placeObject(object, pos);
	guardObject(object, str, zoneGuard);
}

void CRmgTemplateZone::placeSubterraneanGate(int3 pos, si32 guardStrength)
{
	auto factory = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
	auto gate = factory->create(ObjectTemplate());
	placeObject (gate, pos, true);
	addToConnectLater (getAccessibleOffset (gate->appearance, pos)); //guard will be placed on accessibleOffset
	guardObject (gate, guardStrength, true);
}

std::vector<int3> CRmgTemplateZone::getAccessibleOffsets (const CGObjectInstance* object)
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

bool CRmgTemplateZone::guardObject(CGObjectInstance* object, si32 str, bool zoneGuard, bool addToFreePaths)
{
	std::vector<int3> tiles = getAccessibleOffsets(object);

	int3 guardTile(-1, -1, -1);

	if (tiles.size())
	{
		//guardTile = tiles.front();
		guardTile = getAccessibleOffset(object->appearance, object->pos);
		logGlobal->trace("Guard object at %s", object->pos.toString());
	}
	else
	{
		logGlobal->error("Failed to guard object at %s", object->pos.toString());
		return false;
	}

	if (addMonster (guardTile, str, false, zoneGuard)) //do not place obstacles around unguarded object
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

ObjectInfo CRmgTemplateZone::getRandomObject(CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue)
{
	//int objectsVisitableFromBottom = 0; //for debug

	std::vector<std::pair<ui32, ObjectInfo*>> thresholds; //handle complex object via pointer
	ui32 total = 0;

	//calculate actual treasure value range based on remaining value
	ui32 maxVal = desiredValue - currentValue;
	ui32 minValue = 0.25f * (desiredValue - currentValue);

	//roulette wheel
	for (ObjectInfo &oi : possibleObjects) //copy constructor turned out to be costly
	{
		if (oi.value > maxVal)
			break; //this assumes values are sorted in ascending order
		if (oi.value >= minValue && oi.maxPerZone > 0)
		{
			int3 newVisitableOffset = oi.templ.getVisitableOffset(); //visitablePos assumes object will be shifter by visitableOffset
			int3 newVisitablePos = info.nextTreasurePos;

			if (!oi.templ.isVisitableFromTop())
			{
				//objectsVisitableFromBottom++;
				//there must be free tiles under object
				auto blockedOffsets = oi.templ.getBlockedOffsets();
				if (!isAccessibleFromAnywhere(oi.templ, newVisitablePos))
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

			thresholds.push_back (std::make_pair (total, &oi));
		}
	}

	if (thresholds.empty())
	{
		ObjectInfo oi;
		//Generate pandora Box with gold if the value is extremely high
		if (minValue > 20000) //we don't have object valuable enough
		{
			oi.generateObject = [minValue]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
				auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
				obj->resources[Res::GOLD] = minValue;
				return obj;
			};
			oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
			oi.value = minValue;
			oi.probability = 0;
		}
		else //generate empty object with 0 value if the value if we can't spawn anything
		{
			oi.generateObject = []() -> CGObjectInstance *
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

		//binary search = fastest
		auto it = std::lower_bound(thresholds.begin(), thresholds.end(), r,
			[](const std::pair<ui32, ObjectInfo*> &rhs, const int lhs)->bool
		{
			return rhs.first < lhs;
		});
		return *(it->second);
	}
}

void CRmgTemplateZone::addAllPossibleObjects()
{
	ObjectInfo oi;

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
						oi.generateObject = [temp]() -> CGObjectInstance *
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
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			std::vector<ui32> possibleHeroes;
			for (int j = 0; j < gen->map->allowedHeroes.size(); j++)
			{
				if (gen->map->allowedHeroes[j])
					possibleHeroes.push_back(j);
			}

			auto hid = *RandomGeneratorUtil::nextItem(possibleHeroes, gen->rand);
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0);
			auto obj = (CGHeroInstance *) factory->create(ObjectTemplate());


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
					oi.generateObject = [temp, secondaryID]() -> CGObjectInstance *
					{
						auto obj = VLC->objtypeh->getHandlerFor(Obj::CREATURE_GENERATOR1, secondaryID)->create(temp);
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
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::SPELL_SCROLL, 0);
			auto obj = (CGArtifact *) factory->create(ObjectTemplate());
			std::vector<SpellID> out;

			for (auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
			{
				if (gen->isAllowedSpell(spell->id) && spell->level == i + 1)
				{
					out.push_back(spell->id);
				}
			}
			auto a = CArtifactInstance::createScroll(*RandomGeneratorUtil::nextItem(out, gen->rand));
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
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			obj->resources[Res::GOLD] = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 5000;
		oi.probability = 5;
		possibleObjects.push_back(oi);
	}

	//pandora box with experience
	for (int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			obj->gainedExp = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * 6000;
		oi.probability = 20;
		possibleObjects.push_back(oi);
	}

	//pandora box with creatures
	static const int tierValues[] = { 5000, 7000, 9000, 12000, 16000, 21000, 27000 };

	auto creatureToCount = [](CCreature * creature) -> int
	{
		if (!creature->AIValue) //bug #2681
			return 0; //this box won't be generated

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
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
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
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());

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
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());

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

	oi.generateObject = [this]() -> CGObjectInstance *
	{
		auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
		auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());

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

	if(questArtZone.lock()) //we won't be placing seer huts if there is no zone left to place arties
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

			oi.generateObject = [creature, creaturesAmount, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());
				obj->rewardType = CGSeerHut::CREATURE;
				obj->rID = creature->idNumber;
				obj->rVal = creaturesAmount;

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);

				this->questArtZone.lock()->possibleObjects.push_back (generateArtInfo(artid));

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

			oi.generateObject = [i, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());

				obj->rewardType = CGSeerHut::EXPERIENCE;
				obj->rID = 0; //unitialized?
				obj->rVal = seerExpGold[i];

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);

				this->questArtZone.lock()->possibleObjects.push_back(generateArtInfo(artid));

				return obj;
			};

			possibleObjects.push_back(oi);

			oi.generateObject = [i, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());
				obj->rewardType = CGSeerHut::RESOURCES;
				obj->rID = Res::GOLD;
				obj->rVal = seerExpGold[i];

				obj->quest->missionType = CQuest::MISSION_ART;
				ArtifactID artid = *RandomGeneratorUtil::nextItem(gen->getQuestArtsRemaning(), gen->rand);
				obj->quest->m5arts.push_back(artid);
				obj->quest->lastDay = -1;
				obj->quest->isCustomFirst = obj->quest->isCustomNext = obj->quest->isCustomComplete = false;

				gen->banQuestArt(artid);

				this->questArtZone.lock()->possibleObjects.push_back(generateArtInfo(artid));

				return obj;
			};

			possibleObjects.push_back(oi);
		}
	}
}

ObjectInfo::ObjectInfo()
	: templ(), value(0), probability(0), maxPerZone(1)
{

}

void ObjectInfo::setTemplate (si32 type, si32 subtype, ETerrainType terrainType)
{
	templ = VLC->objtypeh->getHandlerFor(type, subtype)->getTemplates(terrainType).front();
}
