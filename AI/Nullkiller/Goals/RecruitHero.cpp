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
	return "Recruit hero at " + town->getNameTranslated();
}

void RecruitHero::accept(AIGateway * ai)
{
	auto t = town;

	if(!t) t = ai->findTownWithTavern();

	if(!t)
	{
		throw cannotFulfillGoalException("No town to recruit hero!");
	}

	logAi->debug("Trying to recruit a hero in %s at %s", t->getNameTranslated(), t->visitablePos().toString());

	auto heroes = cb->getAvailableHeroes(t);

	if(!heroes.size())
	{
		throw cannotFulfillGoalException("No available heroes in tavern in " + t->nodeName());
	}

	auto heroToHire = heroes[0];

	for(auto hero : heroes)
	{
		if(objid == hero->id.getNum())
		{
			heroToHire = hero;
			break;
		}

		if(hero->getTotalStrength() > heroToHire->getTotalStrength())
			heroToHire = hero;
	}

	if(t->visitingHero)
	{
		cb->swapGarrisonHero(t);
	}

	if(t->visitingHero)
		throw cannotFulfillGoalException("Town " + t->nodeName() + " is occupied. Cannot recruit hero!");

	cb->recruitHero(t, heroToHire);
	ai->nullkiller->heroManager->update();

	if(t->visitingHero)
		ai->moveHeroToTile(t->visitablePos(), t->visitingHero.get());
}

}
