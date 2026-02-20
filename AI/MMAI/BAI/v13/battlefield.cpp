/*
 * battlefield.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "battle/BattleHex.h"
#include "battle/IBattleInfoCallback.h"
#include "battle/ReachabilityInfo.h"

#include "BAI/v13/battlefield.h"
#include "BAI/v13/hex.h"
#include "common.h"
#include <algorithm>
#include <memory>
#include <ranges>

namespace MMAI::BAI::V13
{
using HA = HexAttribute;
using SA = StackAttribute;
using LT = LinkType;

// A custom hash function must be provided for the adjmap
struct PairHash
{
	std::size_t operator()(const std::pair<si16, si16> & t) const
	{
		auto h0 = std::hash<int>{}(std::get<0>(t));
		auto h1 = std::hash<int>{}(std::get<1>(t));
		return h0 ^ (h1 << 1);
	}
};

namespace
{
	std::unordered_map<std::pair<si16, si16>, bool, PairHash> InitAdjMap()
	{
		auto res = std::unordered_map<std::pair<si16, si16>, bool, PairHash>{};

		for(int id1 = 0; id1 < GameConstants::BFIELD_SIZE; id1++)
		{
			auto hex1 = BattleHex(id1);
			for(auto dir : BattleHex::hexagonalDirections())
			{
				auto hex2 = hex1.cloneInDirection(dir, false);
				res[{hex1.toInt(), hex2.toInt()}] = true;
			}
		}

		return res;
	}
}

Battlefield::Battlefield(const std::shared_ptr<Hexes> & hexes_, const Stacks & stacks_, const AllLinks & allLinks_, const Stack * astack_)
	: hexes(hexes_), stacks(stacks_), allLinks(allLinks_), astack(astack_) {};

// static
std::shared_ptr<const Battlefield> Battlefield::Create(
	const CPlayerBattleCallback * battle,
	const CStack * acstack,
	const GlobalStats * oldgstats,
	const GlobalStats * gstats,
	std::map<const CStack *, Stack::Stats> & stacksStats,
	bool isMorale
)
{
	auto [stacks, queue] = InitStacks(battle, acstack, oldgstats, gstats, stacksStats, isMorale);
	auto [hexes, astack] = InitHexes(battle, acstack, stacks);
	auto links = InitAllLinks(battle, stacks, queue, hexes);

	return std::make_shared<const Battlefield>(hexes, stacks, links, astack);
}

// static
// result is a vector<UnitID>
// XXX: there is a bug in VCMI when high morale occurs:
//      - the stack acts as if it's already the next unit's turn
//      - as a result, QueuePos for the ACTIVE stack is non-0
//        while the QueuePos for the next (non-active) stack is 0
// (this applies only to good morale; bad morale simply skips turn)
// As a workaround, a "isMorale" flag is passed whenever the astack is
// acting because of high morale and queue is "shifted" accordingly.
Queue Battlefield::GetQueue(const CPlayerBattleCallback * battle, const CStack * astack, bool isMorale)
{
	auto res = Queue{};

	auto tmp = std::vector<battle::Units>{};
	battle->battleGetTurnOrder(tmp, S13::STACK_QUEUE_SIZE, 0);
	for(auto & units : tmp)
	{
		for(auto & unit : units)
		{
			if(res.size() < S13::STACK_QUEUE_SIZE)
				res.push_back(unit->unitId());
			else
				break;
		}
	}

	// XXX: after morale, battleGetTurnOrder() returns wrong order
	//      (where a non-active stack is first)
	//      The active stack *must* be first-in-queue
	if(isMorale && astack && res.at(0) != astack->unitId())
	{
		// logAi->debug("Morale triggered -- will rearrange stack queue");
		std::rotate(res.rbegin(), res.rbegin() + 1, res.rend());
		res.at(0) = astack->unitId();
	}
	else
	{
		// the only scenario where the active stack is not first in queue
		// is at battle end (i.e. no active stack)
		// assert(astack == nullptr || res.at(0) == astack->unitId());
		ASSERT(astack == nullptr || res.at(0) == astack->unitId(), "queue[0] is not the currently active stack!");
	}

	return res;
}

// static
std::tuple<std::shared_ptr<Hexes>, Stack *> Battlefield::InitHexes(const CPlayerBattleCallback * battle, const CStack * acstack, const Stacks & stacks)
{
	auto res = std::make_shared<Hexes>();
	auto ainfo = battle->getAccessibility();
	auto hexstacks = std::map<BattleHex, std::shared_ptr<Stack>>{};
	auto hexobstacles = std::array<std::vector<std::shared_ptr<const CObstacleInstance>>, 165>{};

	std::shared_ptr<ActiveStackInfo> astackinfo = nullptr;
	Stack * astack = nullptr;

	for(const auto & stack : stacks)
	{
		for(const auto & bh : stack->cstack->getHexes())
			if(bh.isAvailable())
				hexstacks.try_emplace(bh, stack);

		// XXX: at battle_end, stack->cstack != acstack even if qpos=0
		if((stack->attr(SA::QUEUE) & 1) && acstack)
			astack = stack.get();
	}

	for(const auto & obstacle : battle->battleGetAllObstacles())
		for(const auto & bh : obstacle->getAffectedTiles())
			if(bh.isAvailable())
				hexobstacles.at(Hex::CalcId(bh)).push_back(obstacle);

	if(astack)
	{
		// astack can be nullptr if battle just begun (no turns yet)
		astackinfo = std::make_shared<ActiveStackInfo>(astack, battle->battleCanShoot(astack->cstack), std::make_shared<ReachabilityInfo>(astack->rinfo));
	}

	auto gatestate = battle->battleGetGateState();

	for(int y = 0; y < 11; ++y)
	{
		for(int x = 0; x < 15; ++x)
		{
			auto i = (y * 15) + x;
			auto bh = BattleHex(x + 1, y);
			res->at(y).at(x) = std::make_unique<Hex>(bh, ainfo.at(bh.toInt()), gatestate, hexobstacles.at(i), hexstacks, astackinfo);
		}
	}

	// XXX: astack can be nullptr (even if acstack is not) -- see above
	return {res, astack};
};

// static
std::tuple<Stacks, Queue> Battlefield::InitStacks(
	const CPlayerBattleCallback * battle,
	const CStack * astack,
	const GlobalStats * oldgstats,
	const GlobalStats * gstats,
	std::map<const CStack *, Stack::Stats> & stacksStats,
	bool isMorale
)
{
	auto stacks = Stacks{};
	auto cstacks = battle->battleGetStacks();

	// Sorting needed to ensure ordered insertion of summons/machines
	std::ranges::sort(
		cstacks,
		[](const CStack * a, const CStack * b)
		{
			return a->unitId() < b->unitId();
		}
	);

	/*
	 * Units for each side are indexed as follows:
	 *
	 *  1. The 7 "regular" army stacks use indexes 0..6 (index=slot)
	 *  2. Up to N* summoned units will use indexes 7+ (ordered by unit ID)
	 *  3. Up to N* war machines will use FREE indexes 7+, if any (ordered by unit ID).
	 *  4. Remaining units from 2. and 3. will use FREE indexes from 1, if any (ordered by unit ID).
	 *  5. Remaining units from 4. will be ignored.
	 */
	auto queue = GetQueue(battle, astack, isMorale);
	auto summons = std::array<std::deque<const CStack *>, 2>{};
	auto machines = std::array<std::deque<const CStack *>, 2>{};

	auto blocking = std::map<const CStack *, bool>{};
	auto blocked = std::map<const CStack *, bool>{};

	auto setBlockedBlocking = [&battle, &blocked, &blocking](const CStack * cstack)
	{
		blocked.emplace(cstack, false);
		blocking.emplace(cstack, false);

		for(const auto * adjacent : battle->battleAdjacentUnits(cstack))
		{
			if(adjacent->unitOwner() == cstack->unitOwner())
				continue;

			if(!blocked[cstack] && cstack->canShoot() && !cstack->hasBonusOfType(BonusType::FREE_SHOOTING) && !cstack->hasBonusOfType(BonusType::SIEGE_WEAPON))
			{
				blocked[cstack] = true;
			}
			if(!blocking[cstack] && adjacent->canShoot() && !adjacent->hasBonusOfType(BonusType::FREE_SHOOTING)
			   && !adjacent->hasBonusOfType(BonusType::SIEGE_WEAPON))
			{
				blocking[cstack] = true;
			}
		}
	};

	// estimated dmg by active stack
	// values are for ranged attack if unit is an unblocked shooter
	// otherwise for melee attack
	auto estdmg = std::map<const CStack *, DamageEstimation>{};

	auto estimateDamage = [&battle, &astack, &estdmg, &blocked](const CStack * cstack)
	{
		if(!astack)
		{
			// no active stack (e.g. called during battleStart or battleEnd)
			estdmg.try_emplace(cstack);
		}
		else if(astack->unitSide() == cstack->unitSide())
		{
			// no damage to friendly units
			estdmg.try_emplace(cstack);
		}
		else
		{
			const auto attinfo = BattleAttackInfo(astack, cstack, 0, astack->canShoot() && !blocked[astack]);
			estdmg.try_emplace(cstack, battle->calculateDmgRange(attinfo));
		}
	};

	// This must be pre-set as dmg estimation depends on it
	if(astack)
		setBlockedBlocking(astack);

	for(auto & cstack : cstacks)
	{
		if(cstack != astack)
			setBlockedBlocking(cstack);

		estimateDamage(cstack);

		auto stack = std::make_shared<Stack>(
			cstack,
			queue,
			// a blank stackStats entry is created if missing
			Stack::StatsContainer{.oldgstats = oldgstats, .gstats = gstats, .stackStats = stacksStats[cstack]},
			battle->getReachability(cstack),
			blocked[cstack],
			blocking[cstack],
			estdmg[cstack]
		);

		stacks.push_back(stack);
	}

	return {stacks, queue};
}

// static
AllLinks Battlefield::InitAllLinks(const CPlayerBattleCallback * battle, const Stacks & stacks, const Queue & queue, std::shared_ptr<Hexes> & hexes)
{
	auto allLinks = AllLinks();

	for(auto i = 0; i < EI(LT::_count); ++i)
		allLinks[static_cast<LT>(i)] = std::make_shared<Links>();

	for(auto & srcrow : *hexes)
	{
		for(auto & srchex : srcrow)
		{
			for(auto & dstrow : *hexes)
			{
				for(auto & dsthex : dstrow)
				{
					LinkTwoHexes(allLinks, battle, stacks, queue, srchex.get(), dsthex.get());
				}
			}
		}
	}

	return allLinks;
}

namespace
{
	float calculateRangeMod(const CPlayerBattleCallback * battle, const CStack * cstack, const BattleHex & src, const BattleHex & dst)
	{
		float rangemod = 1;
		if(battle->battleHasDistancePenalty(cstack, src, dst))
			rangemod *= 0.5;
		if(battle->battleHasWallPenalty(cstack, src, dst))
			rangemod *= 0.5;
		return rangemod;
	}
}

void Battlefield::LinkTwoHexes(
	AllLinks & allLinks,
	const CPlayerBattleCallback * battle,
	const Stacks & stacks,
	const Queue & queue,
	const Hex * src,
	const Hex * dst
)
{
	static const auto adjmap = InitAdjMap();
	bool neighbour = adjmap.contains({src->bhex.toInt(), dst->bhex.toInt()});
	bool reachable = false;
	float rangemod = 0;
	float rangedDmgFrac = 0;
	float meleeDmgFrac = 0;
	float retalDmgFrac = 0;
	int actsBefore = 0;

	if(src->stack && !src->getAttr(HA::IS_REAR) && !src->stack->flag(StackFlag1::SLEEPING))
	{
		reachable = src->stack->rinfo.distances.at(dst->bhex.toInt()) <= src->stack->attr(SA::SPEED);

		// rangemod is set even if dst is free
		if(src->stack->cstack->canShoot() && !src->stack->cstack->coversPos(dst->bhex) && !src->stack->flag(StackFlag1::BLOCKED) && !neighbour)
		{
			rangemod = calculateRangeMod(battle, src->stack->cstack, src->bhex, dst->bhex);
		}

		// *dmgFracs are set only between opposing stacks
		if(dst->stack && (dst->stack->cstack->unitSide() != src->stack->cstack->unitSide()))
		{
			if(rangemod > 0)
			{
				auto estdmg = battle->calculateDmgRange(BattleAttackInfo(src->stack->cstack, dst->stack->cstack, 0, true));
				auto avgdmg = 0.5 * (estdmg.damage.max + estdmg.damage.min);
				// negate the rangemod in the dmg calc (i.e. report the "base" dmg)
				avgdmg *= 1 / rangemod;
				rangedDmgFrac = avgdmg / dst->stack->cstack->getAvailableHealth();
			}

			auto bai = BattleAttackInfo(src->stack->cstack, dst->stack->cstack, 0, false);
			auto retdmg = DamageEstimation{};
			auto estdmg = battle->battleEstimateDamage(bai, &retdmg);
			auto avgdmg = 0.5 * (estdmg.damage.max + estdmg.damage.min);
			meleeDmgFrac = avgdmg / dst->stack->cstack->getAvailableHealth();

			if(retdmg.damage.max > 0)
			{
				auto avgret = 0.5 * (retdmg.damage.max + retdmg.damage.min);
				retalDmgFrac = avgret / src->stack->cstack->getAvailableHealth();
			}
		}
	}

	if(src->stack && dst->stack && src->id != dst->id)
	{
		auto srcpos = src->stack->qposFirst;
		auto dstpos = dst->stack->qposFirst;
		if(srcpos < dstpos)
		{
			ASSERT(dstpos <= queue.size(), "dstpos exceeds queue size");
			actsBefore = true;
		}
	}

	//
	// Build links
	//

	if(neighbour)
		allLinks[LT::ADJACENT]->add(src->id, dst->id, 1);

	if(reachable)
		allLinks[LT::REACH]->add(src->id, dst->id, 1);

	if(actsBefore)
		allLinks[LT::ACTS_BEFORE]->add(src->id, dst->id, std::min<int>(2, actsBefore));

	if(rangemod)
		allLinks[LT::RANGED_MOD]->add(src->id, dst->id, std::min<float>(2, rangemod));

	if(rangedDmgFrac)
		allLinks[LT::RANGED_DMG_REL]->add(src->id, dst->id, std::min<float>(2, rangedDmgFrac));

	if(meleeDmgFrac)
		allLinks[LT::MELEE_DMG_REL]->add(src->id, dst->id, std::min<float>(2, meleeDmgFrac));

	if(retalDmgFrac)
		allLinks[LT::RETAL_DMG_REL]->add(src->id, dst->id, std::min<float>(2, retalDmgFrac));
}
}
