/*
* ExecuteHeroChain.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

namespace Goals
{
	class DLL_EXPORT ExecuteHeroChain : public CGoal<ExecuteHeroChain>
	{
	private:
		AIPath chainPath;
		std::string targetName;

	public:
		ExecuteHeroChain(const AIPath & path, const CGObjectInstance * obj = nullptr);

		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}

		TSubgoal whatToDoToAchieve() override;
		void accept(VCAI * ai) override;
		std::string name() const override;
		virtual bool operator==(const ExecuteHeroChain & other) const override;
		const AIPath & getPath() const { return chainPath; }
	};
}
