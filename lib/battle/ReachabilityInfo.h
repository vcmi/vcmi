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
#include "BattleHexArray.h"
#include "CBattleInfoEssentials.h"
#include "AccessibilityInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleHexArray;

// Reachability info is result of BFS calculation. It's dependent on stack (it's owner, whether it's flying),
// startPosition and perspective.
struct DLL_LINKAGE ReachabilityInfo
{
	using TDistances = std::array<uint32_t, GameConstants::BFIELD_SIZE>;
	using TPredecessors = std::array<BattleHex, GameConstants::BFIELD_SIZE>;

	enum { INFINITE_DIST = 1000000 };

	struct DLL_LINKAGE Parameters
	{
		BattleSide side = BattleSide::NONE;
		bool doubleWide = false;
		bool flying = false;
		bool ignoreKnownAccessible = false; //Ignore obstacles if it is in accessible hexes
		bool bypassEnemyStacks = false; // in case of true will count amount of turns needed to kill enemy and thus move forward
		BattleHexArray knownAccessible; //hexes that will be treated as accessible, even if they're occupied by stack (by default - tiles occupied by stack we do reachability for, so it doesn't block itself)
		std::map<BattleHex, ui8> destructibleEnemyTurns; // hom many turns it is needed to kill enemy on specific hex

		BattleHex startPosition; //assumed position of stack
		BattleSide perspective = BattleSide::ALL_KNOWING; //some obstacles (eg. quicksands) may be invisible for some side

		Parameters() = default;
		Parameters(const battle::Unit * Stack, BattleHex StartPosition);
	};

	Parameters params;
	AccessibilityInfo accessibility;
	TDistances distances;
	TPredecessors predecessors;

	ReachabilityInfo();

	bool isReachable(BattleHex hex) const;

	uint32_t distToNearestNeighbour(
		const BattleHexArray & targetHexes,
		BattleHex * chosenHex = nullptr) const;

	uint32_t distToNearestNeighbour(
		const battle::Unit * attacker,
		const battle::Unit * defender,
		BattleHex * chosenHex = nullptr) const;
};

VCMI_LIB_NAMESPACE_END
