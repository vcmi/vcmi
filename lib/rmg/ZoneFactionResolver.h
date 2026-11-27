/*
 * ZoneFactionResolver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../constants/EntityIdentifiers.h"

class RmgMap;
class Zone;

class ZoneFactionResolver
{
public:
	explicit ZoneFactionResolver(RmgMap & map);

	void run();

private:
	bool hasHint(const Zone & zone, size_t hintIndex) const;
	FactionID fromHint(Zone & zone, size_t hintIndex);
	FactionID randomTownType(Zone & zone, bool matchUndergroundType = false) const;
	FactionID mainFaction(Zone & zone);
	void initTerrainType(Zone & zone) const;

	RmgMap & map;
};
