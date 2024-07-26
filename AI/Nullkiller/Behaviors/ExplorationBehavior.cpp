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
#include "../Goals/CaptureObject.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../Helpers/ExplorationHelper.h"

namespace NKAI
{

using namespace Goals;

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
			case Obj::WHIRLPOOL:
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
			case Obj::WHIRLPOOL:
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

		if(scanResult.scanSector(1))
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(scanResult.scanSector(15))
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(ai->getScanDepth() == ScanDepth::ALL_FULL)
		{
			if(scanResult.scanMap())
			{
				tasks.push_back(scanResult.makeComposition());
			}
		}
	}

	return tasks;
}

}
