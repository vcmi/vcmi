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
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/ResourceTypeHandler.h"

namespace NK2AI
{

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

	ptr.reset(tmp.clone()->asTask());

	return ptr;
}

std::string AbstractGoal::toString() const
{
	std::string desc;
	switch(goalType)
	{
	case COLLECT_RES:
		desc = "COLLECT RESOURCE " + GameResID(resID).toResource()->getJsonKey() + " (" + std::to_string(value) + ")";
		break;
	case TRADE:
	{
		desc = (boost::format("TRADE %d of %s at objid %d") % value % GameResID(resID).toResource()->getJsonKey() % objid).str();
		break;
	}
	case GATHER_TROOPS:
		desc = "GATHER TROOPS";
		break;
	case GET_ART_TYPE:
		desc = "GET ARTIFACT OF TYPE " + ArtifactID(aid).toEntity(LIBRARY)->getNameTranslated();
		break;
	case DIG_AT_TILE:
		desc = "DIG AT TILE " + tile.toString();
		break;
	default:
		return std::to_string(goalType);
	}

	if(hero)
		desc += " (" + hero->getNameTranslated() + ")";

	return desc;
}

//TODO: find out why the following are not generated automatically on MVS?
bool TSubgoal::operator==(const TSubgoal & rhs) const
{
	return *get() == *rhs.get(); //comparison for Goals is overloaded, so they don't need to be identical to match
}

bool TSubgoal::operator<(const TSubgoal & rhs) const
{
	return false;
}

}
