/*
 * battlefield.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "battle/CPlayerBattleCallback.h"

#include "BAI/v13/hex.h"
#include "BAI/v13/links.h"
#include "BAI/v13/stack.h"

namespace MMAI::BAI::V13
{

using LinkType = Schema::V13::LinkType;
using Stacks = std::vector<std::shared_ptr<Stack>>;
using Hexes = std::array<std::array<std::unique_ptr<Hex>, 15>, 11>;
using AllLinks = std::map<LinkType, std::shared_ptr<Links>>;

using XY = std::pair<int, int>;

class Battlefield
{
public:
	static std::shared_ptr<const Battlefield> Create(
		const CPlayerBattleCallback * battle,
		const CStack * acstack,
		const GlobalStats * oldgstats,
		const GlobalStats * gstats,
		std::map<const CStack *, Stack::Stats> & stacksStats,
		bool isMorale
	);

	Battlefield(const std::shared_ptr<Hexes> & hexes, const Stacks & stacks, const AllLinks & allLinks, const Stack * astack);

	const std::shared_ptr<Hexes> hexes;
	const Stacks stacks;
	const AllLinks allLinks;
	const Stack * const astack; // XXX: nullptr on battle start/end, or if army stacks > MAX_STACKS_PER_SIDE
private:
	static std::tuple<Stacks, Queue> InitStacks(
		const CPlayerBattleCallback * battle,
		const CStack * astack,
		const GlobalStats * oldgstats,
		const GlobalStats * gstats,
		std::map<const CStack *, Stack::Stats> & stacksStats,
		bool isMorale
	);

	static std::tuple<std::shared_ptr<Hexes>, Stack *> InitHexes(const CPlayerBattleCallback * battle, const CStack * acstack, const Stacks & stacks);

	static AllLinks InitAllLinks(const CPlayerBattleCallback * battle, const Stacks & stacks, const Queue & queue, std::shared_ptr<Hexes> & hexes);

	static void
	LinkTwoHexes(AllLinks & allLinks, const CPlayerBattleCallback * battle, const Stacks & stacks, const Queue & queue, const Hex * src, const Hex * dst);

	static Queue GetQueue(const CPlayerBattleCallback * battle, const CStack * astack, bool isMorale);
};
}
