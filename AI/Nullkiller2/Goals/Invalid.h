/*
* Invalid.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace NK2AI
{

struct HeroPtr;
class AIGateway;

namespace Goals
{
	class DLL_EXPORT Invalid : public ElementarGoal<Invalid>
	{
	public:
		Invalid()
			: ElementarGoal(Goals::INVALID)
		{
			priority = -1;
		}
		TGoalVec decompose(const Nullkiller * aiNk) const override
		{
			return TGoalVec();
		}

		bool operator==(const Invalid & other) const override
		{
			return true;
		}

		std::string toString() const override
		{
			return "Invalid";
		}

		void accept(AIGateway * aiGw) override
		{
			throw cannotFulfillGoalException("Can not fulfill Invalid goal!");
		}
	};
}

}
