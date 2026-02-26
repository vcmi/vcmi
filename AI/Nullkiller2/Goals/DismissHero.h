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

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT DismissHero : public ElementarGoal<DismissHero>
	{
	private:
		std::string heroName;

	public:
		DismissHero(const CGHeroInstance * hero)
			: ElementarGoal(Goals::DISMISS_HERO)
		{
			sethero(hero);
			heroName = hero->getNameTranslated();
		}

		void accept(AIGateway * aiGw) override;
		std::string toString() const override;
		bool operator==(const DismissHero & other) const override;
	};
}

}
