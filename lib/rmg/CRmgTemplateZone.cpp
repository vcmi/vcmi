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

CTileInfo::CTileInfo():nearestObjectDistance(float(INT_MAX)), terrain(ETerrainType::WRONG),roadType(ERoadType::NO_ROAD)
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


CRmgTemplateZone::CRmgTemplateZone(CMapGenerator * Gen)
	: ZoneOptions(),
	townType(ETownType::NEUTRAL),
	terrainType (ETerrainType::GRASS),
	minGuardedValue(0),
	questArtZone(),
	gen(Gen)
{

}

bool CRmgTemplateZone::isUnderground() const
{
	return getPos().z;
}

void CRmgTemplateZone::setOptions(const ZoneOptions& options)
{
	ZoneOptions::operator=(options);
}

void CRmgTemplateZone::setQuestArtZone(std::shared_ptr<CRmgTemplateZone> otherZone)
{
	questArtZone = otherZone;
}

std::set<int3>* CRmgTemplateZone::getFreePaths()
{
	return &freePaths;
}

void CRmgTemplateZone::addFreePath(const int3 & p)
{
	gen->setOccupied(p, ETileType::FREE);
	freePaths.insert(p);
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

	center.x = static_cast<float>(std::fmod(center.x, 1));
	center.y = static_cast<float>(std::fmod(center.y, 1));

	if(center.x < 0) //fmod seems to work only for positive numbers? we want to stay positive
		center.x = 1 - std::abs(center.x);
	if(center.y < 0)
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

void CRmgTemplateZone::addTile (const int3 &Pos)
{
	tileinfo.insert(Pos);
}

void CRmgTemplateZone::removeTile(const int3 & Pos)
{
	tileinfo.erase(Pos);
	possibleTiles.erase(Pos);
}

std::set<int3> CRmgTemplateZone::getTileInfo () const
{
	return tileinfo;
}
std::set<int3> CRmgTemplateZone::getPossibleTiles() const
{
	return possibleTiles;
}

std::set<int3> CRmgTemplateZone::collectDistantTiles (float distance) const
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
	std::set<int3> discardedTiles;
	for(auto& tile : tileinfo)
	{
		if(tile.dist2d(this->pos) > distance)
		{
			discardedTiles.insert(tile);
		}
	};
	return discardedTiles;
}

void CRmgTemplateZone::clearTiles()
{
	tileinfo.clear();
}

void CRmgTemplateZone::initFreeTiles ()
{
	vstd::copy_if(tileinfo, vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return gen->isPossible(tile);
	});
	if(freePaths.empty())
	{
		addFreePath(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

void CRmgTemplateZone::createBorder()
{
	for(auto tile : tileinfo)
	{
		bool edge = false;
		gen->foreach_neighbour(tile, [this, &edge](int3 &pos)
		{
			if (edge)
				return; //optimization - do it only once
			if (gen->getZoneID(pos) != id) //optimization - better than set search
			{
				//bugfix with missing pos
				if (gen->isPossible(pos))
					gen->setOccupied(pos, ETileType::BLOCKED);
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

void CRmgTemplateZone::createWater(EWaterContent::EWaterContent waterContent, bool debug)
{
	if(waterContent == EWaterContent::NONE || isUnderground())
		return; //do nothing
	
	std::set<int3> waterTiles = collectDistantTiles((float)(getSize() + 1));
	
	//add border tiles as water for ISLANDS
	if(waterContent == EWaterContent::ISLANDS)
	{
		for(auto& tile : tileinfo)
		{
			if(gen->shouldBeBlocked(tile))
			{
				waterTiles.insert(tile);
			}
		}
	}
	
	std::list<int3> tilesQueue(waterTiles.begin(), waterTiles.end()); //tiles need to be processed
	std::set<int3> tilesChecked = waterTiles; //tiles already processed
	std::map<int, std::set<int3>> coastTilesMap; //key: distance to water; value: tiles with that distance
	std::map<int3, int> tilesDist; //key: tile; value: distance to water
	
	//optimization: prefill distance for all tiles marked for water with 0
	for(auto& tile : waterTiles)
	{
		tilesDist[tile] = 0;
	}
	
	//fills the distance-to-water map
	while(!tilesQueue.empty())
	{
		int3 src = tilesQueue.front();
		tilesQueue.pop_front();
		gen->foreachDirectNeighbour(src, [this, &src, &tilesDist, &tilesChecked, &coastTilesMap, &tilesQueue](const int3 & dst)
		{
			if(tilesChecked.find(dst) != tilesChecked.end())
				return;
			
			if(tileinfo.find(dst) != tileinfo.end())
			{
				tilesDist[dst] = tilesDist[src] + 1;
				coastTilesMap[tilesDist[dst]].insert(dst);
				tilesChecked.insert(dst);
				tilesQueue.push_back(dst);
			}
		});
	}
	
	//generating some irregularity of coast
	int coastIdMax = fmin(sqrt(coastTilesMap.size()), 7.f); //size of coastTilesMap shows the most distant tile from water
	assert(coastIdMax > 0);
	tilesChecked.clear();
	for(int coastId = coastIdMax; coastId >= 1; --coastId)
	{
		//amount of iterations shall be proportion of coast perimeter
		const int coastLength = coastTilesMap[coastId].size() / (coastId + 3);
		for(int coastIter = 0; coastIter < coastLength; ++coastIter)
		{
			int3 tile = *RandomGeneratorUtil::nextItem(coastTilesMap[coastId], gen->rand);
			if(tilesChecked.find(tile) != tilesChecked.end())
				continue;
			if(gen->isUsed(tile) || gen->isFree(tile)) //prevent placing water nearby town
				continue;
			
			tilesQueue.push_back(tile);
			tilesChecked.insert(tile);
		}
	}
	
	//if tile is marked as water - connect it with "big" water
	while(!tilesQueue.empty())
	{
		int3 src = tilesQueue.front();
		tilesQueue.pop_front();
		
		if(waterTiles.find(src) != waterTiles.end())
			continue;
		
		waterTiles.insert(src);

		gen->foreach_neighbour(src, [&src, &tilesDist, &tilesChecked, &tilesQueue](const int3 & dst)
		{
			if(tilesChecked.find(dst) != tilesChecked.end())
				return;
			
			if(tilesDist[dst] > 0 && tilesDist[src]-tilesDist[dst] == 1)
			{
				tilesQueue.push_back(dst);
				tilesChecked.insert(dst);
			}
		});
	}
	
	//start filtering of narrow places and coast atrifacts
	std::vector<int3> waterAdd;
	for(int coastId = 1; coastId <= coastIdMax; ++coastId)
	{
		for(auto& tile : coastTilesMap[coastId])
		{
			//collect neighbout water tiles
			auto collectionLambda = [&waterTiles, &coastTilesMap](const int3 & t, std::set<int3> & outCollection)
			{
				if(waterTiles.find(t)!=waterTiles.end())
				{
					coastTilesMap[0].insert(t);
					outCollection.insert(t);
				}
			};
			std::set<int3> waterCoastDirect, waterCoastDiag;
			gen->foreachDirectNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDirect)));
			gen->foreachDiagonalNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDiag)));
			int waterCoastDirectNum = waterCoastDirect.size();
			int waterCoastDiagNum = waterCoastDiag.size();
			
			//remove tiles which are mostly covered by water
			if(waterCoastDirectNum >= 3)
			{
				waterAdd.push_back(tile);
				continue;
			}
			if(waterCoastDiagNum == 4 && waterCoastDirectNum == 2)
			{
				waterAdd.push_back(tile);
				continue;
			}
			if(waterCoastDirectNum == 2 && waterCoastDiagNum >= 2)
			{
				int3 diagSum, dirSum;
				for(auto & i : waterCoastDiag)
					diagSum += i - tile;
				for(auto & i : waterCoastDirect)
					dirSum += i - tile;
				if(diagSum == int3() || dirSum == int3())
				{
					waterAdd.push_back(tile);
					continue;
				}
				if(waterCoastDiagNum == 3 && diagSum != dirSum)
				{
					waterAdd.push_back(tile);
					continue;
				}
			}
		}
	}
	for(auto & i : waterAdd)
		waterTiles.insert(i);
	
	//filtering tiny "lakes"
	for(auto& tile : coastTilesMap[0]) //now it's only coast-water tiles
	{
		if(waterTiles.find(tile) == waterTiles.end()) //for ground tiles
			continue;
		
		std::vector<int3> groundCoast;
		gen->foreachDirectNeighbour(tile, [this, &waterTiles, &groundCoast](const int3 & t)
		{
			if(waterTiles.find(t) == waterTiles.end() && tileinfo.find(t) != tileinfo.end()) //for ground tiles of same zone
			{
				groundCoast.push_back(t);
			}
		});
		
		if(groundCoast.size() >= 3)
		{
			waterTiles.erase(tile);
		}
		else
		{
			if(groundCoast.size() == 2)
			{
				if(groundCoast[0] + groundCoast[1] == int3())
				{
					waterTiles.erase(tile);
				}
			}
			else
			{
				if(!groundCoast.empty())
				{
					coastTiles.insert(tile);
				}
			}
		}
	}
	
	//do not set water on tiles belong to other zones
	vstd::erase_if(coastTiles, [&waterTiles](const int3 & tile)
	{
		return waterTiles.find(tile) == waterTiles.end();
	});
	
	//transforming waterTiles to actual water
	for(auto& tile : waterTiles)
	{
		gen->getZoneWater().second->addTile(tile);
		gen->setZoneID(tile, gen->getZoneWater().first);
		gen->setOccupied(tile, ETileType::POSSIBLE);
		tileinfo.erase(tile);
		possibleTiles.erase(tile);
	}
}

void CRmgTemplateZone::waterInitFreeTiles()
{
	std::set<int3> tilesAll(tileinfo.begin(), tileinfo.end()); //water tiles
	std::list<int3> tilesQueue; //tiles need to be processed
	std::set<int3> tilesChecked;
	
	//lambda for increasing distance of negihbour tiles
	auto lakeSearch = [this, &tilesAll, &tilesQueue](const int3 & dst)
	{
		if(tilesAll.find(dst) == tilesAll.end())
		{
			if(lakes.back().tiles.find(dst)==lakes.back().tiles.end())
			{
				//we reach land! let's store this information
				assert(gen->getZoneID(dst) != gen->getZoneWater().first);
				lakes.back().connectedZones.insert(gen->getZoneID(dst));
				lakes.back().coast.insert(dst);
				lakes.back().distance[dst] = 0;
				return;
			}
		}
		else
		{
			if(lakes.back().tiles.insert(dst).second)
			{
				tilesQueue.push_back(dst);
			}
		}
	};
	
	while(!tilesAll.empty())
	{
		//add some random tile as initial
		tilesQueue.push_back(*tilesAll.begin());
		setPos(tilesQueue.front());
		addFreePath(tilesQueue.front());
		lakes.emplace_back();
		lakes.back().tiles.insert(tilesQueue.front());
		
		//find lake
		while(!tilesQueue.empty())
		{
			int3 tile = tilesQueue.front();
			tilesQueue.pop_front();
			gen->foreachDirectNeighbour(tile, lakeSearch);
		}
		
		//fill distance map
		tilesQueue.assign(lakes.back().coast.begin(), lakes.back().coast.end());
		while(!tilesQueue.empty())
		{
			int3 src = tilesQueue.front();
			tilesQueue.pop_front();
			gen->foreachDirectNeighbour(src, [this, &src, &tilesChecked, &tilesQueue](const int3 & dst)
			{
				if(tilesChecked.find(dst) != tilesChecked.end())
					return;
				
				if(lakes.back().tiles.find(dst) != lakes.back().tiles.end())
				{
					lakes.back().distance[dst] = lakes.back().distance[src] + 1;
					tilesChecked.insert(dst);
					tilesQueue.push_back(dst);
				}
			});
		}
		
		//cleanup
		int lakeIdx = lakes.size();
		for(auto& t : lakes.back().tiles)
		{
			assert(lakeMap.find(t) == lakeMap.end());
			lakeMap[t] = lakeIdx;
			tilesAll.erase(t);
		}
	}
	
#ifdef _BETA
	{
		std::ofstream out1("lakes_id.txt");
		std::ofstream out2("lakes_map.txt");
		std::ofstream out3("lakes_dist.txt");
		int levels = gen->map->twoLevel ? 2 : 1;
		int width =  gen->map->width;
		int height = gen->map->height;
		for (int k = 0; k < levels; k++)
		{
			for(int j=0; j<height; j++)
			{
				for (int i=0; i<width; i++)
				{
					int3 tile{i,j,k};
					if(lakeMap[tile]>9)
						out1 << '#';
					else
						out1 << lakeMap[tile];
					
					bool found = false;
					for(auto& lake : lakes)
					{
						if(lake.coast.count(tile))
						{
							out2 << '@';
							out3 << lake.distance[tile];
							found = true;
						}
						else if(lake.tiles.count(tile))
						{
							out2 << '~';
							out3 << lake.distance[tile];
							found = true;
						}
					}
					if(!found)
					{
						out2 << ' ';
						out3 << ' ';
					}
				}
				out1 << std::endl;
				out2 << std::endl;
				out3 << std::endl;
			}
			out1 << std::endl;
			out2 << std::endl;
			out3 << std::endl;
		}
		out1 << std::endl;
		out2 << std::endl;
		out3 << std::endl;
	}
#endif
}

bool CRmgTemplateZone::waterKeepConnection(TRmgTemplateZoneId zoneA, TRmgTemplateZoneId zoneB)
{
	for(auto & lake : lakes)
	{
		if(lake.connectedZones.count(zoneA) && lake.connectedZones.count(zoneB))
		{
			lake.keepConnections.insert(zoneA);
			lake.keepConnections.insert(zoneB);
			return true;
		}
	}
	return false;
}

void CRmgTemplateZone::waterConnection(CRmgTemplateZone& dst)
{
	if(isUnderground() || dst.getCoastTiles().empty())
		return;
	
	//block zones are not connected by template
	for(auto& lake : lakes)
	{
		if(lake.connectedZones.count(dst.getId()))
		{
			if(!lake.keepConnections.count(dst.getId()))
			{
				for(auto & ct : lake.coast)
				{
					if(gen->getZoneID(ct) == dst.getId() && gen->isPossible(ct))
						gen->setOccupied(ct, ETileType::BLOCKED);
				}
				continue;
			}
	
			int3 coastTile(-1, -1, -1);
			int zoneTowns = dst.playerTowns.getTownCount() + dst.playerTowns.getCastleCount() +
							dst.neutralTowns.getTownCount() + dst.neutralTowns.getCastleCount();
			
			if(dst.getType() == ETemplateZoneType::PLAYER_START || dst.getType() == ETemplateZoneType::CPU_START || zoneTowns)
			{
				coastTile = dst.createShipyard(lake.tiles, gen->getConfig().shipyardGuard);
				if(!coastTile.valid())
				{
					coastTile = makeBoat(dst.getId(), lake.tiles);
				}
			}
			else
			{
				coastTile = makeBoat(dst.getId(), lake.tiles);
			}
				
			if(coastTile.valid())
			{
				if(connectPath(coastTile, true))
				{
					addFreePath(coastTile);
				}
				else
					logGlobal->error("Cannot build water route for zone %d", dst.getId());
			}
			else
				logGlobal->error("No entry from water to zone %d", dst.getId());
					
		}
	}
}

const std::set<int3>& CRmgTemplateZone::getCoastTiles() const
{
	return coastTiles;
}

bool CRmgTemplateZone::isWaterConnected(TRmgTemplateZoneId zone, const int3 & tile) const
{
	int lakeId = gen->getZoneWater().second->lakeMap.at(tile);
	if(lakeId == 0)
		return false;
	
	return  gen->getZoneWater().second->lakes.at(lakeId - 1).connectedZones.count(zone) &&
			gen->getZoneWater().second->lakes.at(lakeId - 1).keepConnections.count(zone);
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
	for(auto ti : treasureInfo)
		totalDensity += ti.density;
	const float minDistance = 10 * 10; //squared

	for(auto tile : tileinfo)
	{
		if(gen->isPossible(tile))
			possibleTiles.insert(tile);
	}
	assert (clearedTiles.size()); //this should come from zone connections

	std::vector<int3> nodes; //connect them with a grid

	if(type != ETemplateZoneType::JUNCTION)
	{
		//junction is not fractalized, has only one straight path
		//everything else remains blocked
		while(!possibleTiles.empty())
		{
			//link tiles in random order
			std::vector<int3> tilesToMakePath(possibleTiles.begin(), possibleTiles.end());
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, gen->rand);

			int3 nodeFound(-1, -1, -1);

			for(auto tileToMakePath : tilesToMakePath)
			{
				//find closest free tile
				float currentDistance = 1e10;
				int3 closestTile(-1, -1, -1);

				for(auto clearTile : clearedTiles)
				{
					float distance = static_cast<float>(tileToMakePath.dist2dSQ(clearTile));

					if(distance < currentDistance)
					{
						currentDistance = distance;
						closestTile = clearTile;
					}
					if(currentDistance <= minDistance)
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

			for(auto tileToClear : tilesToIgnore)
			{
				//these tiles are already connected, ignore them
				vstd::erase_if_present(possibleTiles, tileToClear);
			}
			if(!nodeFound.valid()) //nothing else can be done (?)
				break;
			tilesToIgnore.clear();
		}
	}

	//cut straight paths towards the center. A* is too slow for that.
	for (auto node : nodes)
	{
		auto subnodes = nodes;
		boost::sort(subnodes, [&node](const int3& ourNode, const int3& otherNode) -> bool
		{
			return node.dist2dSQ(ourNode) < node.dist2dSQ(otherNode);
		});

		std::vector <int3> nearbyNodes;
		if (subnodes.size() >= 2)
		{
			nearbyNodes.push_back(subnodes[1]); //node[0] is our node we want to connect
		}
		if (subnodes.size() >= 3)
		{
			nearbyNodes.push_back(subnodes[2]);
		}

		//connect with all the paths
		crunchPath(node, findClosestTile(freePaths, node), true, &freePaths);
		//connect with nearby nodes
		for (auto nearbyNode : nearbyNodes)
		{
			crunchPath(node, nearbyNode, true, &freePaths); //do not allow to make another path network
		}
	}
	for (auto node : nodes)
		gen->setOccupied(node, ETileType::FREE); //make sure they are clear

	//now block most distant tiles away from passages

	float blockDistance = minDistance * 0.25f;

	for (auto tile : tileinfo)
	{
		if(!gen->isPossible(tile))
			continue;
		
		if(freePaths.count(tile))
			continue;

		bool closeTileFound = false;

		for(auto clearTile : freePaths)
		{
			float distance = static_cast<float>(tile.dist2dSQ(clearTile));

			if(distance < blockDistance)
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
		std::ofstream out(boost::to_string(boost::format("zone_%d.txt") % id));
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
	for (const int3 & node : tilesToConnectLater)
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
	float distance = static_cast<float>(currentPos.dist2dSQ (dst));

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
								distance = static_cast<float>(currentPos.dist2dSQ (dst));
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
							lastDistance = static_cast<float>(currentPos.dist2dSQ(dst));
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
boost::heap::priority_queue<CRmgTemplateZone::TDistance, boost::heap::compare<CRmgTemplateZone::NodeComparer>> CRmgTemplateZone::createPriorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}

bool CRmgTemplateZone::createRoad(const int3& src, const int3& dst)
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	auto pq = createPriorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
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
				gen->foreachDiagonalNeighbour(currentNode, foo);
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
	auto open = createPriorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
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

				int distance = static_cast<int>(distances[currentNode]) + 1;
				int bestDistanceSoFar = std::numeric_limits<int>::max();
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = static_cast<int>(it->second);

				if (distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, (float)distance));
					distances[pos] = static_cast<float>(distance);
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
		if(gen->isPossible(tile))
			gen->setOccupied (tile, ETileType::BLOCKED);
		vstd::erase_if_present(possibleTiles, tile);
	}
	return false;
}

bool CRmgTemplateZone::connectWithCenter(const int3& src, bool onlyStraight, bool passThroughBlocked)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm

	std::set<int3> closed;    // The set of nodes already evaluated.
	auto open = createPriorityQueue(); // The set of tentative nodes to be evaluated, initially containing the start node
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
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances, passThroughBlocked](int3& pos) -> void
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
				else if(passThroughBlocked && gen->shouldBeBlocked(pos))
					movementCost = 3;
				else
					return;

				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = std::numeric_limits<int>::max(); //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = static_cast<int>(it->second);

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
void CRmgTemplateZone::addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget)
{
	nearbyObjects.push_back(std::make_pair(obj, nearbyTarget));
}
void CRmgTemplateZone::addObjectAtPosition(CGObjectInstance * obj, const int3 & position, si32 strength)
{
	//TODO: use strength
	instantObjects.push_back(std::make_pair(obj, position));
}

void CRmgTemplateZone::addToConnectLater(const int3& src)
{
	tilesToConnectLater.insert(src);
}

bool CRmgTemplateZone::addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426

	int mapMonsterStrength = gen->getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};

	int strength1 = static_cast<int>(std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]));
	int strength2 = static_cast<int>(std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]));

	strength = strength1 + strength2;
	if (strength < gen->getConfig().minGuardStrength)
		return false; //no guard at all

	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for (auto cre : VLC->creh->objects)
	{
		if (cre->special)
			continue;
		if (!cre->AIValue) //bug #2681
			continue;
		if (!vstd::contains(monsterTypes, cre->faction))
			continue;
		if (((si32)(cre->AIValue * (cre->ammMin + cre->ammMax) / 2) < strength) && (strength < (si32)cre->AIValue * 100)) //at least one full monster. size between average size of given stack and 100
		{
			possibleCreatures.push_back(cre->idNumber);
		}
	}
	if (possibleCreatures.size())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, gen->rand);
		amount = strength / VLC->creh->objects[creId]->AIValue;
		if (amount >= 4)
			amount = static_cast<int>(amount * gen->rand.nextDouble(0.75, 1.25));
	}
	else //just pick any available creature
	{
		creId = CreatureID(132); //Azure Dragon
		amount = strength / VLC->creh->objects[creId]->AIValue;
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
	while (currentValue <= (int)desiredValue - 100) //no objects with value below 100 are available
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
			object->appearance = oi.templ;

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
				if (gen->isPossible(tile) && gen->getZoneID(tile) == getId()) //we can place new treasure only on possible tile
				{
					bool here = true;
					gen->foreach_neighbour (tile, [this, &here, minDistance](int3 pos)
					{
						if (!(gen->isBlocked(pos) || gen->isPossible(pos)) || gen->getZoneID(pos) != getId() || gen->getNearestObjectDistance(pos) < minDistance)
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
				minTreasureDistance = static_cast<float>(closestFreeTile.dist2d(visitablePos));
			}
		}
		for (auto visitablePos : info.visitableFromTopPositions) //all objects are accessible from any direction
		{
			int3 closestFreeTile = findClosestTile(freePaths, visitablePos);
			if (closestFreeTile.dist2d(visitablePos) < minTreasureDistance)
			{
				closestTile = visitablePos;
				minTreasureDistance = static_cast<float>(closestFreeTile.dist2d(visitablePos));
			}
		}
		assert (closestTile.valid());

		for (auto tile : info.occupiedPositions)
		{
			if (gen->map->isInTheMap(tile) && gen->isPossible(tile) && gen->getZoneID(tile)==id) //pile boundary may reach map border
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

		bool isPileGuarded = isGuardNeededForTreasure(currentValue);

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
				placeObject(treasure.second, treasure.first + visitableOffset);
			}
			if (isPileGuarded && addMonster(guardPos, currentValue, false))
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
		if(gen->isPossible(pos))
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
		auto clearPos = [this](const int3 & pos)
		{
			if (gen->isPossible(pos))
				gen->setOccupied(pos, ETileType::FREE);
		};
		for (auto blockedTile : town->getBlockedPos())
		{
			gen->foreach_neighbour(blockedTile, clearPos);
		}
		//clear town entry
		gen->foreach_neighbour(town->visitablePos()+int3{0,1,0}, clearPos);
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
				//FIXME: discovered bug with small zones - getPos is close to map boarder and we have outOfMap exception
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
			townType = gen->getMapGenOptions().getPlayersSettings().find(player)->second.getStartingTown();

			if (townType == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				randomizeTownType(true);
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

void CRmgTemplateZone::randomizeTownType(bool matchUndergroundType)
{
	auto townTypesAllowed = (townTypes.size() ? townTypes : getDefaultTownTypes());
	if(matchUndergroundType && gen->getMapGenOptions().getHasTwoLevels())
	{
		std::set<TFaction> townTypesVerify;
		for(TFaction factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*VLC->townh)[factionIdx]->preferUndergroundPlacement;
			if(isUnderground() ? preferUnderground : !preferUnderground)
			{
				townTypesVerify.insert(factionIdx);
			}
		}
		if(!townTypesVerify.empty())
			townTypesAllowed = townTypesVerify;
	}
	
	townType = *RandomGeneratorUtil::nextItem(townTypesAllowed, gen->rand);
}

void CRmgTemplateZone::initTerrainType ()
{
	if (type==ETemplateZoneType::WATER)
	{
		terrainType = ETerrainType::WATER;
	}
	else
	{
		if (matchTerrainToTown && townType != ETownType::NEUTRAL)
			terrainType = (*VLC->townh)[townType]->nativeTerrain;
		else
			terrainType = *RandomGeneratorUtil::nextItem(terrainTypes, gen->rand);

		//TODO: allow new types of terrain?
		{
			if(isUnderground())
			{
				if(!vstd::contains(gen->getConfig().terrainUndergroundAllowed, terrainType))
					terrainType = ETerrainType::SUBTERRANEAN;
			}
			else
			{
				if(vstd::contains(gen->getConfig().terrainGroundProhibit, terrainType))
					terrainType = ETerrainType::DIRT;
			}
		}
	}
	paintZoneTerrain (terrainType);
}

void CRmgTemplateZone::paintZoneTerrain (ETerrainType terrainType)
{
	std::vector<int3> tiles(tileinfo.begin(), tileinfo.end());
	gen->getEditManager()->getTerrainSelection().setSelection(tiles);
	gen->getEditManager()->drawTerrain(terrainType, &gen->rand);
}

bool CRmgTemplateZone::placeMines ()
{
	using namespace Res;
	std::vector<CGMine*> createdMines;
	
	for(const auto & mineInfo : mines)
	{
		ERes res = (ERes)mineInfo.first;
		for(int i = 0; i < mineInfo.second; ++i)
		{
			auto mine = (CGMine*) VLC->objtypeh->getHandlerFor(Obj::MINE, res)->create(ObjectTemplate());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			createdMines.push_back(mine);
			
			if(!i && (res == ERes::WOOD || res == ERes::ORE))
				addCloseObject(mine, gen->getConfig().mineValues.at(res)); //only first wood&ore mines are close
			else
				addRequiredObject(mine, gen->getConfig().mineValues.at(res));
		}
	}
	
	//create extra resources
	if(int extraRes = gen->getConfig().mineExtraResources)
	{
		for(auto * mine : createdMines)
		{
			for(int rc = gen->rand.nextInt(1, extraRes); rc > 0; --rc)
			{
				auto resourse = (CGResource*) VLC->objtypeh->getHandlerFor(Obj::RESOURCE, mine->producedResource)->create(ObjectTemplate());
				resourse->amount = CGResource::RANDOM_AMOUNT;
				addNearbyObject(resourse, mine);
			}
		}
	}
		

	return true;
}

EObjectPlacingResult::EObjectPlacingResult CRmgTemplateZone::tryToPlaceObjectAndConnectToPath(CGObjectInstance * obj, const int3 & pos)
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
		bool attempt = true;
		while (!finished && attempt)
		{
			attempt = false;

			std::vector<int3> tiles(possibleTiles.begin(), possibleTiles.end());
			//new tiles vector after each object has been placed, OR misplaced area has been sealed off

			boost::remove_if(tiles, [obj, this](int3 &tile)-> bool
			{
				//object must be accessible from at least one surounding tile
				return !this->isAccessibleFromSomewhere(obj.first->appearance, tile);
			});

			auto targetPosition = requestedPositions.find(obj.first)!=requestedPositions.end() ? requestedPositions[obj.first] : pos;
			// smallest distance to zone center, greatest distance to nearest object
			auto isCloser = [this, &targetPosition, &tilesBlockedByObject](const int3 & lhs, const int3 & rhs) -> bool
			{
				float lDist = std::numeric_limits<float>::max();
				float rDist = std::numeric_limits<float>::max();
				for(int3 t : tilesBlockedByObject)
				{
					t += targetPosition;
					lDist = fmin(lDist, static_cast<float>(t.dist2d(lhs)));
					rDist = fmin(rDist, static_cast<float>(t.dist2d(rhs)));
				}
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
				if(!areAllTilesAvailable(obj.first, tile, tilesBlockedByObject))
					continue;

				attempt = true;

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

	//create nearby objects (e.g. extra resources close to mines)
	for(const auto & object : nearbyObjects)
	{
		auto obj = object.first;
		std::set<int3> possiblePositions;
		for (auto blockedTile : object.second->getBlockedPos())
		{
			gen->foreachDirectNeighbour(blockedTile, [this, &possiblePositions](int3 pos)
			{
				if (!gen->isBlocked(pos) && tileinfo.count(pos))
				{
					//some resources still could be unaccessible, at least one free cell shall be
					gen->foreach_neighbour(pos, [this, &possiblePositions, &pos](int3 p)
												{
						if(gen->isFree(p))
							possiblePositions.insert(pos);
					});
				}
			});
		}

		if(possiblePositions.empty())
		{
			delete obj; //is it correct way to prevent leak?
		}
		else
		{
			auto pos = *RandomGeneratorUtil::nextItem(possiblePositions, gen->rand);
			placeObject(obj, pos);
		}
	}
	
	//create object on specific positions
	//TODO: implement guards
	for (const auto &obj : instantObjects)
	{
		if(tryToPlaceObjectAndConnectToPath(obj.first, obj.second)==EObjectPlacingResult::SUCCESS)
		{
			placeObject(obj.first, obj.second);
			//TODO: guardObject(...)
		}
	}

	requiredObjects.clear();
	closeObjects.clear();
	nearbyObjects.clear();
	instantObjects.clear();

	return true;
}

int3 CRmgTemplateZone::makeBoat(TRmgTemplateZoneId land, const std::set<int3> & lake)
{
	std::set<int3> lakeCoast;
	std::set_intersection(gen->getZones()[land]->getCoastTiles().begin(), gen->getZones()[land]->getCoastTiles().end(), lake.begin(), lake.end(), std::inserter(lakeCoast, lakeCoast.begin()));
	for(int randomAttempts = 0; randomAttempts<5; ++randomAttempts)
	{
		auto coastTile = *RandomGeneratorUtil::nextItem(lakeCoast, gen->rand);
		if(gen->getZoneID(coastTile) == gen->getZoneWater().first && isWaterConnected(land, coastTile) && makeBoat(land, coastTile))
			return coastTile;
	}
	//if no success on random selection, use brute force
	for(const auto& coastTile : lakeCoast)
	{
		if(gen->getZoneID(coastTile) == gen->getZoneWater().first && isWaterConnected(land, coastTile) && makeBoat(land, coastTile))
			return coastTile;
	}
	return int3(-1,-1,-1);
}

bool CRmgTemplateZone::makeBoat(TRmgTemplateZoneId land, const int3 & coast)
{
	//verify coast
	if(gen->getZoneWater().first != id)
		throw rmgException("Cannot make a ship: not a water zone");
	if(gen->getZoneID(coast) != id)
		throw rmgException("Cannot make a ship: coast tile doesn't belong to water");
	
	//find zone for ship boarding
	std::vector<int3> landTiles;
	gen->foreach_neighbour(coast, [this, &landTiles, land](const int3 & t)
	{
		if(land == gen->getZoneID(t) && gen->isPossible(t))
		{
			landTiles.push_back(t);
		}
	});
	
	if(landTiles.empty())
		return false;
	
	int3 landTile = {-1, -1, -1};
	for(auto& lt : landTiles)
	{
		if(gen->getZones()[land]->connectPath(lt, false))
		{
			landTile = lt;
			gen->setOccupied(landTile, ETileType::FREE);
			break;
		}
	}
	
	if(!landTile.valid())
		return false;
	
	auto subObjects = VLC->objtypeh->knownSubObjects(Obj::BOAT);
	auto* boat = (CGBoat*)VLC->objtypeh->getHandlerFor(Obj::BOAT, *RandomGeneratorUtil::nextItem(subObjects, gen->rand))->create(ObjectTemplate());
	
	auto targetPos = boat->getVisitableOffset() + coast + int3{1, 0, 0}; //+1 offset for boat - bug?
	if (gen->map->isInTheMap(targetPos) && gen->isPossible(targetPos) && gen->getZoneID(targetPos) == getId())
	{
		//don't connect to path because it's not initialized
		addObjectAtPosition(boat, targetPos);
		gen->setOccupied(targetPos, ETileType::USED);
		return true;
	}
	
	return false;
}

int3 CRmgTemplateZone::createShipyard(const std::set<int3> & lake, si32 guardStrength)
{
	std::set<int3> lakeCoast;
	std::set_intersection(getCoastTiles().begin(), getCoastTiles().end(), lake.begin(), lake.end(), std::inserter(lakeCoast, lakeCoast.begin()));
	for(int randomAttempts = 0; randomAttempts < 5; ++randomAttempts)
	{
		auto coastTile = *RandomGeneratorUtil::nextItem(lakeCoast, gen->rand);
		if(gen->getZoneID(coastTile) == gen->getZoneWater().first && isWaterConnected(id, coastTile) && createShipyard(coastTile, guardStrength))
			return coastTile;
	}
	//if no success on random selection, use brute force
	for(const auto& coastTile : lakeCoast)
	{
		if(gen->getZoneID(coastTile) == gen->getZoneWater().first && isWaterConnected(id, coastTile) && createShipyard(coastTile, guardStrength))
			return coastTile;
	}
	return int3(-1,-1,-1);
}

bool CRmgTemplateZone::createShipyard(const int3 & position, si32 guardStrength)
{
	auto subObjects = VLC->objtypeh->knownSubObjects(Obj::SHIPYARD);
	auto shipyard = (CGShipyard*) VLC->objtypeh->getHandlerFor(Obj::SHIPYARD, *RandomGeneratorUtil::nextItem(subObjects, gen->rand))->create(ObjectTemplate());
	shipyard->tempOwner = PlayerColor::NEUTRAL;
	
	setTemplateForObject(shipyard);
	std::vector<int3> outOffsets;
	auto tilesBlockedByObject = shipyard->getBlockedOffsets();
	tilesBlockedByObject.insert(shipyard->getVisitableOffset());
	shipyard->getOutOffsets(outOffsets);
	
	int3 targetTile(-1, -1, -1);
	std::set<int3> shipAccessCandidates;
	
	for(const auto & outOffset : outOffsets)
	{
		auto candidateTile = position - outOffset;
		std::set<int3> tilesBlockedAbsolute;
		
		//check space under object
		bool allClear = true;
		for(const auto & objectTileOffset : tilesBlockedByObject)
		{
			auto objectTile = candidateTile + objectTileOffset;
			tilesBlockedAbsolute.insert(objectTile);
			if(!gen->map->isInTheMap(objectTile) || !gen->isPossible(objectTile) || gen->getZoneID(objectTile)!=id)
			{
				allClear = false;
				break;
			}
		}
		if(!allClear) //cannot place shipyard anyway
			continue;
		
		//prepare temporary map
		for(auto& blockedPos : tilesBlockedAbsolute)
			gen->setOccupied(blockedPos, ETileType::USED);
		
		
		//check if boarding position is accessible
		gen->foreach_neighbour(position, [this, &shipAccessCandidates](const int3 & v)
		{
			if(!gen->isBlocked(v) && gen->getZoneID(v)==id)
			{
				//make sure that it's possible to create path to boarding position
				if(connectWithCenter(v, false, false))
					shipAccessCandidates.insert(v);
			}
		});
		
		//check if we can connect shipyard entrance with path
		if(!connectWithCenter(candidateTile + shipyard->getVisitableOffset(), false))
			shipAccessCandidates.clear();
				
		//rollback temporary map
		for(auto& blockedPos : tilesBlockedAbsolute)
			gen->setOccupied(blockedPos, ETileType::POSSIBLE);
		
		if(!shipAccessCandidates.empty() && isAccessibleFromSomewhere(shipyard->appearance, candidateTile))
		{
			targetTile = candidateTile;
			break; //no need to check other offsets as we already found position
		}
		
		shipAccessCandidates.clear(); //invalidate positions
	}
	
	if(!targetTile.valid())
	{
		delete shipyard;
		return false;
	}
	
	if(tryToPlaceObjectAndConnectToPath(shipyard, targetTile)==EObjectPlacingResult::SUCCESS)
	{
		placeObject(shipyard, targetTile);
		guardObject(shipyard, guardStrength, false, true);
	
		for(auto& accessPosition : shipAccessCandidates)
		{
			if(connectPath(accessPosition, false))
			{
				gen->setOccupied(accessPosition, ETileType::FREE);
				return true;
			}
		}
	}
	
	logGlobal->warn("Cannot find path to shipyard boarding position");
	delete shipyard;
	return false;
}

void CRmgTemplateZone::createTreasures()
{
	int mapMonsterStrength = gen->getMapGenOptions().getMonsterStrength();
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
		const float minDistance = std::max<float>((125.f / totalDensity), 2.0f);
		//distance lower than 2 causes objects to overlap and crash

		bool stop = false;
		do {
			//optimization - don't check tiles which are not allowed
			vstd::erase_if(possibleTiles, [this](const int3 &tile) -> bool
			{
				//for water area we sholdn't place treasures close to coast
				for(auto & lake : lakes)
					if(vstd::contains(lake.distance, tile) && lake.distance[tile] < 2)
						return true;
				
				return !gen->isPossible(tile) || gen->getZoneID(tile)!=getId();
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
		gen->getEditManager()->getTerrainSelection().setSelection(accessibleTiles);
		gen->getEditManager()->drawTerrain(terrainType, &gen->rand);
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
						obstaclesBySize[(ui8)temp.getBlockedOffsets().size()].push_back(temp);
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

	auto sel = gen->getEditManager()->getTerrainSelection();
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
		//fill tiles that should be blocked with obstacles
		if (gen->shouldBeBlocked(tile))
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

	gen->getEditManager()->getTerrainSelection().setSelection(tiles);
	gen->getEditManager()->drawRoad(gen->getConfig().defaultRoadType, &gen->rand);
}


bool CRmgTemplateZone::fill()
{
	initTerrainType();
	
	addAllPossibleObjects();
	
	//zone center should be always clear to allow other tiles to connect
	initFreeTiles();
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

	bool needsGuard = isGuardNeededForTreasure(value);

	//logGlobal->info("Min dist for density %f is %d", density, min_dist);
	for(auto tile : possibleTiles)
	{
		auto dist = gen->getNearestObjectDistance(tile);

		if ((dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			gen->foreach_neighbour (tile, [this, &allTilesAvailable, needsGuard](int3 neighbour)
			{
				if (!(gen->isPossible(neighbour) || gen->shouldBeBlocked(neighbour) || gen->getZoneID(neighbour)==getId() || (!needsGuard && gen->isFree(neighbour))))
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
		if (!gen->map->isInTheMap(t) || !(gen->isPossible(t) || gen->shouldBeBlocked(t)) || !temp.canBePlacedAt(gen->map->getTile(t).terType))
		{
			return false; //if at least one tile is not possible, object can't be placed here
		}
	}
	return true;
}

bool CRmgTemplateZone::isAccessibleFromSomewhere(ObjectTemplate & appearance, const int3 & tile) const
{
	return getAccessibleOffset(appearance, tile).valid();
}

int3 CRmgTemplateZone::getAccessibleOffset(ObjectTemplate & appearance, const int3 & tile) const
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
						if (appearance.isVisitableFrom(x, y) && !gen->isBlocked(nearbyPos) && tileinfo.find(nearbyPos) != tileinfo.end())
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

bool CRmgTemplateZone::areAllTilesAvailable(CGObjectInstance* obj, int3& tile, const std::set<int3>& tilesBlockedByObject) const
{
	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = tile + blockingTile;
		if (!gen->map->isInTheMap(t) || !gen->isPossible(t) || gen->getZoneID(t)!=getId())
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
		if (!isAccessibleFromSomewhere(obj->appearance, tile))
			continue;

		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			if (areAllTilesAvailable(obj, tile, tilesBlockedByObject))
			{
				best_distance = static_cast<int>(dist);
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

	gen->getEditManager()->insertObject(object);
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
	case Obj::SHIPYARD:
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
		gen->setNearestObjectDistance(tile, std::min((float)d, gen->getNearestObjectDistance(tile)));
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

bool CRmgTemplateZone::isGuardNeededForTreasure(int value)
{
	return getType() != ETemplateZoneType::WATER && value > minGuardedValue;
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
			if (gen->isPossible(pos) && gen->getZoneID(pos) == id)
				gen->setOccupied(pos, ETileType::BLOCKED);
		}
		gen->foreach_neighbour (guardTile, [&](int3& pos)
		{
			if (gen->isPossible(pos) && gen->getZoneID(pos) == id)
				gen->setOccupied(pos, ETileType::FREE);
		});

		gen->setOccupied (guardTile, ETileType::USED);
	}
	else //allow no guard or other object in front of this object
	{
		for (auto tile : tiles)
			if (gen->isPossible(tile))
				gen->setOccupied(tile, ETileType::FREE);
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
	ui32 minValue = static_cast<ui32>(0.25f * (desiredValue - currentValue));

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
				if (!isAccessibleFromSomewhere(oi.templ, newVisitablePos))
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

	if(thresholds.empty())
	{
		ObjectInfo oi;
		//Generate pandora Box with gold if the value is extremely high
		if(minValue > gen->getConfig().treasureValueLimit) //we don't have object valuable enough
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
			return (int)rhs.first < lhs;
		});
		return *(it->second);
	}
}

void CRmgTemplateZone::addAllPossibleObjects()
{
	ObjectInfo oi;

	int numZones = static_cast<int>(gen->getZones().size());

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
	
	if(type == ETemplateZoneType::WATER)
		return;

	//prisons
	//levels 1, 5, 10, 20, 30
	static int prisonsLevels = std::min(gen->getConfig().prisonExperience.size(), gen->getConfig().prisonValues.size());
	for(int i = 0; i < prisonsLevels; i++)
	{
		oi.generateObject = [i, this]() -> CGObjectInstance *
		{
			std::vector<ui32> possibleHeroes;
			for(int j = 0; j < gen->map->allowedHeroes.size(); j++)
			{
				if(gen->map->allowedHeroes[j])
					possibleHeroes.push_back(j);
			}

			auto hid = *RandomGeneratorUtil::nextItem(possibleHeroes, gen->rand);
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0);
			auto obj = (CGHeroInstance *) factory->create(ObjectTemplate());


			obj->subID = hid; //will be initialized later
			obj->exp = gen->getConfig().prisonExperience[i];
			obj->setOwner(PlayerColor::NEUTRAL);
			gen->map->allowedHeroes[hid] = false; //ban this hero
			gen->decreasePrisonsRemaining();
			obj->appearance = VLC->objtypeh->getHandlerFor(Obj::PRISON, 0)->getTemplates(terrainType).front(); //can't init template with hero subID

			return obj;
		};
		oi.setTemplate(Obj::PRISON, 0, terrainType);
		oi.value = gen->getConfig().prisonValues[i];
		oi.probability = 30;
		oi.maxPerZone = gen->getPrisonsRemaning() / 5; //probably not perfect, but we can't generate more prisons than hereos.
		possibleObjects.push_back(oi);
	}

	//all following objects are unlimited
	oi.maxPerZone = std::numeric_limits<ui32>().max();
	
	std::vector<CCreature *> creatures; //native creatures for this zone
	for (auto cre : VLC->creh->objects)
	{
		if (!cre->special && cre->faction == townType)
		{
			creatures.push_back(cre);
		}
	}

	//dwellings
	auto dwellingTypes = {Obj::CREATURE_GENERATOR1, Obj::CREATURE_GENERATOR4};

	for(auto dwellingType : dwellingTypes)
	{
		auto subObjects = VLC->objtypeh->knownSubObjects(dwellingType);

		if(dwellingType == Obj::CREATURE_GENERATOR1)
		{
			//don't spawn original "neutral" dwellings that got replaced by Conflux dwellings in AB
			static int elementalConfluxROE[] = {7, 13, 16, 47};
			for(int i = 0; i < 4; i++)
				vstd::erase_if_present(subObjects, elementalConfluxROE[i]);
		}

		for(auto secondaryID : subObjects)
		{
			auto dwellingHandler = dynamic_cast<const CDwellingInstanceConstructor *>(VLC->objtypeh->getHandlerFor(dwellingType, secondaryID).get());
			auto creatures = dwellingHandler->getProducedCreatures();
			if(creatures.empty())
				continue;

			auto cre = creatures.front();
			if(cre->faction == townType)
			{
				float nativeZonesCount = static_cast<float>(gen->getZoneCount(cre->faction));
				oi.value = static_cast<ui32>(cre->AIValue * cre->growth * (1 + (nativeZonesCount / gen->getTotalZoneCount()) + (nativeZonesCount / 2)));
				oi.probability = 40;

				for(auto tmplate : dwellingHandler->getTemplates())
				{
					if(tmplate.canBePlacedAt(terrainType))
					{
						oi.generateObject = [tmplate, secondaryID, dwellingType]() -> CGObjectInstance *
						{
							auto obj = VLC->objtypeh->getHandlerFor(dwellingType, secondaryID)->create(tmplate);
							obj->tempOwner = PlayerColor::NEUTRAL;
							return obj;
						};

						oi.templ = tmplate;
						possibleObjects.push_back(oi);
					}
				}
			}
		}
	}

	for(int i = 0; i < gen->getConfig().scrollValues.size(); i++)
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
		oi.value = gen->getConfig().scrollValues[i];
		oi.probability = 30;
		possibleObjects.push_back(oi);
	}

	//pandora box with gold
	for(int i = 1; i < 5; i++)
	{
		oi.generateObject = [i]() -> CGObjectInstance *
		{
			auto factory = VLC->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
			auto obj = (CGPandoraBox *) factory->create(ObjectTemplate());
			obj->resources[Res::GOLD] = i * 5000;
			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = i * gen->getConfig().pandoraMultiplierGold;
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
		oi.value = i * gen->getConfig().pandoraMultiplierExperience;
		oi.probability = 20;
		possibleObjects.push_back(oi);
	}

	//pandora box with creatures
	const std::vector<int> & tierValues = gen->getConfig().pandoraCreatureValues;

	auto creatureToCount = [&tierValues](CCreature * creature) -> int
	{
		if (!creature->AIValue) //bug #2681
			return 0; //this box won't be generated

		int actualTier = creature->level > tierValues.size() ?
						 tierValues.size() - 1 :
						 creature->level - 1;
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
		return static_cast<int>(creaturesAmount);
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
		oi.value = static_cast<ui32>((2 * (creature->AIValue) * creaturesAmount * (1 + (float)(gen->getZoneCount(creature->faction)) / gen->getTotalZoneCount())) / 3);
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
			for (int j = 0; j < std::min(12, (int)spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = (i + 1) * gen->getConfig().pandoraMultiplierSpells; //5000 - 15000
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
			for (int j = 0; j < std::min(15, (int)spells.size()); j++)
			{
				obj->spells.push_back(spells[j]->id);
			}

			return obj;
		};
		oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
		oi.value = gen->getConfig().pandoraSpellSchool;
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
		for (int j = 0; j < std::min(60, (int)spells.size()); j++)
		{
			obj->spells.push_back(spells[j]->id);
		}

		return obj;
	};
	oi.setTemplate(Obj::PANDORAS_BOX, 0, terrainType);
	oi.value = gen->getConfig().pandoraSpell60;
	oi.probability = 2;
	possibleObjects.push_back(oi);

	//seer huts with creatures or generic rewards

	if(questArtZone.lock()) //we won't be placing seer huts if there is no zone left to place arties
	{
		static const int genericSeerHuts = 8;
		int seerHutsPerType = 0;
		const int questArtsRemaining = static_cast<int>(gen->getQuestArtsRemaning().size());

		//general issue is that not many artifact types are available for quests

		if (questArtsRemaining >= genericSeerHuts + (int)creatures.size())
		{
			seerHutsPerType = questArtsRemaining / (genericSeerHuts + (int)creatures.size());
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

		for(int i = 0; i < std::min((int)creatures.size(), questArtsRemaining - genericSeerHuts); i++)
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
			oi.value = static_cast<ui32>(((2 * (creature->AIValue) * creaturesAmount * (1 + (float)(gen->getZoneCount(creature->faction)) / gen->getTotalZoneCount())) - 4000) / 3);
			oi.probability = 3;
			possibleObjects.push_back(oi);
		}

		static int seerLevels = std::min(gen->getConfig().questValues.size(), gen->getConfig().questRewardValues.size());
		for(int i = 0; i < seerLevels; i++) //seems that code for exp and gold reward is similiar
		{
			int randomAppearance = *RandomGeneratorUtil::nextItem(VLC->objtypeh->knownSubObjects(Obj::SEER_HUT), gen->rand);

			oi.setTemplate(Obj::SEER_HUT, randomAppearance, terrainType);
			oi.value = gen->getConfig().questValues[i];
			oi.probability = 10;

			oi.generateObject = [i, randomAppearance, this, generateArtInfo]() -> CGObjectInstance *
			{
				auto factory = VLC->objtypeh->getHandlerFor(Obj::SEER_HUT, randomAppearance);
				auto obj = (CGSeerHut *) factory->create(ObjectTemplate());

				obj->rewardType = CGSeerHut::EXPERIENCE;
				obj->rID = 0; //unitialized?
				obj->rVal = gen->getConfig().questRewardValues[i];

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
				obj->rVal = gen->getConfig().questRewardValues[i];

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
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	auto templates = templHandler->getTemplates(terrainType);
	if(templates.empty())
		return;
	
	templ = templates.front();
}
