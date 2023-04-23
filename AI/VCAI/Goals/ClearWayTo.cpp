/*
* ClearWayTo.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ClearWayTo.h"
#include "Explore.h"
#include "RecruitHero.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool ClearWayTo::operator==(const ClearWayTo & other) const
{
	return other.hero.h == hero.h && other.tile == tile;
}

TSubgoal ClearWayTo::whatToDoToAchieve()
{
	assert(cb->isInTheMap(tile)); //set tile
	if(!cb->isVisible(tile))
	{
		logAi->error("Clear way should be used with visible tiles!");
		return sptr(Explore());
	}

	return (fh->chooseSolution(getAllPossibleSubgoals()));
}

bool ClearWayTo::fulfillsMe(TSubgoal goal)
{
	if (goal->goalType == VISIT_TILE)
	{
		if (!hero || hero == goal->hero)
			return tile == goal->tile;
	}
	return false;
}

TGoalVec ClearWayTo::getAllPossibleSubgoals()
{
	TGoalVec ret;

	std::vector<const CGHeroInstance *> heroes;
	if(hero)
		heroes.push_back(hero.h);
	else
		heroes = cb->getHeroesInfo();

	for(auto h : heroes)
	{
		//TODO: handle clearing way to allied heroes that are blocked
		//if ((hero && hero->visitablePos() == tile && hero == *h) || //we can't free the way ourselves
		//	h->visitablePos() == tile) //we are already on that tile! what does it mean?
		//	continue;

		//if our hero is trapped, make sure we request clearing the way from OUR perspective

		vstd::concatenate(ret, ai->ah->howToVisitTile(h, tile));
	}

	if(ret.empty() && ai->canRecruitAnyHero())
		ret.push_back(sptr(RecruitHero()));

	if(ret.empty())
	{
		logAi->warn("There is no known way to clear the way to tile %s", tile.toString());
		throw goalFulfilledException(sptr(ClearWayTo(tile))); //make sure asigned hero gets unlocked
	}

	return ret;
}
