/*
* CollectRes.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT CollectRes : public CGoal<CollectRes>
	{
	public:
		CollectRes()
			: CGoal(Goals::COLLECT_RES)
		{
		}
		CollectRes(int rid, int val)
			: CGoal(Goals::COLLECT_RES)
		{
			resID = rid;
			value = val;
		}
		/*TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;*/
		TSubgoal whatToDoToTrade();
		virtual bool operator==(const CollectRes & other) const override;
	};
}
