/*
* BuyArmy.h, part of VCMI engine
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
	class DLL_EXPORT BuyArmy : public ElementarGoal<BuyArmy>
	{
	private:
		BuyArmy()
			: ElementarGoal(Goals::BUY_ARMY)
		{
		}
	public:
		BuyArmy(const CGTownInstance * Town, int val)
			: ElementarGoal(Goals::BUY_ARMY)
		{
			town = Town; //where to buy this army
			value = val; //expressed in AI unit strength
			priority = 3;//TODO: evaluate?
		}

		virtual bool operator==(const BuyArmy & other) const override;

		virtual std::string toString() const override;

		virtual void accept(AIGateway * ai) override;
	};
}

}
