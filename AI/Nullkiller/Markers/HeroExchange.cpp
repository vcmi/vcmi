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
#include "HeroExchange.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"
#include "../Analyzers/ArmyManager.h"

namespace NKAI
{

using namespace Goals;

bool HeroExchange::operator==(const HeroExchange & other) const
{
	return false;
}

std::string HeroExchange::toString() const
{
	return "Hero exchange " + exchangePath.toString();
}

uint64_t HeroExchange::getReinforcementArmyStrength() const
{
	uint64_t armyValue = ai->nullkiller->armyManager->howManyReinforcementsCanGet(hero.get(), exchangePath.heroArmy);

	return armyValue;
}

}
