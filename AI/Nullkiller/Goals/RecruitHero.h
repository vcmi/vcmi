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
	public:
		RecruitHero(const CGTownInstance * townWithTavern, const CGHeroInstance * heroToBuy)
			: RecruitHero(townWithTavern)
		{
			objid = heroToBuy->id.getNum();
		}

		RecruitHero(const CGTownInstance * townWithTavern)
			: ElementarGoal(Goals::RECRUIT_HERO)
		{
			priority = 1;
			town = townWithTavern;
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
