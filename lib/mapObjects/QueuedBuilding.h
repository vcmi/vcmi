/*
 * QueuedBuilding.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../ResourceSet.h"

/// A building queued for future construction in a town, together with the resources
/// that were actually charged when it was queued - refunds always use this locked-in
/// cost rather than the building's current price, so a later price change (mods, difficulty,
/// bonuses) can never over- or under-refund a cancelled or lost queue entry.
struct DLL_LINKAGE QueuedBuilding
{
	BuildingID building;
	TResources resources;

	QueuedBuilding() = default;
	QueuedBuilding(const BuildingID & building, const TResources & resources)
		: building(building)
		, resources(resources)
	{
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & building;
		h & resources;
	}
};
