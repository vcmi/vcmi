/*
* BuildBoat.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "BuildBoat.h"
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"

extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool BuildBoat::operator==(const BuildBoat & other) const
{
	return shipyard->o->id == other.shipyard->o->id;
}

TSubgoal BuildBoat::whatToDoToAchieve()
{
	if(cb->getPlayerRelations(ai->playerID, shipyard->o->tempOwner) == PlayerRelations::ENEMIES)
	{
		return fh->chooseSolution(ai->ah->howToVisitObj(shipyard->o));
	}

	if(shipyard->shipyardStatus() != IShipyard::GOOD)
	{
		throw cannotFulfillGoalException("Shipyard is busy.");
	}

	TResources boatCost;
	shipyard->getBoatCost(boatCost);

	return ai->ah->whatToDo(boatCost, this->iAmElementar());
}

void BuildBoat::accept(VCAI * ai)
{
	TResources boatCost;
	shipyard->getBoatCost(boatCost);

	if(!cb->getResourceAmount().canAfford(boatCost))
	{
		throw cannotFulfillGoalException("Can not afford boat");
	}

	if(cb->getPlayerRelations(ai->playerID, shipyard->o->tempOwner) == PlayerRelations::ENEMIES)
	{
		throw cannotFulfillGoalException("Can not build boat in enemy shipyard");
	}

	if(shipyard->shipyardStatus() != IShipyard::GOOD)
	{
		throw cannotFulfillGoalException("Shipyard is busy.");
	}

	logAi->trace(
		"Building boat at shipyard %s located at %s, estimated boat position %s", 
		shipyard->o->getObjectName(),
		shipyard->o->visitablePos().toString(),
		shipyard->bestLocation().toString());

	cb->buildBoat(shipyard);

	throw goalFulfilledException(sptr(*this));
}

std::string BuildBoat::name() const
{
	return "BuildBoat";
}

std::string BuildBoat::completeMessage() const
{
	return "Boat have been built at " + shipyard->o->visitablePos().toString();
}
