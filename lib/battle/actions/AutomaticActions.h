/*
 * AutomaticActions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../BattleAction.h"

class CBattleInfoCallback;
class CStack;
class GameRandomizer;

namespace battle
{
class Unit;
}

/// Actions that a unit makes at the start of its turn without the will of its player
namespace AutomaticActions
{
/// Action that bad morale, berserk or forced control makes for the unit, or nothing if its player acts. Rolls bad morale and manual control
DLL_LINKAGE std::optional<BattleAction> chooseAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer);

/// Action of a unit with ATTACKS_NEAREST_CREATURE: shoots the nearest unit, else attacks the nearest reachable one, else walks toward it.
/// NO_ACTION if it can do none of these
DLL_LINKAGE BattleAction chooseBerserkAction(const CBattleInfoCallback & battle, const battle::Unit & unit);
}
