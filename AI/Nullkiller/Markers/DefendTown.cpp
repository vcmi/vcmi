/*
* ArmyUpgrade.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "DefendTown.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"

namespace NKAI
{

using namespace Goals;

DefendTown::DefendTown(const CGTownInstance * town, const HitMapInfo & treat, const AIPath & defencePath, bool isContrattack)
	: CGoal(Goals::DEFEND_TOWN), treat(treat), defenceArmyStrength(defencePath.getHeroStrength()), turn(defencePath.turn()), contrattack(isContrattack)
{
	settown(town);
	sethero(defencePath.targetHero);
}

DefendTown::DefendTown(const CGTownInstance * town, const HitMapInfo & treat, const CGHeroInstance * defender)
	: CGoal(Goals::DEFEND_TOWN), treat(treat), defenceArmyStrength(defender->getTotalStrength()), turn(0)
{
	settown(town);
	sethero(defender);
}

bool DefendTown::operator==(const DefendTown & other) const
{
	return false;
}

std::string DefendTown::toString() const
{
	return "Defend town " + town->getObjectName();
}

}
