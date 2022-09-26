/*
* RecruitHero.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"


namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

std::string RecruitHero::toString() const
{
	return "Recruit hero at " + town->name;
}

void RecruitHero::accept(AIGateway * ai)
{
	auto t = town;

	if(!t) t = ai->findTownWithTavern();

	if(t)
	{
		ai->recruitHero(t, true);
		//TODO try to free way to blocked town
		//TODO: adventure map tavern or prison?
	}
	else
	{
		throw cannotFulfillGoalException("No town to recruit hero!");
	}
}

}
