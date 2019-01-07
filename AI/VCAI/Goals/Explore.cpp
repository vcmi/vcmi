/*
* Explore.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"
#include "../../../lib/CPlayerState.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool Explore::operator==(const Explore & other) const
{
	return other.hero.h == hero.h && other.allowGatherArmy == allowGatherArmy;
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->name + " completed exploration";
}

TSubgoal Explore::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());
	if(hero) //use best step for this hero
	{
		return ret;
	}
	else
	{
		if(ret->hero.get(true))
			return sptr(Explore().sethero(ret->hero.h)); //choose this hero and then continue with him
		else
			return ret; //other solutions, like buying hero from tavern
	}
}

TGoalVec Explore::getAllPossibleSubgoals()
{
	TGoalVec ret;
	std::vector<const CGHeroInstance *> heroes;

	if(hero)
	{
		heroes.push_back(hero.h);
	}
	else
	{
		//heroes = ai->getUnblockedHeroes();
		heroes = cb->getHeroesInfo();
		vstd::erase_if(heroes, [](const HeroPtr h)
		{
			if(ai->getGoal(h)->goalType == EXPLORE) //do not reassign hero who is already explorer
				return true;

			if(!ai->isAbleToExplore(h))
				return true;

			return !h->movement; //saves time, immobile heroes are useless anyway
		});
	}

	//try to use buildings that uncover map
	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(!vstd::contains(ai->alreadyVisited, obj))
		{
			switch(obj->ID.num)
			{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			case Obj::CARTOGRAPHER:
				objs.push_back(obj);
				break;
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				assert(ai->knownTeleportChannels.find(tObj->channel) != ai->knownTeleportChannels.end());
				if(TeleportChannel::IMPASSABLE != ai->knownTeleportChannels[tObj->channel]->passability)
					objs.push_back(obj);
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
				if(TeleportChannel::IMPASSABLE == ai->knownTeleportChannels[tObj->channel]->passability)
					break;
				for(auto exit : ai->knownTeleportChannels[tObj->channel]->exits)
				{
					if(!cb->getObj(exit))
					{ // Always attempt to visit two-way teleports if one of channel exits is not visible
						objs.push_back(obj);
						break;
					}
				}
				break;
			}
		}
	}

	auto primaryHero = ai->primaryHero().h;
	for(auto h : heroes)
	{
		for(auto obj : objs) //double loop, performance risk?
		{
			auto waysToVisitObj = ai->ah->howToVisitObj(h, obj, allowGatherArmy);

			vstd::concatenate(ret, waysToVisitObj);
		}

		TSubgoal goal = exploreNearestNeighbour(h);

		if(!goal->invalid())
		{
			ret.push_back(goal);
		}
	}

	if(ret.empty())
	{
		for(auto h : heroes)
		{
			TSubgoal goal = explorationNewPoint(h);

			if(goal->invalid())
			{
				ai->markHeroUnableToExplore(h); //there is no freely accessible tile, do not poll this hero anymore
			}
			else
			{
				ret.push_back(goal);
			}
		}
	}

	//we either don't have hero yet or none of heroes can explore
	if((!hero || ret.empty()) && ai->canRecruitAnyHero())
		ret.push_back(sptr(RecruitHero()));

	if(ret.empty())
	{
		throw goalFulfilledException(sptr(Explore().sethero(hero)));
	}

	return ret;
}

bool Explore::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == EXPLORE)
	{
		if(goal->hero)
			return hero == goal->hero;
		else
			return true; //cancel ALL exploration
	}
	return false;
}

bool Explore::hasReachableNeighbor(const int3 &pos, HeroPtr hero, CCallback * cbp, VCAI * vcai) const
{
	for(crint3 dir : int3::getDirs())
	{
		int3 tile = pos + dir;
		if(cbp->isInTheMap(tile))
		{
			auto isAccessible = vcai->ah->isTileAccessible(hero, tile);

			if(isAccessible)
				return true;
		}
	}

	return false;
}

int Explore::howManyTilesWillBeDiscovered(
	const int3 & pos, 
	int radious, 
	CCallback * cbp, 
	const TeamState * ts,
	VCAI * aip,
	HeroPtr h) const
{
	int ret = 0;
	for(int x = pos.x - radious; x <= pos.x + radious; x++)
	{
		for(int y = pos.y - radious; y <= pos.y + radious; y++)
		{
			int3 npos = int3(x, y, pos.z);
			if(cbp->isInTheMap(npos) 
				&& pos.dist2d(npos) - 0.5 < radious 
				&& !ts->fogOfWarMap[npos.x][npos.y][npos.z]
				&& hasReachableNeighbor(npos, h, cbp, aip))
			{
				ret++;
			}
		}
	}

	return ret;
}

TSubgoal Explore::explorationBestNeighbour(int3 hpos, int radius, HeroPtr h) const
{
	std::map<int3, int> dstToRevealedTiles;
	VCAI * aip = ai.get();
	CCallback * cbp = cb.get();
	const TeamState * ts = cbp->getPlayerTeam(ai->playerID);

	for(crint3 dir : int3::getDirs())
	{
		int3 tile = hpos + dir;
		if(cb->isInTheMap(tile))
		{
			if(isBlockVisitObj(tile))
				continue;

			if(isSafeToVisit(h, tile) && ai->isAccessibleForHero(tile, h))
			{
				auto distance = hpos.dist2d(tile); // diagonal movement opens more tiles but spends more mp
				int tilesDiscovered = howManyTilesWillBeDiscovered(tile, radius, cbp, ts, aip, h);

				dstToRevealedTiles[tile] = tilesDiscovered / distance;
			}
		}
	}

	if(dstToRevealedTiles.empty()) //yes, it DID happen!
		return sptr(Invalid());

	auto best = dstToRevealedTiles.begin();
	for(auto i = dstToRevealedTiles.begin(); i != dstToRevealedTiles.end(); i++)
	{
		const CGPathNode * pn = cb->getPathsInfo(h.get())->getPathInfo(i->first);
		//const TerrainTile *t = cb->getTile(i->first);
		if(best->second < i->second && pn->reachable() && pn->accessible == CGPathNode::ACCESSIBLE)
			best = i;
	}

	if(best->second)
		return sptr(VisitTile(best->first).sethero(h));

	return sptr(Invalid());
}

TSubgoal Explore::explorationNewPoint(HeroPtr h) const
{
	int radius = h->getSightRadius();
	CCallback * cbp = cb.get();
	VCAI *aip = ai.get();
	const TeamState * ts = cbp->getPlayerTeam(aip->playerID);

	int3 mapSize = cbp->getMapSize();
	int perimiter = 2 * radius * (mapSize.x + mapSize.y);
	
	std::vector<int3> from;
	std::vector<int3> to;

	from.reserve(perimiter);
	to.reserve(perimiter);

	foreach_tile_pos([&](const int3 & pos)
	{
		if(ts->fogOfWarMap[pos.x][pos.y][pos.z])
		{
			bool hasInvisibleNeighbor = false;

			foreach_neighbour(cbp, pos, [&](CCallback * cbp, int3 neighbour)
			{
				if(!ts->fogOfWarMap[neighbour.x][neighbour.y][neighbour.z])
				{
					hasInvisibleNeighbor = true;
				}
			});

			if(hasInvisibleNeighbor)
				from.push_back(pos);
		}
	});

	for(int i = 0; i < radius; i++)
	{
		getVisibleNeighbours(from, to, cbp, ts);
		vstd::concatenate(from, to);
		vstd::removeDuplicates(from);
	}

	return explorationScanRange(h, from);
}

TSubgoal Explore::explorationScanRange(HeroPtr h, std::vector<int3> & range) const
{
	int radius = h->getSightRadius();
	CCallback * cbp = cb.get();
	VCAI *aip = ai.get();
	const TeamState * ts = cbp->getPlayerTeam(aip->playerID);

	float bestValue = 0; //discovered tile to node distance ratio
	TSubgoal bestWay = sptr(Invalid());
	int3 ourPos = h->convertPosition(h->pos, false);

	for(const int3 & tile : range)
	{
		if(tile == ourPos) //shouldn't happen, but it does
			continue;

		int tilesDiscovered = howManyTilesWillBeDiscovered(tile, radius, cbp, ts, aip, h);
		if(!tilesDiscovered)
			continue;

		auto waysToVisit = aip->ah->howToVisitTile(h, tile, allowGatherArmy);
		for(auto goal : waysToVisit)
		{
			if(goal->evaluationContext.movementCost == 0) // should not happen
				continue;

			float ourValue = (float)tilesDiscovered * tilesDiscovered / goal->evaluationContext.movementCost;

			if(ourValue > bestValue) //avoid costly checks of tiles that don't reveal much
			{
				auto obj = cb->getTopObj(tile);
				if(obj && obj->blockVisit && !isObjectRemovable(obj)) //we can't stand on that object
				{
					continue;
				}

				if(isSafeToVisit(h, tile))
				{
					bestWay = goal;
					bestValue = ourValue;
				}
			}
		}
	}

	if(!bestWay->invalid())
	{
		return bestWay;
	}

	return bestWay;
}

TSubgoal Explore::exploreNearestNeighbour(HeroPtr h) const
{
	TimeCheck tc("where to explore");
	int radius = h->getSightRadius();
	int3 hpos = h->visitablePos();

	//look for nearby objs -> visit them if they're close enouh
	const int DIST_LIMIT = 3;
	const int MP_LIMIT = DIST_LIMIT * 150; // aproximate cost of diagonal movement

	std::vector<const CGObjectInstance *> nearbyVisitableObjs;
	for(int x = hpos.x - DIST_LIMIT; x <= hpos.x + DIST_LIMIT; ++x) //get only local objects instead of all possible objects on the map
	{
		for(int y = hpos.y - DIST_LIMIT; y <= hpos.y + DIST_LIMIT; ++y)
		{
			for(auto obj : cb->getVisitableObjs(int3(x, y, hpos.z), false))
			{
				if(ai->isGoodForVisit(obj, h, MP_LIMIT))
				{
					nearbyVisitableObjs.push_back(obj);
				}
			}
		}
	}

	if(nearbyVisitableObjs.size())
	{
		vstd::removeDuplicates(nearbyVisitableObjs); //one object may occupy multiple tiles
		boost::sort(nearbyVisitableObjs, CDistanceSorter(h.get()));

		TSubgoal pickupNearestObj = fh->chooseSolution(ai->ah->howToVisitObj(h, nearbyVisitableObjs.back(), false));

		if(!pickupNearestObj->invalid())
		{
			return pickupNearestObj;
		}
	}

	//check if nearby tiles allow us to reveal anything - this is quick
	return explorationBestNeighbour(hpos, radius, h);
}

void Explore::getVisibleNeighbours(const std::vector<int3> & tiles, std::vector<int3> & out, CCallback * cbp, const TeamState * ts) const
{
	for(const int3 & tile : tiles)
	{
		foreach_neighbour(cbp, tile, [&](CCallback * cbp, int3 neighbour)
		{
			if(ts->fogOfWarMap[neighbour.x][neighbour.y][neighbour.z])
			{
				out.push_back(neighbour);
			}
		});
	}
}