/*
 * ClassicRulesAdapter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <cstdint>
#include <memory>
#include "../../../lib/GameConstants.h"
#include "../../../lib/battle/BattleSide.h"

class CBattleInfoCallback;

namespace battle
{
class Unit;
}

/// SoD rule queries used by classic BattleAI when native VCMI intentionally
/// represents the same battle object differently.
class ClassicRulesAdapter
{
public:
	static int32_t attack(const battle::Unit * unit, bool ranged);
	static int32_t defense(const battle::Unit * unit);
	static int32_t minDamage(const battle::Unit * unit, bool ranged);
	static int32_t maxDamage(const battle::Unit * unit, bool ranged);
	static uint32_t movementRangeAfterBindCleanup(
		const CBattleInfoCallback & battle,
		const battle::Unit * unit);
	static bool failedSiege(const std::shared_ptr<CBattleInfoCallback> & battle, BattleSide side);
};
