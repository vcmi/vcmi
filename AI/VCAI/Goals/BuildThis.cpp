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
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/building/CBuilding.h"

using namespace Goals;

bool BuildThis::operator==(const BuildThis & other) const
{
	return town == other.town && bid == other.bid;
}

TSubgoal BuildThis::whatToDoToAchieve()
{
	auto b = BuildingID(bid);

	// find town if not set
	if(!town && hero)
		town = hero->getVisitedTown();

	if(!town)
	{
		for(const CGTownInstance * candidateTown : cb->getTownsInfo())
		{
			switch(cb->canBuildStructure(candidateTown, b))
			{
			case EBuildingState::ALLOWED:
				town = candidateTown;
				break; //TODO: look for prerequisites? this is not our responsibility
			default:
				continue;
			}
		}
	}

	if(town) //we have specific town to build this
	{
		switch(cb->canBuildStructure(town, b))
		{
		case EBuildingState::ALLOWED:
		case EBuildingState::NO_RESOURCES:
		{
			auto res = town->getTown()->buildings.at(BuildingID(bid))->resources;
			return ai->ah->whatToDo(res, iAmElementar()); //realize immediately or gather resources
		}
		break;
		default:
			throw cannotFulfillGoalException("Not possible to build");
		}
	}
	else
		throw cannotFulfillGoalException("Cannot find town to build this");
}
