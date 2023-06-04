/*
* RecruitHero.h, part of VCMI engine
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
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT RecruitHero : public ElementarGoal<RecruitHero>
	{
	private:
		const CGHeroInstance * heroToBuy;

	public:
		RecruitHero(const CGTownInstance * townWithTavern, const CGHeroInstance * heroToBuy)
			: ElementarGoal(Goals::RECRUIT_HERO), heroToBuy(heroToBuy)
		{
			town = townWithTavern;
			priority = 1;
		}

		RecruitHero(const CGTownInstance * townWithTavern)
			: RecruitHero(townWithTavern, nullptr)
		{
		}

		virtual bool operator==(const RecruitHero & other) const override
		{
			return true;
		}

		virtual std::string toString() const override;
		void accept(AIGateway * ai) override;
	};
}

}
