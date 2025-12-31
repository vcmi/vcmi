/*
 * CRoadRandomizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

class RmgMap;

class CRoadRandomizer
{
public:
	explicit CRoadRandomizer(RmgMap & map);
	~CRoadRandomizer() = default;

	void dropRandomRoads(vstd::RNG * rand);

private:
	RmgMap & map;
};

// Helper functions for Union-Find (Disjoint Set Union)
TRmgTemplateZoneId findSet(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId x);
void unionSets(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId x, TRmgTemplateZoneId y);

VCMI_LIB_NAMESPACE_END