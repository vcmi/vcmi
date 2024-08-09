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

	for (auto obj : ai->memory->visitableObjs)
	{
		switch (obj->ID.num)
		{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			{
				auto rObj = dynamic_cast<const CRewardableObject*>(obj);
				if (!rObj->wasScouted(ai->playerID))
					tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 200)).addNext(CaptureObject(obj))));
				break;
			}
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
			case Obj::WHIRLPOOL:
			{
				auto tObj = dynamic_cast<const CGTeleport*>(obj);
				for (auto exit : cb->getTeleportChannelExits(tObj->channel))
				{
					if (exit != tObj->id)
					{
						if (!cb->isVisible(cb->getObjInstance(exit)))
							tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
					}
				}
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
