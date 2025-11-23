/*
* HeroExchange.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExplorationPoint.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"
#include "../Analyzers/ArmyManager.h"

namespace NK2AI
{

using namespace Goals;

bool ExplorationPoint::operator==(const ExplorationPoint & other) const
{
	return false;
}

std::string ExplorationPoint::toString() const
{
	return "Explore " +tile.toString() + " for " + std::to_string(value) + " tiles";
}

}
