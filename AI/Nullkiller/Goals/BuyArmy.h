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

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT BuyArmy : public CGoal<BuyArmy>
	{
	private:
		BuyArmy()
			: CGoal(Goals::BUY_ARMY)
		{
		}
	public:
		BuyArmy(const CGTownInstance * Town, int val)
			: CGoal(Goals::BUY_ARMY)
		{
			town = Town; //where to buy this army
			value = val; //expressed in AI unit strength
			priority = 3;//TODO: evaluate?
		}

		TSubgoal whatToDoToAchieve() override;
		virtual bool operator==(const BuyArmy & other) const override;
	};
}
