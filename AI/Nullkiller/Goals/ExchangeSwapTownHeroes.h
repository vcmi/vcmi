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

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT ExchangeSwapTownHeroes : public ElementarGoal<ExchangeSwapTownHeroes>
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

		void accept(AIGateway * ai) override;
		std::string toString() const override;
		virtual bool operator==(const ExchangeSwapTownHeroes & other) const override;

		const CGHeroInstance * getGarrisonHero() const { return garrisonHero; }
		HeroLockedReason getLockingReason() const { return lockingReason; }
	};
}

}
