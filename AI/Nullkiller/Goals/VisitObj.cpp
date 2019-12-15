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