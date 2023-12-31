/*
* Trade.h, part of VCMI engine
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
	class DLL_EXPORT Trade : public CGoal<Trade>
	{
	public:
		Trade()
			: CGoal(Goals::TRADE)
		{
		}
		Trade(GameResID rid, int val, int Objid)
			: CGoal(Goals::TRADE)
		{
			resID = rid.getNum();
			value = val;
			objid = Objid;
			priority = 3; //trading is instant, but picking resources is free
		}
		TSubgoal whatToDoToAchieve() override;
		virtual bool operator==(const Trade & other) const override;
	};
}
