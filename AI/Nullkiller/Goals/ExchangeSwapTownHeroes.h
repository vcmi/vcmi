/*
* ExchangeSwapTownHeroes.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"
#include "../Engine/Nullkiller.h"

namespace Goals
{
	class DLL_EXPORT ExchangeSwapTownHeroes : public CGoal<ExchangeSwapTownHeroes>
	{
	private:
		const CGTownInstance * town;
		const CGHeroInstance * garrisonHero;
		HeroLockedReason lockingReason;

	public:
		ExchangeSwapTownHeroes(
			const CGTownInstance * town,
			const CGHeroInstance * garrisonHero = nullptr,
			HeroLockedReason lockingReason = HeroLockedReason::NOT_LOCKED);

		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}

		TSubgoal whatToDoToAchieve() override;
		void accept(VCAI * ai) override;
		std::string name() const override;
		virtual bool operator==(const ExchangeSwapTownHeroes & other) const override;
	};
}
