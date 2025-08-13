/*
* BuildThis.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "BuildThis.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/building/CBuilding.h"

namespace NKAI
{

using namespace Goals;

BuildThis::BuildThis(BuildingID Bid, const CGTownInstance * tid)
	: ElementarGoal(Goals::BUILD_STRUCTURE)
{
	buildingInfo = BuildingInfo(
		tid->getTown()->buildings.at(Bid).get(),
		nullptr,
		CreatureID::NONE,
		tid,
		nullptr);

	bid = Bid.getNum();
	town = tid;
}

bool BuildThis::operator==(const BuildThis & other) const
{
	return town == other.town && bid == other.bid;
}

std::string BuildThis::toString() const
{
	return "Build " + buildingInfo.name + " in " + town->getNameTranslated();
}

void BuildThis::accept(AIGateway * ai)
{
	auto b = BuildingID(bid);

	if(town)
	{
		if(cb->canBuildStructure(town, b) == EBuildingState::ALLOWED)
		{
			logAi->debug("Player %d will build %s in town of %s at %s",
				ai->playerID, town->getTown()->buildings.at(b)->getNameTranslated(), town->getNameTranslated(), town->anchorPos().toString());
			cb->buildBuilding(town, b);

			return;
		}
	}

	throw cannotFulfillGoalException("Cannot build a given structure!");
}

}
