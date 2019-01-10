/*
* Trade.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Trade.h"

using namespace Goals;

bool Trade::operator==(const Trade & other) const
{
	return resID == other.resID;
}

TSubgoal Trade::whatToDoToAchieve()
{
	return iAmElementary();
}
