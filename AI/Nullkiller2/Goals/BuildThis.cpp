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

namespace NK2AI
{

using namespace Goals;

bool BuildThis::operator==(const BuildThis & other) const
{
	return town == other.town && bid == other.bid;
}

std::string BuildThis::toString() const
{
	return "Build " + buildingInfo.name + " in " + town->getNameTextID();
}

void BuildThis::accept(AIGateway * aiGw)
{
	auto b = BuildingID(bid);

	if(town)
	{
		if(aiGw->cc->canBuildStructure(town, b) == EBuildingState::ALLOWED)
		{
			logAi->debug("Player %d will build %s in town of %s at %s",
				aiGw->playerID, town->getTown()->buildings.at(b)->getNameTextID(), town->getNameTextID(), town->anchorPos().toString());
			aiGw->cc->buildBuilding(town, b);

			return;
		}
	}

	throw cannotFulfillGoalException("Cannot build a given structure!");
}

}
