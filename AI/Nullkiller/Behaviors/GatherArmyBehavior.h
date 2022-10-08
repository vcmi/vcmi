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

#include "lib/VCMI_Lib.h"
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

		virtual TGoalVec decompose() const override;
		virtual std::string toString() const override;

		virtual bool operator==(const GatherArmyBehavior & other) const override
		{
			return true;
		}

	private:
		TGoalVec deliverArmyToHero(const CGHeroInstance * hero) const;
		TGoalVec upgradeArmy(const CGTownInstance * upgrader) const;
	};
}


}
