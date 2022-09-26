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

namespace NKAI
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
		TGoalVec decompose() const override
		{
			return TGoalVec();
		}

		virtual bool operator==(const Invalid & other) const override
		{
			return true;
		}

		virtual std::string toString() const override
		{
			return "Invalid";
		}

		virtual void accept(AIGateway * ai) override
		{
			throw cannotFulfillGoalException("Can not fulfill Invalid goal!");
		}
	};
}

}
