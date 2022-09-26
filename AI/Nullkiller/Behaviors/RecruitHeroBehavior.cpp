/*
* RecruitHeroBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "RecruitHeroBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/RecruitHero.h"
#include "../Goals/ExecuteHeroChain.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

std::string RecruitHeroBehavior::toString() const
{
	return "Recruit hero";
}

Goals::TGoalVec RecruitHeroBehavior::decompose() const
{
	Goals::TGoalVec tasks;
	auto towns = cb->getTownsInfo();

	for(auto town : towns)
	{
		if(!town->garrisonHero && !town->visitingHero && ai->canRecruitAnyHero(town))
		{
			if(cb->getHeroesInfo().size() < cb->getTownsInfo().size() + 1
				|| (ai->nullkiller->getFreeResources()[Res::GOLD] > 10000
					&& ai->nullkiller->buildAnalyzer->getGoldPreasure() < MAX_GOLD_PEASURE))
			{
				tasks.push_back(Goals::sptr(Goals::RecruitHero(town).setpriority(3)));
			}
		}
	}

	return tasks;
}

}
