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
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

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
		town = hero->visitedTown;

	if(!town)
	{
		for(const CGTownInstance * t : cb->getTownsInfo())
		{
			switch(cb->canBuildStructure(town, b))
			{
			case EBuildingState::ALLOWED:
				town = t;
				break; //TODO: look for prerequisites? this is not our reponsibility
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
			auto res = town->town->buildings.at(BuildingID(bid))->resources;
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
