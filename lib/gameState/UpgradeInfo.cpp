/*
 * UpgradeInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "UpgradeInfo.h"
#include "CCreatureHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void UpgradeInfo::addUpgrade(const CreatureID & upgradeID, const Creature * creature, int costPercentageModifier)
{
	isAvailable = costPercentageModifier >= 0;

	upgradesIDs.push_back(upgradeID);

	ResourceSet upgradeCost = (upgradeID.toCreature()->getFullRecruitCost() - creature->getFullRecruitCost()) * costPercentageModifier / 100;
	upgradeCost.positive(); //upgrade cost can't be negative, ignore missing resources
	upgradesCosts.push_back(std::move(upgradeCost));

	// sort from highest ID to smallest
	size_t pos = upgradesIDs.size() - 1;
	while(pos > 0 && upgradesIDs[pos] > upgradesIDs[pos - 1])
	{
		std::swap(upgradesIDs[pos], upgradesIDs[pos - 1]);
		std::swap(upgradesCosts[pos], upgradesCosts[pos - 1]);
		--pos;
	}
}

VCMI_LIB_NAMESPACE_END
