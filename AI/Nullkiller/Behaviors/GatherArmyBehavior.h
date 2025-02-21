/*
* GatherArmyBehavior.h, part of VCMI engine
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
namespace Goals
{
	class GatherArmyBehavior : public CGoal<GatherArmyBehavior>
	{
	public:
		GatherArmyBehavior()
			:CGoal(Goals::GATHER_ARMY)
		{
		}

		TGoalVec decompose(const Nullkiller * ai) const override;
		std::string toString() const override;

		bool operator==(const GatherArmyBehavior & other) const override
		{
			return true;
		}

	private:
		TGoalVec deliverArmyToHero(const Nullkiller * ai, const CGHeroInstance * hero) const;
		TGoalVec upgradeArmy(const Nullkiller * ai, const CGTownInstance * upgrader) const;
	};
}


}
