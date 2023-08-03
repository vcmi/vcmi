/*
 * RandomMapInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Structure that describes placement rules for this object in random map
struct DLL_LINKAGE RandomMapInfo
{
	/// How valuable this object is, 1k = worthless, 10k = Utopia-level
	ui32 value;

	/// How many of such objects can be placed on map, 0 = object can not be placed by RMG
	std::optional<ui32> mapLimit;

	/// How many of such objects can be placed in one zone, 0 = unplaceable
	ui32 zoneLimit;

	/// Rarity of object, 5 = extremely rare, 100 = common
	ui32 rarity;

	RandomMapInfo():
		value(0),
		zoneLimit(0),
		rarity(0)
	{}

	void setMapLimit(ui32 val) { mapLimit = val; }

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & value;
		h & mapLimit;
		h & zoneLimit;
		h & rarity;
	}
};

VCMI_LIB_NAMESPACE_END
