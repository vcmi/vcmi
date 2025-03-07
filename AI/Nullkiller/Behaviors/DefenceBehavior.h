/*
* DefenceBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/GameLibrary.h"
#include "../Goals/CGoal.h"
#include "../AIUtility.h"

namespace NKAI
{

struct HitMapInfo;

namespace Goals
{
	class DefenceBehavior : public CGoal<DefenceBehavior>
	{
	public:
		DefenceBehavior()
			:CGoal(DEFENCE)
		{
		}

		Goals::TGoalVec decompose(const Nullkiller * ai) const override;
		std::string toString() const override;

		bool operator==(const DefenceBehavior & other) const override
		{
			return true;
		}

	private:
		void evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town, const Nullkiller * ai) const;
		void evaluateRecruitingHero(Goals::TGoalVec & tasks, const HitMapInfo & threat, const CGTownInstance * town, const Nullkiller * ai) const;
	};
}


}
