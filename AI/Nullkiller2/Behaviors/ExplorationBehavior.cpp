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
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/CaptureObject.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../Goals/Invalid.h"
#include "../Helpers/ExplorationHelper.h"
#include "../Markers/ExplorationPoint.h"
#include "ExplorationBehavior.h"

#include "../../../lib/CPlayerState.h"

namespace NK2AI
{

using namespace Goals;

BoatExplorationEvaluation evaluateBoatExplorationCandidate(const BoatExplorationCandidate & candidate)
{
	BoatExplorationEvaluation evaluation;

	if(!candidate.available || candidate.hiddenTilesDiscovered <= 0)
		return evaluation;

	evaluation.accepted = true;
	evaluation.explorationValue = candidate.hiddenTilesDiscovered;
	return evaluation;
}

namespace
{
int countHiddenTilesAround(const Nullkiller * aiNk, const int3 & pos, const int sightRadius)
{
	const auto * teamState = aiNk->cc->getPlayerTeam(aiNk->playerID);
	const auto & fow = teamState->fogOfWarMap;
	int result = 0;
	int3 tile(0, 0, pos.z);

	for(tile.x = pos.x - sightRadius; tile.x <= pos.x + sightRadius; ++tile.x)
	{
		for(tile.y = pos.y - sightRadius; tile.y <= pos.y + sightRadius; ++tile.y)
		{
			if(aiNk->cc->isInTheMap(tile)
				&& pos.dist2d(tile) - 0.5 < sightRadius
				&& !fow[tile])
			{
				++result;
			}
		}
	}

	return result;
}

std::optional<int> getBoatExplorationValue(const Nullkiller * aiNk, const CGObjectInstance * obj)
{
	if(obj->ID != Obj::BOAT)
		return std::nullopt;

	const CGObjectInstance * topObj = aiNk->cc->getTopObj(obj->visitablePos());
	BoatExplorationCandidate candidate;
	candidate.available = topObj && topObj->id == obj->id;

	if(!candidate.available)
		return std::nullopt;

	int bestValue = 0;
	for(const CGHeroInstance * hero : aiNk->cc->getHeroesInfo())
		bestValue = std::max(bestValue, countHiddenTilesAround(aiNk, obj->visitablePos(), hero->getSightRadius()));

	candidate.hiddenTilesDiscovered = bestValue;
	const auto evaluation = evaluateBoatExplorationCandidate(candidate);
	if(!evaluation.accepted)
		return std::nullopt;

	return evaluation.explorationValue;
}
}

std::string ExplorationBehavior::toString() const
{
	return "Explore";
}

Goals::TGoalVec ExplorationBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
		if(!obj)
			continue;

		switch(obj->ID.num)
		{
			case Obj::BOAT:
			{
				if(auto explorationValue = getBoatExplorationValue(aiNk, obj))
				{
					tasks.push_back(sptr(Composition()
						.addNext(ExplorationPoint(obj->visitablePos(), *explorationValue))
						.addNext(CaptureObject(obj))));
				}
				break;
			}
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			{
				auto rObj = dynamic_cast<const CRewardableObject *>(obj);
				if(!rObj->wasScouted(aiNk->playerID))
					tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 200)).addNext(CaptureObject(obj))));
				break;
			}
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
			case Obj::WHIRLPOOL:
			{
				const auto tObj = dynamic_cast<const CGTeleport *>(obj);
				for(auto exit : aiNk->cc->getTeleportChannelExits(tObj->channel))
				{
					if(exit != tObj->id)
					{
						const CGObjectInstance * exitObj = aiNk->cc->getObjInstance(exit);

						// Please check for visible tile and not the visible object due to possible partial visibility
						// checking the object itself might incorrectly abort the exploration of the fog behind it
						if(exitObj && !aiNk->cc->isVisible(exitObj->visitablePos()))
						{
							tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
						}
					}
				}
				break;
			}
		}
	}

	const auto heroes = aiNk->cc->getHeroesInfo();
	for(const CGHeroInstance * hero : heroes)
	{
		if(aiNk->isHeroLocked(hero))
			continue;

		ExplorationHelper scanResult(hero, aiNk);
		const bool canUseDimensionDoor = scanResult.canUseDimensionDoor();
		auto improveWithDimensionDoor = [canUseDimensionDoor, &scanResult]()
		{
			return canUseDimensionDoor && scanResult.considerDimensionDoorExplorationTargets();
		};

		const bool foundNearbyTarget = scanResult.scanSector(1);

		if(foundNearbyTarget)
		{
			// Let DD compete with local walking exploration; otherwise any nearby fog
			// tile would hide a much stronger spell landing from the task selector.
			improveWithDimensionDoor();

			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		const bool foundSectorTarget = scanResult.scanSector(30);
		const bool foundDimensionDoorTarget = improveWithDimensionDoor();

		if(foundSectorTarget || foundDimensionDoorTarget)
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(aiNk->getScanDepth() == ScanDepth::ALL_FULL)
		{
			if(scanResult.scanMap())
			{
				tasks.push_back(scanResult.makeComposition());
			}
			// Last-resort scout movement: if no full-map exploration target survived,
			// spend remaining MP on safe adjacent fog instead of ending the turn idle.
			else if(const auto neighbourTarget = ExploreNeighbourTile::findTarget(hero, aiNk))
			{
				const bool strategicOnlyMove = neighbourTarget->tilesDiscovered == 0;
				tasks.push_back(sptr(Composition()
					.addNext(ExplorationPoint(neighbourTarget->tile, neighbourTarget->tilesDiscovered))
					.addNext(ExploreNeighbourTile(hero, strategicOnlyMove ? 1 : 5, strategicOnlyMove))));
			}
		}
	}

	return tasks;
}

}
