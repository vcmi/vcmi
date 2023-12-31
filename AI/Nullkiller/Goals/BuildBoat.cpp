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
#include "../AIGateway.h"
#include "../Behaviors/CaptureObjectsBehavior.h"

namespace NKAI
{

using namespace Goals;

bool BuildBoat::operator==(const BuildBoat & other) const
{
	return shipyard == other.shipyard;
}
//
//TSubgoal BuildBoat::decomposeSingle() const
//{
//	if(cb->getPlayerRelations(ai->playerID, shipyard->o->tempOwner) == PlayerRelations::ENEMIES)
//	{
//		return sptr(CaptureObjectsBehavior(shipyard->o));
//	}
//
//	if(shipyard->shipyardStatus() != IShipyard::GOOD)
//	{
//		throw cannotFulfillGoalException("Shipyard is busy.");
//	}
//
//	TResources boatCost;
//	shipyard->getBoatCost(boatCost);
//
//	return iAmElementar();
//}

void BuildBoat::accept(AIGateway * ai)
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

std::string BuildBoat::toString() const
{
	return "BuildBoat";
}

}
