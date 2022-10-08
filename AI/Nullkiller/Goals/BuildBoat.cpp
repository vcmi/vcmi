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
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../Behaviors/CaptureObjectsBehavior.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

bool BuildBoat::operator==(const BuildBoat & other) const
{
	return shipyard->o->id == other.shipyard->o->id;
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

std::string BuildBoat::toString() const
{
	return "BuildBoat";
}

}
