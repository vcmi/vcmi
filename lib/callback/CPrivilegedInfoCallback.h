/*
 * CPrivilegedInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
	class RNG;
}

class DLL_LINKAGE CPrivilegedInfoCallback : public CGameInfoCallback
{
public:
	using CGameInfoCallback::gameState; // make public

	//used for random spawns
	void getFreeTiles(std::vector<int3> &tiles) const;

	//mode 1 - only unrevealed tiles; mode 0 - all, mode -1 -  only revealed
	void getTilesInRange(std::unordered_set<int3> & tiles,
						 const int3 & pos,
						 int radius,
						 ETileVisibility mode,
						 std::optional<PlayerColor> player = std::optional<PlayerColor>(),
						 int3::EDistanceFormula formula = int3::DIST_2D) const;

	//returns all tiles on given level (-1 - both levels, otherwise number of level)
	void getAllTiles(std::unordered_set<int3> &tiles, std::optional<PlayerColor> player, int level, std::function<bool(const TerrainTile *)> filter) const;

	//gives 3 treasures, 3 minors, 1 major -> used by Black Market and Artifact Merchant
	void pickAllowedArtsSet(std::vector<ArtifactID> & out, vstd::RNG & rand);
	void getAllowedSpells(std::vector<SpellID> &out, std::optional<ui16> level = std::nullopt);
};

VCMI_LIB_NAMESPACE_END
