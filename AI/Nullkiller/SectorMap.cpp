/*
* SectorMap.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "SectorMap.h"
#include "VCAI.h"

#include "../../CCallback.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/CGameState.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

SectorMap::SectorMap()
{
	update();
}

SectorMap::SectorMap(HeroPtr h)
{
	update();
	makeParentBFS(h->visitablePos());
}

bool SectorMap::markIfBlocked(TSectorID & sec, crint3 pos, const TerrainTile * t)
{
	if (t->blocked && !t->visitable)
	{
		sec = NOT_AVAILABLE;
		return true;
	}

	return false;
}

bool SectorMap::markIfBlocked(TSectorID & sec, crint3 pos)
{
	return markIfBlocked(sec, pos, getTile(pos));
}

void SectorMap::update()
{
	visibleTiles = cb->getAllVisibleTiles();
	auto shape = visibleTiles->shape();
	sector.resize(boost::extents[shape[0]][shape[1]][shape[2]]);

	clear();
	int curSector = 3; //0 is invisible, 1 is not explored

	CCallback * cbp = cb.get(); //optimization
	foreach_tile_pos([&](crint3 pos)
	{
		if (retrieveTile(pos) == NOT_CHECKED)
		{
			if (!markIfBlocked(retrieveTile(pos), pos))
				exploreNewSector(pos, curSector++, cbp);
		}
	});
	valid = true;
}

SectorMap::TSectorID & SectorMap::retrieveTileN(SectorMap::TSectorArray & a, const int3 & pos)
{
	return a[pos.x][pos.y][pos.z];
}

const SectorMap::TSectorID & SectorMap::retrieveTileN(const SectorMap::TSectorArray & a, const int3 & pos)
{
	return a[pos.x][pos.y][pos.z];
}

void SectorMap::clear()
{
	//TODO: rotate to [z][x][y]
	auto fow = cb->getVisibilityMap();
	//TODO: any magic to automate this? will need array->array conversion
	//std::transform(fow.begin(), fow.end(), sector.begin(), [](const ui8 &f) -> unsigned short
	//{
	//	return f; //type conversion
	//});
	auto width = fow.size();
	auto height = fow.front().size();
	auto depth = fow.front().front().size();
	for (size_t x = 0; x < width; x++)
	{
		for (size_t y = 0; y < height; y++)
		{
			for (size_t z = 0; z < depth; z++)
				sector[x][y][z] = fow[x][y][z];
		}
	}
	valid = false;
}

void SectorMap::exploreNewSector(crint3 pos, int num, CCallback * cbp)
{
	Sector & s = infoOnSectors[num];
	s.id = num;
	s.water = getTile(pos)->isWater();

	std::queue<int3> toVisit;
	toVisit.push(pos);
	while (!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		TSectorID & sec = retrieveTile(curPos);
		if (sec == NOT_CHECKED)
		{
			const TerrainTile * t = getTile(curPos);
			if (!markIfBlocked(sec, curPos, t))
			{
				if (t->isWater() == s.water) //sector is only-water or only-land
				{
					sec = num;
					s.tiles.push_back(curPos);
					foreach_neighbour(cbp, curPos, [&](CCallback * cbp, crint3 neighPos)
					{
						if (retrieveTile(neighPos) == NOT_CHECKED)
						{
							toVisit.push(neighPos);
							//parent[neighPos] = curPos;
						}
						const TerrainTile * nt = getTile(neighPos);
						if (nt && nt->isWater() != s.water && canBeEmbarkmentPoint(nt, s.water))
						{
							s.embarkmentPoints.push_back(neighPos);
						}
					});

					if (t->visitable)
					{
						auto obj = t->visitableObjects.front();
						if (cb->getObj(obj->id, false)) // FIXME: we have to filter invisible objcts like events, but probably TerrainTile shouldn't be used in SectorMap at all
							s.visitableObjs.push_back(obj);
					}
				}
			}
		}
	}

	vstd::removeDuplicates(s.embarkmentPoints);
}

void SectorMap::write(crstring fname)
{
	std::ofstream out(fname);
	for (int k = 0; k < cb->getMapSize().z; k++)
	{
		for (int j = 0; j < cb->getMapSize().y; j++)
		{
			for (int i = 0; i < cb->getMapSize().x; i++)
			{
				out << (int)sector[i][j][k] << '\t';
			}
			out << std::endl;
		}
		out << std::endl;
	}
}

/*
this functions returns one target tile or invalid tile. We will use it to poll possible destinations
For ship construction etc, another function (goal?) is needed
*/
int3 SectorMap::firstTileToGet(HeroPtr h, crint3 dst)
{
	int3 ret(-1, -1, -1);

	int sourceSector = retrieveTile(h->visitablePos());
	int destinationSector = retrieveTile(dst);

	const Sector * src = &infoOnSectors[sourceSector];
	const Sector * dest = &infoOnSectors[destinationSector];

	if (sourceSector != destinationSector) //use ships, shipyards etc..
	{
		if (ai->isAccessibleForHero(dst, h)) //pathfinder can find a way using ships and gates if tile is not blocked by objects
			return dst;

		std::map<const Sector *, const Sector *> preds;
		std::queue<const Sector *> sectorQueue;
		sectorQueue.push(src);
		while (!sectorQueue.empty())
		{
			const Sector * s = sectorQueue.front();
			sectorQueue.pop();

			for (int3 ep : s->embarkmentPoints)
			{
				Sector * neigh = &infoOnSectors[retrieveTile(ep)];
				//preds[s].push_back(neigh);
				if (!preds[neigh])
				{
					preds[neigh] = s;
					sectorQueue.push(neigh);
				}
			}
		}

		if (!preds[dest])
		{
			//write("test.txt");

			return ret;
			//throw cannotFulfillGoalException(boost::str(boost::format("Cannot find connection between sectors %d and %d") % src->id % dst->id));
		}

		std::vector<const Sector *> toTraverse;
		toTraverse.push_back(dest);
		while (toTraverse.back() != src)
		{
			toTraverse.push_back(preds[toTraverse.back()]);
		}

		if (preds[dest])
		{
			//TODO: would be nice to find sectors in loop
			const Sector * sectorToReach = toTraverse.at(toTraverse.size() - 2);

			if (!src->water && sectorToReach->water) //embark
			{
				//embark on ship -> look for an EP with a boat
				auto firstEP = boost::find_if(src->embarkmentPoints, [=](crint3 pos) -> bool
				{
					const TerrainTile * t = getTile(pos);
					if (t && t->visitableObjects.size() == 1 && t->topVisitableId() == Obj::BOAT)
					{
						if (retrieveTile(pos) == sectorToReach->id)
							return true;
					}
					return false;
				});

				if (firstEP != src->embarkmentPoints.end())
				{
					return *firstEP;
				}
				else
				{
					//we need to find a shipyard with an access to the desired sector's EP
					//TODO what about Summon Boat spell?
					std::vector<const IShipyard *> shipyards;
					for (const CGTownInstance * t : cb->getTownsInfo())
					{
						if (t->hasBuilt(BuildingID::SHIPYARD))
							shipyards.push_back(t);
					}

					for (const CGObjectInstance * obj : ai->getFlaggedObjects())
					{
						if (obj->ID != Obj::TOWN) //towns were handled in the previous loop
						{
							if (const IShipyard * shipyard = IShipyard::castFrom(obj))
								shipyards.push_back(shipyard);
						}
					}

					shipyards.erase(boost::remove_if(shipyards, [=](const IShipyard * shipyard) -> bool
					{
						return shipyard->shipyardStatus() != 0 || retrieveTile(shipyard->bestLocation()) != sectorToReach->id;
					}), shipyards.end());

					if (!shipyards.size())
					{
						//TODO consider possibility of building shipyard in a town
						return ret;

						//throw cannotFulfillGoalException("There is no known shipyard!");
					}

					//we have only shipyards that possibly can build ships onto the appropriate EP
					auto ownedGoodShipyard = boost::find_if(shipyards, [](const IShipyard * s) -> bool
					{
						return s->o->tempOwner == ai->playerID;
					});

					if (ownedGoodShipyard != shipyards.end())
					{
						const IShipyard * s = *ownedGoodShipyard;
						TResources shipCost;
						s->getBoatCost(shipCost);
						if (cb->getResourceAmount().canAfford(shipCost))
						{
							int3 ret = s->bestLocation();
							cb->buildBoat(s); //TODO: move actions elsewhere
							return ret;
						}
						else
						{
							//TODO gather res
							return ret;

							//throw cannotFulfillGoalException("Not enough resources to build a boat");
						}
					}
					else
					{
						//TODO pick best shipyard to take over
						return shipyards.front()->o->visitablePos();
					}
				}
			}
			else if (src->water && !sectorToReach->water)
			{
				//TODO
				//disembark
				return ret;
			}
			else //use subterranean gates - not needed since gates are now handled via Pathfinder
			{
				return ret;
				//throw cannotFulfillGoalException("Land-land and water-water inter-sector transitions are not implemented!");
			}
		}
		else
		{
			return ret;
			//throw cannotFulfillGoalException("Inter-sector route detection failed: not connected sectors?");
		}
	}
	else //tiles are in same sector
	{
		return findFirstVisitableTile(h, dst);
	}
}

int3 SectorMap::findFirstVisitableTile(HeroPtr h, crint3 dst)
{
	int3 ret(-1, -1, -1);
	int3 curtile = dst;

	while (curtile != h->visitablePos())
	{
		auto topObj = cb->getTopObj(curtile);
		if (topObj && topObj->ID == Obj::HERO && topObj != h.h)
		{
			if (cb->getPlayerRelations(h->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
			{
				logAi->warn("Another allied hero stands in our way");
				return ret;
			}
		}
		if (ai->myCb->getPathsInfo(h.get())->getPathInfo(curtile)->reachable())
		{
			return curtile;
		}
		else
		{
			auto i = parent.find(curtile);
			if (i != parent.end())
			{
				assert(curtile != i->second);
				curtile = i->second;
			}
			else
			{
				return ret;
				//throw cannotFulfillGoalException("Unreachable tile in sector? Should not happen!");
			}
		}
	}
	return ret;
}

void SectorMap::makeParentBFS(crint3 source)
{
	parent.clear();

	int mySector = retrieveTile(source);
	std::queue<int3> toVisit;
	toVisit.push(source);
	while (!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		TSectorID & sec = retrieveTile(curPos);
		assert(sec == mySector); //consider only tiles from the same sector
		UNUSED(sec);

		foreach_neighbour(curPos, [&](crint3 neighPos)
		{
			if (retrieveTile(neighPos) == mySector && !vstd::contains(parent, neighPos))
			{
				if (cb->canMoveBetween(curPos, neighPos))
				{
					toVisit.push(neighPos);
					parent[neighPos] = curPos;
				}
			}
		});
	}
}

SectorMap::TSectorID & SectorMap::retrieveTile(crint3 pos)
{
	return retrieveTileN(sector, pos);
}

TerrainTile * SectorMap::getTile(crint3 pos) const
{
	//out of bounds access should be handled by boost::multi_array
	//still we cached this array to avoid any checks
	return visibleTiles->operator[](pos.x)[pos.y][pos.z];
}

std::vector<const CGObjectInstance *> SectorMap::getNearbyObjs(HeroPtr h, bool sectorsAround)
{
	const Sector * heroSector = &infoOnSectors[retrieveTile(h->visitablePos())];
	if (sectorsAround)
	{
		std::vector<const CGObjectInstance *> ret;
		for (auto embarkPoint : heroSector->embarkmentPoints)
		{
			const Sector * embarkSector = &infoOnSectors[retrieveTile(embarkPoint)];
			range::copy(embarkSector->visitableObjs, std::back_inserter(ret));
		}
		return ret;
	}
	return heroSector->visitableObjs;
}