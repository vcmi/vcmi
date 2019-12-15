/*
* GatherTroops.h, part of VCMI engine
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
	class DLL_EXPORT GatherTroops : public CGoal<GatherTroops>
	{
	public:
		GatherTroops()
			: CGoal(Goals::GATHER_TROOPS)
		{
			priority = 2;
		}
		GatherTroops(int type, int val)
			: CGoal(Goals::GATHER_TROOPS)
		{
			objid = type;
			value = val;
			priority = 2;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		virtual bool operator==(const GatherTroops & other) const override;

	private:
		int getCreaturesCount(const CArmedInstance * army);
	};
}
