/*
 * action.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BAI/v13/battlefield.h"
#include "BAI/v13/hex.h"
#include "BAI/v13/hexaction.h"

namespace MMAI::BAI::V13
{
/*
 * Wrapper around Schema::Action
 */
struct Action
{
	static std::unique_ptr<Hex> initHex(const Schema::Action & a, const Battlefield * bf);
	static std::unique_ptr<Hex> initAMoveTargetHex(const Schema::Action & a, const Battlefield * bf);
	static HexAction initHexAction(const Schema::Action & a, const Battlefield * bf);

	Action(Schema::Action action_, const Battlefield * bf, const std::string & color);

	const Schema::Action action;
	const std::unique_ptr<Hex> hex;
	const std::unique_ptr<Hex> aMoveTargetHex;
	const HexAction hexaction; // XXX: must come after action
	const std::string color;

	std::string name() const;
};
}
