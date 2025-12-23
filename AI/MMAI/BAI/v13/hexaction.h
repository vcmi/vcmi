/*
 * hexaction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "battle/BattleHex.h"

#include "schema/v13/constants.h"
#include "schema/v13/types.h"

// There is a cyclic dependency if those are placed in action.h:
// action.h -> battlefield.h -> hex.h -> actmask.h -> action.h
namespace MMAI::BAI::V13
{
using GlobalAction = Schema::V13::GlobalAction;
using HexAction = Schema::V13::HexAction;

static_assert(EI(HexAction::AMOVE_TR) == 0);
static_assert(EI(HexAction::AMOVE_R) == 1);
static_assert(EI(HexAction::AMOVE_BR) == 2);
static_assert(EI(HexAction::AMOVE_BL) == 3);
static_assert(EI(HexAction::AMOVE_L) == 4);
static_assert(EI(HexAction::AMOVE_TL) == 5);
static_assert(EI(HexAction::AMOVE_2TR) == 6);
static_assert(EI(HexAction::AMOVE_2R) == 7);
static_assert(EI(HexAction::AMOVE_2BR) == 8);
static_assert(EI(HexAction::AMOVE_2BL) == 9);
static_assert(EI(HexAction::AMOVE_2L) == 10);
static_assert(EI(HexAction::AMOVE_2TL) == 11);

constexpr auto AMOVE_TO_EDIR = std::array<BattleHex::EDir, 12>{
	BattleHex::TOP_RIGHT,
	BattleHex::RIGHT,
	BattleHex::BOTTOM_RIGHT,
	BattleHex::BOTTOM_LEFT,
	BattleHex::LEFT,
	BattleHex::TOP_LEFT,
	BattleHex::TOP_RIGHT,
	BattleHex::RIGHT,
	BattleHex::BOTTOM_RIGHT,
	BattleHex::BOTTOM_LEFT,
	BattleHex::LEFT,
	BattleHex::TOP_LEFT,
};

static_assert(EI(GlobalAction::_count) == Schema::V13::N_NONHEX_ACTIONS);
static_assert(EI(HexAction::_count) == Schema::V13::N_HEX_ACTIONS);

constexpr int N_ACTIONS = EI(GlobalAction::_count) + (EI(HexAction::_count) * 165);
}
