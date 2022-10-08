/*
* SpecialAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "SpecialAction.h"
#include "../../AIGateway.h"
#include "../../Goals/CGoal.h"
#include "../../Goals/Invalid.h"

namespace NKAI
{

Goals::TSubgoal SpecialAction::decompose(const CGHeroInstance * hero) const
{
	return Goals::sptr(Goals::Invalid());
}

void SpecialAction::execute(const CGHeroInstance * hero) const
{
	throw cannotFulfillGoalException("Can not execute " + toString());
}

}
