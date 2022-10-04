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

namespace NKAI
{

struct HeroPtr;
class AIGateway;
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
		Trade(int rid, int val, int Objid)
			: CGoal(Goals::TRADE)
		{
			resID = rid;
			value = val;
			objid = Objid;
		}
		virtual bool operator==(const Trade & other) const override;
	};
}

}
