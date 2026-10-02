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

void UpgradeInfo::addUpgrade(const CreatureID & upgradeID, const Creature * creature, int costPercentageModifier, bool discountGoldOnly)
{
	const bool upgradeAvaiable = costPercentageModifier >= 0;

	const ResourceSet fullUpgradeCost = upgradeID.toCreature()->getFullRecruitCost() - creature->getFullRecruitCost();
	ResourceSet upgradeCost = fullUpgradeCost * costPercentageModifier / 100;

	if(discountGoldOnly)
	{
		for(size_t i = 0; i < upgradeCost.size(); ++i)
			if(i != GameResID(EGameResID::GOLD).getNum())
				upgradeCost[i] = fullUpgradeCost[i];
	}
	upgradeCost.positive(); //upgrade cost can't be negative, ignore missing resources

	auto idIt = std::find(upgradesIDs.begin(), upgradesIDs.end(), upgradeID);
	if(idIt != upgradesIDs.end())
	{
		auto pos = std::distance(upgradesIDs.begin(), idIt);
		ResourceSet & existingCost = upgradesCosts[pos];

		// Prefer an available offer over an unavailable one;
		// otherwise replace only with an offer no more expensive in any resource.
		if (upgradeAvaiable && (!isAvailable || existingCost.canAfford(upgradeCost)))
		{
			existingCost = upgradeCost;
			isAvailable = true;
		}

		return;
	}

	isAvailable = upgradeAvaiable;
	upgradesIDs.push_back(upgradeID);
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
