/*
* DismissHero.h, part of VCMI engine
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
	class DLL_EXPORT DismissHero : public CGoal<DismissHero>
	{
	public:
		DismissHero(HeroPtr hero)
			: CGoal(Goals::DISMISS_HERO)
		{
			sethero(hero);
		}

		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}

		TSubgoal whatToDoToAchieve() override;
		void accept(VCAI * ai) override;
		std::string name() const override;
		virtual bool operator==(const DismissHero & other) const override;
	};
}
