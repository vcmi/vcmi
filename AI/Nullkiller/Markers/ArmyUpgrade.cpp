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
#include "ArmyUpgrade.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"

namespace NKAI
{

extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

ArmyUpgrade::ArmyUpgrade(const AIPath & upgradePath, const CGObjectInstance * upgrader, const ArmyUpgradeInfo & upgrade)
	: CGoal(Goals::ARMY_UPGRADE), upgrader(upgrader), upgradeValue(upgrade.upgradeValue),
	initialValue(upgradePath.heroArmy->getArmyStrength()), goldCost(upgrade.upgradeCost[EGameResID::GOLD])
{
	sethero(upgradePath.targetHero);
}

bool ArmyUpgrade::operator==(const ArmyUpgrade & other) const
{
	return false;
}

std::string ArmyUpgrade::toString() const
{
	return "Army upgrade at " + upgrader->getObjectName() + upgrader->visitablePos().toString();
}

}
