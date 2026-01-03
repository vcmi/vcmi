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

namespace NK2AI
{

using namespace Goals;

std::string ExplorationBehavior::toString() const
{
	return "Explore";
}

Goals::TGoalVec ExplorationBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	for (const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
		if(!obj)
			continue;

		switch (obj->ID.num)
		{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			{
				auto rObj = dynamic_cast<const CRewardableObject*>(obj);
				if (!rObj->wasScouted(aiNk->playerID))
					tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 200)).addNext(CaptureObject(obj))));
				break;
			}
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
			case Obj::WHIRLPOOL:
			{
				const auto tObj = dynamic_cast<const CGTeleport*>(obj);
				for (auto exit : aiNk->cc->getTeleportChannelExits(tObj->channel))
				{
					if (exit != tObj->id)
					{
						// TODO: Mircea: Looks like a bug. Should use isVisibleFor with aiNk->playerID
						if (!aiNk->cc->isVisible(aiNk->cc->getObjInstance(exit)))
							tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
					}
				}
			}
		}
	}

	const auto heroes = aiNk->cc->getHeroesInfo();
	for(const CGHeroInstance * hero : heroes)
	{
		ExplorationHelper scanResult(hero, aiNk);
		if(scanResult.scanSector(1))
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(scanResult.scanSector(30))
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
		}
	}

	return tasks;
}

}
