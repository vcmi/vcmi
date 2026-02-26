/*
 * action.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "battle/CBattleInfoEssentials.h"

#include "BAI/v13/action.h"
#include "BAI/v13/hex.h"
#include "BAI/v13/hexaction.h"
#include "common.h"

namespace MMAI::BAI::V13
{

// static
std::unique_ptr<Hex> Action::initHex(const Schema::Action & a, const Battlefield * bf)
{
	// Control actions (<0) should never reach here
	ASSERT(a >= 0 && a < N_ACTIONS, "Invalid action: " + std::to_string(a));

	auto i = a - EI(GlobalAction::_count);

	if(i < 0)
		return nullptr;

	i = i / EI(HexAction::_count);
	auto y = i / 15;
	auto x = i % 15;

	// create a new unique_ptr with a copy of Hex
	return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
}

// static
std::unique_ptr<Hex> Action::initAMoveTargetHex(const Schema::Action & a, const Battlefield * bf)
{
	auto hex = initHex(a, bf);
	if(!hex)
		return nullptr;

	auto ha = initHexAction(a, bf);
	if(EI(ha) == -1)
		return nullptr;

	if(ha == HexAction::MOVE || ha == HexAction::SHOOT)
		return nullptr;
	// throw std::runtime_error("MOVE and SHOOT are not AMOVE actions");

	const auto & bh = hex->bhex;

	auto edir = AMOVE_TO_EDIR.at(EI(ha));
	auto nbh = bh.cloneInDirection(edir);

	ASSERT(nbh.isAvailable(), "unavailable AMOVE target hex #" + std::to_string(nbh.toInt()));

	auto [x, y] = Hex::CalcXY(nbh);

	// create a new unique_ptr with a copy of Hex
	return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
}

// static
HexAction Action::initHexAction(const Schema::Action & a, const Battlefield * bf)
{
	if(a < EI(GlobalAction::_count))
		return static_cast<HexAction>(-1); // a is not about a hex
	return static_cast<HexAction>((a - EI(GlobalAction::_count)) % EI(HexAction::_count));
}

Action::Action(const Schema::Action action_, const Battlefield * bf, const std::string & color_)
	: action(action_), hex(initHex(action_, bf)), aMoveTargetHex(initAMoveTargetHex(action_, bf)), hexaction(initHexAction(action_, bf)), color(color_)
{
}

std::string Action::name() const
{
	if(action == Schema::V13::ACTION_RETREAT)
		return "Retreat";
	else if(action == Schema::V13::ACTION_WAIT)
		return "Wait";

	ASSERT(hex, "hex is null");

	auto ha = static_cast<HexAction>((action - EI(GlobalAction::_count)) % EI(HexAction::_count));
	auto res = std::string{};
	std::shared_ptr<const Stack> stack = nullptr;
	std::string stackstr;

	if(ha == HexAction::SHOOT || ha == HexAction::MOVE)
	{
		stack = hex->stack;
	}
	else if(aMoveTargetHex)
	{
		stack = aMoveTargetHex->stack;
	}

	// colored output does not look good (scrambles default VCMI log coloring)
	// Additionally, hardcoded red/blue colors are relevant during training only
	// => replace with attacker/defender colorless strings instead
	// if (stack) {
	//     std::string targetcolor = "\033[31m";  // red
	//     if (color == "red") targetcolor = "\033[34m"; // blue
	//     stackstr = targetcolor + "#" + std::string(1, stack->getAlias()) + "\033[0m";
	// } else {
	//     std::string targetcolor = "\033[7m";  // white
	//     stackstr = targetcolor + "#?" + "\033[0m";
	// }

	if(stack)
	{
		std::string targetside = (color == "red") ? "L" : "R";
		stackstr = targetside + "-" + std::string(1, stack->getAlias());
	}
	else
	{
		stackstr = "?";
	}

	switch(ha)
	{
		case HexAction::MOVE:
			res = (stack && hex->bhex == stack->cstack->getPosition() ? "Defend on hex(" : "Move to (") + hex->name() + ")";
			break;
		case HexAction::AMOVE_TL:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /top-left/";
			break;
		case HexAction::AMOVE_TR:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /top-right/";
			break;
		case HexAction::AMOVE_R:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /right/";
			break;
		case HexAction::AMOVE_BR:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /bottom-right/";
			break;
		case HexAction::AMOVE_BL:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /bottom-left/";
			break;
		case HexAction::AMOVE_L:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /left/";
			break;
		case HexAction::AMOVE_2BL:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /bottom-left-2/";
			break;
		case HexAction::AMOVE_2L:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /left-2/";
			break;
		case HexAction::AMOVE_2TL:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /top-left-2/";
			break;
		case HexAction::AMOVE_2TR:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /top-right-2/";
			break;
		case HexAction::AMOVE_2R:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /right-2/";
			break;
		case HexAction::AMOVE_2BR:
			res = "Attack stack(" + stackstr + ") from hex(" + hex->name() + ") /bottom-right-2/";
			break;
		case HexAction::SHOOT:
			res = "Attack stack(" + stackstr + ") " + hex->name() + " (ranged)";
			break;
		default:
			THROW_FORMAT("Unexpected hexaction: %d", EI(ha));
	}

	return res;
}

}
