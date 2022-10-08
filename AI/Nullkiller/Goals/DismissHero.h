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

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT DismissHero : public ElementarGoal<DismissHero>
	{
	public:
		DismissHero(HeroPtr hero)
			: ElementarGoal(Goals::DISMISS_HERO)
		{
			sethero(hero);
		}

		void accept(AIGateway * ai) override;
		std::string toString() const override;
		virtual bool operator==(const DismissHero & other) const override;
	};
}

}
