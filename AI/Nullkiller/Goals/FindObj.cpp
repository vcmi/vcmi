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
#include "../VCAI.h"
#include "../AIUtility.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool FindObj::operator==(const FindObj & other) const
{
	return other.hero.h == hero.h && other.objid == objid;
}
//
//TSubgoal FindObj::whatToDoToAchieve()
//{
//	const CGObjectInstance * o = nullptr;
//	if(resID > -1) //specified
//	{
//		for(const CGObjectInstance * obj : ai->visitableObjs)
//		{
//			if(obj->ID == objid && obj->subID == resID)
//			{
//				o = obj;
//				break; //TODO: consider multiple objects and choose best
//			}
//		}
//	}
//	else
//	{
//		for(const CGObjectInstance * obj : ai->visitableObjs)
//		{
//			if(obj->ID == objid)
//			{
//				o = obj;
//				break; //TODO: consider multiple objects and choose best
//			}
//		}
//	}
//	if(o && ai->isAccessible(o->pos)) //we don't use isAccessibleForHero as we don't know which hero it is
//		return sptr(VisitObj(o->id.getNum()));
//}