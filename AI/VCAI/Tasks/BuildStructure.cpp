/*
* Goals.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "../VCAI.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "BuildStructure.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Tasks;

void BuildStructure::execute()
{
	cb->buildBuilding(town, BuildingID(buildingID));
}

std::string BuildStructure::toString()
{
	return "BuildStructure " + std::to_string(buildingID) + " in " + town->name;
}