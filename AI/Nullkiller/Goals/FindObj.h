/*
* FindObj.h, part of VCMI engine
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
	class DLL_EXPORT FindObj : public CGoal<FindObj>
	{
	public:
		FindObj() {} // empty constructor not allowed

		FindObj(int ID)
			: CGoal(Goals::FIND_OBJ)
		{
			objid = ID;
			resID = -1; //subid unspecified
		}
		FindObj(int ID, int subID)
			: CGoal(Goals::FIND_OBJ)
		{
			objid = ID;
			resID = subID;
		}
		virtual bool operator==(const FindObj & other) const override;

	private:
		//TSubgoal decomposeSingle() const override;
	};
}