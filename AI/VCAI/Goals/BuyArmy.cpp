/*
* BuyArmy.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "BuyArmy.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapObjects/CGTownInstance.h"

using namespace Goals;

bool BuyArmy::operator==(const BuyArmy & other) const
{
	return town == other.town && objid == other.objid;
}

bool BuyArmy::fulfillsMe(TSubgoal goal)
{
	//if (hero && hero != goal->hero)
	//	return false;
	return town == goal->town && goal->value >= value; //can always buy more army
}
TSubgoal BuyArmy::whatToDoToAchieve()
{
	//TODO: calculate the actual cost of units instead
	TResources price;
	price[EGameResID::GOLD] = static_cast<int>(value * 0.4f); //some approximate value
	return ai->ah->whatToDo(price, iAmElementar()); //buy right now or gather resources
}

std::string BuyArmy::completeMessage() const
{
	return boost::format("Bought army of value %d in town of %s") % value, town->getNameTranslated();
}
