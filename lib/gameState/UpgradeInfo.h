/*
 * UpgradeInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE UpgradeInfo
{
public:
	UpgradeInfo() = delete;
	UpgradeInfo(CreatureID base)
		: oldID(base), isAvailable(true)
	{
	}

	CreatureID oldID; //creature to be upgraded

	const std::vector<CreatureID> & getAvailableUpgrades() const
	{
		return upgradesIDs;
	}

	const CreatureID & getUpgrade() const
	{
		return upgradesIDs.back();
	}

	const ResourceSet & getUpgradeCostsFor(CreatureID id) const
	{
		auto idIt = std::find(upgradesIDs.begin(), upgradesIDs.end(), id);

		assert(idIt != upgradesIDs.end());

		return upgradesCosts[std::distance(upgradesIDs.begin(), idIt)];
	}

	const std::vector<ResourceSet> & getAvailableUpgradeCosts() const
	{
		return upgradesCosts;
	}

	const ResourceSet & getUpgradeCosts() const
	{
		return upgradesCosts.back();
	}

	bool canUpgrade() const
	{
		return !upgradesIDs.empty() && isAvailable;
	}

	bool hasUpgrades() const
	{
		return !upgradesIDs.empty();
	}

	// Adds a new upgrade and ensures alignment and sorted order
	void addUpgrade(const CreatureID & upgradeID, const Creature * creature, int costPercentageModifier = 100);

	auto size() const
	{
		return upgradesIDs.size();
	}

private:
	std::vector<CreatureID> upgradesIDs; //possible upgrades
	std::vector<ResourceSet> upgradesCosts; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>; cost is for single unit (not entire stack)
	bool isAvailable;		// flag for unavailableUpgrades like in miniHillFort from HoTA
};

VCMI_LIB_NAMESPACE_END

