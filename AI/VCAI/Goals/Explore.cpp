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

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

namespace Goals
{
	struct ExplorationHelper
	{
		HeroPtr hero;
		int sightRadius;
		float bestValue;
		TSubgoal bestGoal;
		VCAI * aip;
		CCallback * cbp;
		const TeamState * ts;
		int3 ourPos;
		bool allowDeadEndCancellation;
		bool allowGatherArmy;

		ExplorationHelper(HeroPtr h, bool gatherArmy)
		{
			cbp = cb.get();
			aip = ai.get();
			hero = h;
			ts = cbp->getPlayerTeam(ai->playerID);
			sightRadius = hero->getSightRadius();
			bestGoal = sptr(Goals::Invalid());
			bestValue = 0;
			ourPos = h->visitablePos();
			allowDeadEndCancellation = true;
			allowGatherArmy = gatherArmy;
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

			logAi->debug("Exploration scan visible area perimeter for hero %s", hero.name);

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

			logAi->debug("Exploration scan all possible tiles for hero %s", hero.name);

			for(const int3 & tile : from)
			{
				scanTile(tile);
			}
		}

		void scanTile(const int3 & tile)
		{
			if(tile == ourPos
				|| !aip->ah->isTileAccessible(hero, tile)) //shouldn't happen, but it does
				return;

			int tilesDiscovered = howManyTilesWillBeDiscovered(tile);
			if(!tilesDiscovered)
				return;

			auto waysToVisit = aip->ah->howToVisitTile(hero, tile, allowGatherArmy);
			for(const auto & goal : waysToVisit)
			{
				if(goal->evaluationContext.movementCost <= 0.0) // should not happen
					continue;

				float ourValue = (float)tilesDiscovered * tilesDiscovered / goal->evaluationContext.movementCost;

				if(ourValue > bestValue) //avoid costly checks of tiles that don't reveal much
				{
					auto obj = cb->getTopObj(tile);

					// picking up resources does not yield any exploration at all.
					// if it blocks the way to some explorable tile AIPathfinder will take care of it
					if(obj && obj->blockVisit)
					{
						continue;
					}

					if(isSafeToVisit(hero, tile))
					{
						bestGoal = goal;
						bestValue = ourValue;
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

		bool hasReachableNeighbor(const int3 &pos) const
		{
			for(crint3 dir : int3::getDirs())
			{
				int3 tile = pos + dir;
				if(cbp->isInTheMap(tile))
				{
					auto isAccessible = aip->ah->isTileAccessible(hero, tile);

					if(isAccessible)
						return true;
				}
			}

			return false;
		}
	};
}

bool Explore::operator==(const Explore & other) const
{
	return other.hero.h == hero.h && other.allowGatherArmy == allowGatherArmy;
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->getNameTranslated() + " completed exploration";
}

TSubgoal Explore::whatToDoToAchieve()
{
	return fh->chooseSolution(getAllPossibleSubgoals());
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
				for(const auto & exit : ai->knownTeleportChannels[tObj->channel]->exits)
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
			logAi->trace("Exploration searching for a new point for hero %s", h->getNameTranslated());

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

TSubgoal Explore::explorationBestNeighbour(int3 hpos, HeroPtr h) const
{
	ExplorationHelper scanResult(h, allowGatherArmy);

	for(crint3 dir : int3::getDirs())
	{
		int3 tile = hpos + dir;
		if(cb->isInTheMap(tile))
		{
			scanResult.scanTile(tile);
		}
	}

	return scanResult.bestGoal;
}


TSubgoal Explore::explorationNewPoint(HeroPtr h) const
{
	ExplorationHelper scanResult(h, allowGatherArmy);

	scanResult.scanSector(10);

	if(!scanResult.bestGoal->invalid())
	{
		return scanResult.bestGoal;
	}

	scanResult.scanMap();

	return scanResult.bestGoal;
}


TSubgoal Explore::exploreNearestNeighbour(HeroPtr h) const
{
	TimeCheck tc("where to explore");
	int3 hpos = h->visitablePos();

	//look for nearby objs -> visit them if they're close enough
	const int DIST_LIMIT = 3;
	const float COST_LIMIT = .2f; //todo: fine tune

	std::vector<const CGObjectInstance *> nearbyVisitableObjs;
	for(int x = hpos.x - DIST_LIMIT; x <= hpos.x + DIST_LIMIT; ++x) //get only local objects instead of all possible objects on the map
	{
		for(int y = hpos.y - DIST_LIMIT; y <= hpos.y + DIST_LIMIT; ++y)
		{
			for(auto obj : cb->getVisitableObjs(int3(x, y, hpos.z), false))
			{
				if(ai->isGoodForVisit(obj, h, COST_LIMIT))
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
	return explorationBestNeighbour(hpos, h);
}
