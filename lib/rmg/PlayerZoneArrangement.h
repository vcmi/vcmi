/*
 * PlayerZoneArrangement.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CRmgTemplate.h"
#include "float3.h"
#include "../GameConstants.h"

#include <map>
#include <utility>
#include <vector>

/// One player start zone, reduced to what its place in the layout depends on.
struct DLL_LINKAGE PlayerZoneInfo
{
	TRmgTemplateZoneId id = 0;
	int owner = 0;             // 1-based player index from the template
	bool participates = false; // the owner is a player in this game
	int level = 0;
	int size = 0;              // prescaled size
	float3 center;
	std::vector<std::pair<TRmgTemplateZoneId, rmg::EConnectionType>> links; // other zone, connection type
};

/// Groups of zones that can trade places without changing any connection. Groups of one are left out.
DLL_LINKAGE std::vector<std::vector<TRmgTemplateZoneId>> findInterchangeableZones(const std::vector<PlayerZoneInfo> & zones);

/// New centers for one interchangeable group; the set of centers is unchanged, only who holds which.
DLL_LINKAGE std::map<TRmgTemplateZoneId, float3> arrangeByPlayerOrder(const std::vector<PlayerZoneInfo> & group);

struct DLL_LINKAGE ZonePairDistance
{
	float distance = 0.f; // between the two centers
	float touching = 0.f; // distance at which the two zones just touch
};

/// Mean of touching / distance over the pairs, 0 for no pairs. Lower means starts are further apart.
DLL_LINKAGE float playerCrowding(const std::vector<ZonePairDistance> & pairs);

/// How hard two player start zones push each other away. 1 unless both owners are in the game.
DLL_LINKAGE float playerPairWeight(int ownerA, int ownerB, bool bothParticipate);
