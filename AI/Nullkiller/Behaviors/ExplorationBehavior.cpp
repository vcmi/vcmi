/*
* ExplorationBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExplorationBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/Invalid.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Markers/ExplorationPoint.h"
#include "CaptureObjectsBehavior.h"
#include "../Goals/CaptureObject.h"
#include "../../../lib/CPlayerState.h"

namespace NKAI
{

using namespace Goals;

struct ExplorationHelper
{
	const CGHeroInstance * hero;
	int sightRadius;
	float bestValue;
	TSubgoal bestGoal;
	int3 bestTile;
	int bestTilesDiscovered;
	const Nullkiller * ai;
	CCallback * cbp;
	const TeamState * ts;
	int3 ourPos;
	bool allowDeadEndCancellation;

	ExplorationHelper(const CGHeroInstance * hero, const Nullkiller * ai)
		:ai(ai), cbp(ai->cb.get()), hero(hero)
	{
		ts = cbp->getPlayerTeam(ai->playerID);
		sightRadius = hero->getSightRadius();
		bestGoal = sptr(Goals::Invalid());
		bestValue = 0;
		bestTilesDiscovered = 0;
		ourPos = hero->visitablePos();
		allowDeadEndCancellation = true;
	}

	TSubgoal makeComposition() const
	{
		Composition c;
		c.addNext(ExplorationPoint(bestTile, bestTilesDiscovered));
		c.addNext(bestGoal);
		return sptr(c);
	}

	void scanSector(int scanRadius)
	{
		int3 tile = int3(0, 0, ourPos.z);

		const auto & slice = (*(ts->fogOfWarMap))[ourPos.z];

		for(tile.x = ourPos.x - scanRadius; tile.x <= ourPos.x + scanRadius; tile.x++)
		{
			for(tile.y = ourPos.y - scanRadius; tile.y <= ourPos.y + scanRadius; tile.y++)
			{

				if(cbp->isInTheMap(tile) && slice[tile.x][tile.y])
				{
					scanTile(tile);
				}
			}
		}
	}

	void scanMap()
	{
		int3 mapSize = cbp->getMapSize();
		int perimeter = 2 * sightRadius * (mapSize.x + mapSize.y);

		std::vector<int3> from;
		std::vector<int3> to;

		from.reserve(perimeter);
		to.reserve(perimeter);

		foreach_tile_pos([&](const int3 & pos)
			{
				if((*(ts->fogOfWarMap))[pos.z][pos.x][pos.y])
				{
					bool hasInvisibleNeighbor = false;

					foreach_neighbour(cbp, pos, [&](CCallback * cbp, int3 neighbour)
						{
							if(!(*(ts->fogOfWarMap))[neighbour.z][neighbour.x][neighbour.y])
							{
								hasInvisibleNeighbor = true;
							}
						});

					if(hasInvisibleNeighbor)
						from.push_back(pos);
				}
			});

		logAi->debug("Exploration scan visible area perimeter for hero %s", hero->getNameTranslated());

		for(const int3 & tile : from)
		{
			scanTile(tile);
		}

		if(!bestGoal->invalid())
		{
			return;
		}

		allowDeadEndCancellation = false;

		for(int i = 0; i < sightRadius; i++)
		{
			getVisibleNeighbours(from, to);
			vstd::concatenate(from, to);
			vstd::removeDuplicates(from);
		}

		logAi->debug("Exploration scan all possible tiles for hero %s", hero->getNameTranslated());

		for(const int3 & tile : from)
		{
			scanTile(tile);
		}
	}

	void scanTile(const int3 & tile)
	{
		if(tile == ourPos
			|| !ai->pathfinder->isTileAccessible(hero, tile)) //shouldn't happen, but it does
			return;

		int tilesDiscovered = howManyTilesWillBeDiscovered(tile);
		if(!tilesDiscovered)
			return;

		auto paths = ai->pathfinder->getPathInfo(tile);
		auto waysToVisit = CaptureObjectsBehavior::getVisitGoals(paths, ai, ai->cb->getTopObj(tile));

		for(int i = 0; i != paths.size(); i++)
		{
			auto & path = paths[i];
			auto goal = waysToVisit[i];

			if(path.exchangeCount > 1 || path.targetHero != hero || path.movementCost() <= 0.0 || goal->invalid())
				continue;

			float ourValue = (float)tilesDiscovered * tilesDiscovered / path.movementCost();

			if(ourValue > bestValue) //avoid costly checks of tiles that don't reveal much
			{
				auto obj = cb->getTopObj(tile);

				// picking up resources does not yield any exploration at all.
				// if it blocks the way to some explorable tile AIPathfinder will take care of it
				if(obj && obj->isBlockedVisitable())
				{
					continue;
				}

				if(isSafeToVisit(hero, path.heroArmy, path.getTotalDanger()))
				{
					bestGoal = goal;
					bestValue = ourValue;
					bestTile = tile;
					bestTilesDiscovered = tilesDiscovered;
				}
			}
		}
	}

	void getVisibleNeighbours(const std::vector<int3> & tiles, std::vector<int3> & out) const
	{
		for(const int3 & tile : tiles)
		{
			foreach_neighbour(cbp, tile, [&](CCallback * cbp, int3 neighbour)
				{
					if((*(ts->fogOfWarMap))[neighbour.z][neighbour.x][neighbour.y])
					{
						out.push_back(neighbour);
					}
				});
		}
	}

	int howManyTilesWillBeDiscovered(const int3 & pos) const
	{
		int ret = 0;
		int3 npos = int3(0, 0, pos.z);

		const auto & slice = (*(ts->fogOfWarMap))[pos.z];

		for(npos.x = pos.x - sightRadius; npos.x <= pos.x + sightRadius; npos.x++)
		{
			for(npos.y = pos.y - sightRadius; npos.y <= pos.y + sightRadius; npos.y++)
			{
				if(cbp->isInTheMap(npos)
					&& pos.dist2d(npos) - 0.5 < sightRadius
					&& !slice[npos.x][npos.y])
				{
					if(allowDeadEndCancellation
						&& !hasReachableNeighbor(npos))
					{
						continue;
					}

					ret++;
				}
			}
		}

		return ret;
	}

	bool hasReachableNeighbor(const int3 & pos) const
	{
		for(const int3 & dir : int3::getDirs())
		{
			int3 tile = pos + dir;
			if(cbp->isInTheMap(tile))
			{
				auto isAccessible = ai->pathfinder->isTileAccessible(hero, tile);

				if(isAccessible)
					return true;
			}
		}

		return false;
	}
};

std::string ExplorationBehavior::toString() const
{
	return "Explore";
}

Goals::TGoalVec ExplorationBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;

	for(auto obj : ai->memory->visitableObjs)
	{
		if(!vstd::contains(ai->memory->alreadyVisited, obj))
		{
			switch(obj->ID.num)
			{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
				tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 200)).addNext(CaptureObject(obj))));
				break;
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				if(TeleportChannel::IMPASSABLE != ai->memory->knownTeleportChannels[tObj->channel]->passability)
				{
					tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
				}
				break;
			}
		}
		else
		{
			switch(obj->ID.num)
			{
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				if(TeleportChannel::IMPASSABLE == ai->memory->knownTeleportChannels[tObj->channel]->passability)
					break;
				for(auto exit : ai->memory->knownTeleportChannels[tObj->channel]->exits)
				{
					if(!cb->getObj(exit))
					{ 
						// Always attempt to visit two-way teleports if one of channel exits is not visible
						tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
						break;
					}
				}
				break;
			}
		}
	}

	auto heroes = ai->cb->getHeroesInfo();

	for(const CGHeroInstance * hero : heroes)
	{
		ExplorationHelper scanResult(hero, ai);

		scanResult.scanSector(1);

		if(!scanResult.bestGoal->invalid())
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		scanResult.scanSector(15);

		if(!scanResult.bestGoal->invalid())
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(ai->getScanDepth() == ScanDepth::ALL_FULL)
		{
			scanResult.scanMap();

			if(!scanResult.bestGoal->invalid())
			{
				tasks.push_back(scanResult.makeComposition());
			}
		}
	}

	return tasks;
}

}
