/*
* Conquer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool Conquer::operator==(const Conquer & other) const
{
	return other.hero.h == hero.h;
}

TSubgoal Conquer::whatToDoToAchieve()
{
	logAi->trace("Entering goal CONQUER");

	return fh->chooseSolution(getAllPossibleSubgoals());
}

TGoalVec Conquer::getAllPossibleSubgoals()
{
	TGoalVec ret;

	auto conquerable = [](const CGObjectInstance * obj) -> bool
	{
		if(cb->getPlayerRelations(ai->playerID, obj->tempOwner) == PlayerRelations::ENEMIES)
		{
			switch(obj->ID.num)
			{
			case Obj::TOWN:
			case Obj::HERO:
			case Obj::CREATURE_GENERATOR1:
			case Obj::MINE: //TODO: check ai->knownSubterraneanGates
				return true;
			}
		}
		return false;
	};

	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(conquerable(obj))
			objs.push_back(obj);
	}

	for(auto h : cb->getHeroesInfo())
	{
		std::vector<const CGObjectInstance *> ourObjs(objs); //copy common objects

		for(auto obj : ai->reservedHeroesMap[h]) //add objects reserved by this hero
		{
			if(conquerable(obj))
				ourObjs.push_back(obj);
		}
		for(auto obj : ourObjs)
		{
			auto waysToGo = ai->ah->howToVisitObj(h, ObjectIdRef(obj));

			vstd::concatenate(ret, waysToGo);
		}
	}
	if(!objs.empty() && ai->canRecruitAnyHero()) //probably no point to recruit hero if we see no objects to capture
		ret.push_back(sptr(RecruitHero()));

	if(ret.empty())
		ret.push_back(sptr(Explore())); //we need to find an enemy
	return ret;
}
