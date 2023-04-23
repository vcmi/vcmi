/*
* FindObj.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "FindObj.h"
#include "VisitObj.h"
#include "Explore.h"
#include "../VCAI.h"
#include "../AIUtility.h"

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool FindObj::operator==(const FindObj & other) const
{
	return other.hero.h == hero.h && other.objid == objid;
}

TSubgoal FindObj::whatToDoToAchieve()
{
	const CGObjectInstance * o = nullptr;
	if(resID > -1) //specified
	{
		for(const CGObjectInstance * obj : ai->visitableObjs)
		{
			if(obj->ID == objid && obj->subID == resID)
			{
				o = obj;
				break; //TODO: consider multiple objects and choose best
			}
		}
	}
	else
	{
		for(const CGObjectInstance * obj : ai->visitableObjs)
		{
			if(obj->ID == objid)
			{
				o = obj;
				break; //TODO: consider multiple objects and choose best
			}
		}
	}
	if(o && ai->isAccessible(o->pos)) //we don't use isAccessibleForHero as we don't know which hero it is
		return sptr(VisitObj(o->id.getNum()));
	else
		return sptr(Explore());
}

bool FindObj::fulfillsMe(TSubgoal goal)
{
	if (goal->goalType == VISIT_TILE) //visiting tile visits object at same time
	{
		if (!hero || hero == goal->hero)
			for (auto obj : cb->getVisitableObjs(goal->tile)) //check if any object on that tile matches criteria
				if (obj->visitablePos() == goal->tile) //object could be removed
					if (obj->ID == objid && obj->subID == resID) //same type and subtype
						return true;
	}
	return false;
}
