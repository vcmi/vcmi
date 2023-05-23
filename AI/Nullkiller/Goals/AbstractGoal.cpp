/*
* AbstractGoal.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AbstractGoal.h"
#include "../AIGateway.h"
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

TSubgoal Goals::sptr(const AbstractGoal & tmp)
{
	TSubgoal ptr;
	ptr.reset(tmp.clone());
	return ptr;
}

TTask Goals::taskptr(const AbstractGoal & tmp)
{
	TTask ptr;

	if(!tmp.isElementar())
		throw cannotFulfillGoalException(tmp.toString() + " is not elementar");

	ptr.reset(dynamic_cast<ITask *>(tmp.clone()));

	return ptr;
}

std::string AbstractGoal::toString() const //TODO: virtualize
{
	std::string desc;
	switch(goalType)
	{
	case COLLECT_RES:
		desc = "COLLECT RESOURCE " + GameConstants::RESOURCE_NAMES[resID] + " (" + std::to_string(value) + ")";
		break;
	case TRADE:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if (obj)
			desc = (boost::format("TRADE %d of %s at %s") % value % GameConstants::RESOURCE_NAMES[resID] % obj->getObjectName()).str();
	}
	break;
	case GATHER_TROOPS:
		desc = "GATHER TROOPS";
		break;
	case GET_ART_TYPE:
		desc = "GET ARTIFACT OF TYPE " + VLC->artifacts()->getByIndex(aid)->getNameTranslated();
		break;
	case DIG_AT_TILE:
		desc = "DIG AT TILE " + tile.toString();
		break;
	default:
		return std::to_string(goalType);
	}
	if(hero.get(true)) //FIXME: used to crash when we lost hero and failed goal
		desc += " (" + hero->getNameTranslated() + ")";
	return desc;
}

bool AbstractGoal::operator==(const AbstractGoal & g) const
{
	return false;
}

//TODO: find out why the following are not generated automatically on MVS?
bool TSubgoal::operator==(const TSubgoal & rhs) const
{
	return *get() == *rhs.get(); //comparison for Goals is overloaded, so they don't need to be identical to match
}

bool AbstractGoal::invalid() const
{
	return goalType == EGoals::INVALID;
}

}
