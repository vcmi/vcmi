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

using namespace Goals;

bool BuildBoat::operator==(const BuildBoat & other) const
{
	return shipyard == other.shipyard;
}

TSubgoal BuildBoat::whatToDoToAchieve()
{
	if(cb->getPlayerRelations(ai->playerID, shipyard->getObject()->getOwner()) == PlayerRelations::ENEMIES)
	{
		return fh->chooseSolution(ai->ah->howToVisitObj(dynamic_cast<const CGObjectInstance*>(shipyard)));
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

	if(cb->getPlayerRelations(ai->playerID, shipyard->getObject()->getOwner()) == PlayerRelations::ENEMIES)
	{
		throw cannotFulfillGoalException("Can not build boat in enemy shipyard");
	}

	if(shipyard->shipyardStatus() != IShipyard::GOOD)
	{
		throw cannotFulfillGoalException("Shipyard is busy.");
	}

	logAi->trace(
		"Building boat at shipyard located at %s, estimated boat position %s",
		shipyard->getObject()->visitablePos().toString(),
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
	return "Boat have been built at " + shipyard->getObject()->visitablePos().toString();
}
