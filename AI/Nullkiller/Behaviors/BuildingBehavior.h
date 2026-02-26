/*
* BuildingBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/GameLibrary.h"
#include "../AIUtility.h"
#include "../Goals/CGoal.h"

namespace NKAI
{
namespace Goals
{
	class BuildingBehavior : public CGoal<BuildingBehavior>
	{
	public:
		BuildingBehavior()
			:CGoal(Goals::BUILD)
		{
		}

		Goals::TGoalVec decompose(const Nullkiller * ai) const override;
		std::string toString() const override;
		bool operator==(const BuildingBehavior & other) const override
		{
			return true;
		}
	};
}

}
