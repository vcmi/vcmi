/*
 * ReachabilityInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "BattleHex.h"
#include "CBattleInfoEssentials.h"
#include "AccessibilityInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

// Reachability info is result of BFS calculation. It's dependent on stack (it's owner, whether it's flying),
// startPosition and perpective.
struct DLL_LINKAGE ReachabilityInfo
{
	typedef std::array<uint32_t, GameConstants::BFIELD_SIZE> TDistances;
	typedef std::array<BattleHex, GameConstants::BFIELD_SIZE> TPredecessors;

	enum { INFINITE_DIST = 1000000 };

	struct DLL_LINKAGE Parameters
	{
		ui8 side{0};
		bool doubleWide;
		bool flying;
		std::vector<BattleHex> knownAccessible; //hexes that will be treated as accessible, even if they're occupied by stack (by default - tiles occupied by stack we do reachability for, so it doesn't block itself)

		BattleHex startPosition; //assumed position of stack
		BattlePerspective::BattlePerspective perspective{BattlePerspective::ALL_KNOWING}; //some obstacles (eg. quicksands) may be invisible for some side

		Parameters();
		Parameters(const battle::Unit * Stack, BattleHex StartPosition);
	};

	Parameters params;
	AccessibilityInfo accessibility{};
	TDistances distances{};
	TPredecessors predecessors;

	ReachabilityInfo();

	bool isReachable(BattleHex hex) const;

	uint32_t distToNearestNeighbour(
		const std::vector<BattleHex> & targetHexes,
		BattleHex * chosenHex = nullptr) const;

	uint32_t distToNearestNeighbour(
		const battle::Unit * attacker,
		const battle::Unit * defender,
		BattleHex * chosenHex = nullptr) const;
};

VCMI_LIB_NAMESPACE_END
