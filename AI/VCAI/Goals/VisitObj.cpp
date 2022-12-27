/*
* VisitObj.cpp, part of VCMI engine
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


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool VisitObj::operator==(const VisitObj & other) const
{
	return other.hero.h == hero.h && other.objid == objid;
}

std::string VisitObj::completeMessage() const
{
	return "hero " + hero.get()->getNameTranslated() + " captured Object ID = " + boost::lexical_cast<std::string>(objid);
}

TGoalVec VisitObj::getAllPossibleSubgoals()
{
	TGoalVec goalList;
	const CGObjectInstance * obj = cb->getObjInstance(ObjectInstanceID(objid));
	if(!obj)
	{
		throw cannotFulfillGoalException("Object is missing - goal is invalid now!");
	}

	int3 pos = obj->visitablePos();
	if(hero)
	{
		if(ai->isAccessibleForHero(pos, hero))
		{
			if(isSafeToVisit(hero, pos))
				goalList.push_back(sptr(VisitObj(obj->id.getNum()).sethero(hero)));
			else
				goalList.push_back(sptr(GatherArmy((int)(fh->evaluateDanger(pos, hero.h) * SAFE_ATTACK_CONSTANT)).sethero(hero).setisAbstract(true)));

			return goalList;
		}
	}
	else
	{
		for(auto potentialVisitor : cb->getHeroesInfo())
		{
			if(ai->isAccessibleForHero(pos, potentialVisitor))
			{
				if(isSafeToVisit(potentialVisitor, pos))
					goalList.push_back(sptr(VisitObj(obj->id.getNum()).sethero(potentialVisitor)));
				else
					goalList.push_back(sptr(GatherArmy((int)(fh->evaluateDanger(pos, potentialVisitor) * SAFE_ATTACK_CONSTANT)).sethero(potentialVisitor).setisAbstract(true)));
			}
		}
		if(!goalList.empty())
		{
			return goalList;
		}
	}

	goalList.push_back(sptr(ClearWayTo(pos)));
	return goalList;
}

TSubgoal VisitObj::whatToDoToAchieve()
{
	auto bestGoal = fh->chooseSolution(getAllPossibleSubgoals());

	if(bestGoal->goalType == VISIT_OBJ && bestGoal->hero)
		bestGoal->setisElementar(true);

	return bestGoal;
}

VisitObj::VisitObj(int Objid)
	: CGoal(VISIT_OBJ)
{
	objid = Objid;
	auto obj = ai->myCb->getObjInstance(ObjectInstanceID(objid));
	if(obj)
		tile = obj->visitablePos();
	else
		logAi->error("VisitObj constructed with invalid object instance %d", Objid);

	priority = 3;
}

bool VisitObj::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == VISIT_TILE)
	{
		if (!hero || hero == goal->hero)
		{
			auto obj = cb->getObjInstance(ObjectInstanceID(objid));
			if (obj && obj->visitablePos() == goal->tile) //object could be removed
				return true;
		}
	}
	return false;
}
