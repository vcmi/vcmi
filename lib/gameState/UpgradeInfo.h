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
#include <boost/container/small_vector.hpp>

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

	const boost::container::small_vector<CreatureID, 4> & getAvailableUpgrades() const
	{
		static const boost::container::small_vector<CreatureID, 4> emptyUpgrades;
		
		return (isAvailable && !upgradesIDs.empty()) ? upgradesIDs : emptyUpgrades;
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

	const boost::container::small_vector<ResourceSet, 4> & getAvailableUpgradeCosts() const
	{
		static const boost::container::small_vector<ResourceSet, 4> emptyCosts;

		return (isAvailable && !upgradesCosts.empty()) ? upgradesCosts : emptyCosts;
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
	boost::container::small_vector<CreatureID, 4> upgradesIDs; //possible upgrades
	boost::container::small_vector<ResourceSet, 4> upgradesCosts; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>; cost is for single unit (not entire stack)
	bool isAvailable;		// flag for unavailableUpgrades like in miniHillFort from HoTA
};

VCMI_LIB_NAMESPACE_END

