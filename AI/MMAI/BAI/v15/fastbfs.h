#pragma once

#include "common.h" // IWYU pragma: keep

#include "CStack.h"
#include "battle/AccessibilityInfo.h"
#include "battle/BattleHex.h"
#include "battle/CObstacleInstance.h"
#include "battle/CPlayerBattleCallback.h"
#include "constants/Enumerations.h"

namespace MMAI::BAI::V15
{

struct FastBFS
{
	using dtype = uint8_t; // for debugging use uint16_t (vscode prints uint8_t as char)
	static constexpr dtype INFINITE_DIST = std::numeric_limits<dtype>::max();
	using Distances = std::array<dtype, GameConstants::BFIELD_SIZE>;

	explicit FastBFS(const CPlayerBattleCallback & battle, const AccessibilityInfo & accessibility)
		: moat(FindMoat(battle))
		, isWideMoat(moat && moat->getAffectedTiles().contains(BattleHex::GATE_BRIDGE))
		, isNarrowMoat(moat && !isWideMoat)
		, gatestate(battle.battleGetGateState())
		, accessL1(BuildAccessibilityMask(accessibility, BattleSide::LEFT_SIDE, false))
		, accessL2(BuildAccessibilityMask(accessibility, BattleSide::LEFT_SIDE, true))
		, accessR1(BuildAccessibilityMask(accessibility, BattleSide::RIGHT_SIDE, false))
		, accessR2(BuildAccessibilityMask(accessibility, BattleSide::RIGHT_SIDE, true))
		, stopL1(BuildStopMask(battle, BattleSide::LEFT_SIDE, false, gatestate))
		, stopL2(BuildStopMask(battle, BattleSide::LEFT_SIDE, true, gatestate))
		, stopR1(BuildStopMask(battle, BattleSide::RIGHT_SIDE, false, gatestate))
		, stopR2(BuildStopMask(battle, BattleSide::RIGHT_SIDE, true, gatestate))
	{
	}

	Distances run(const BattleHex oldpos, const BattleHex newpos, const BattleSide side, bool isFlying, bool isWide, int speed) const
	{
		assert(oldpos.isValid());
		assert(newpos.isValid());
		return isFlying ? calcAirReachability(oldpos, newpos, side, isWide) : calcLandReachability(oldpos, newpos, side, isWide, speed);
	}

private:
	using Mask = std::array<bool, GameConstants::BFIELD_SIZE>;
	using TPredecessors = std::array<BattleHex, GameConstants::BFIELD_SIZE>;
	using ObstaclePtr = std::shared_ptr<const CObstacleInstance>;

	static constexpr int16_t I16(int i)
	{
		return static_cast<int16_t>(i);
	}

	static ObstaclePtr FindMoat(const CPlayerBattleCallback & battle)
	{

		for(const auto & o : battle.battleGetAllObstacles(battle.battleGetMySide()))
			if(o->obstacleType == CObstacleInstance::MOAT)
				return o;
		return nullptr;
	};

	static Mask BuildAccessibilityMask(const AccessibilityInfo & accessibility, BattleSide side, bool wide)
	{
		auto mask = Mask{};
		for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
			mask[i] = accessibility.accessible(BattleHex(I16(i)), wide, side);
		return mask;
	}

	static Mask BuildStopMask(const CPlayerBattleCallback & battle, BattleSide side, bool wide, EGateState gatestate)
	{
		auto mask = Mask{};
		mask.fill(false);

		auto stoppers = BattleHexArray();

		// We can only "see" the obstacles visible from our own perspective
		// (regardless which side we are evaluating stoppers for)
		// XXX: stolen from CBattleInfoCallback::getStoppers
		// but modified to fix a bug with gate hex (see notes/moats.txt)
		for(const auto & o : battle.battleGetAllObstacles(battle.battleGetMySide()))
		{
			// XXX: enemies can't see the quicksand, but they will still be stopped
			//      by it => disable this "if"
			// if(!battle.battleIsObstacleVisibleForSide(*o, side))
			//     continue;

			for(const auto & hex : o->getStoppingTile())
			{
				if(hex == BattleHex::GATE_BRIDGE
				   // we only care for wide moat where bridge can cover it (aka. fortress)
				   && o->obstacleType == CObstacleInstance::MOAT

				   // If bridge is not blocked, we only care about attackers
				   // (for defenders the bridge would lower and cover the moat)
				   // However, if blocked, then defenders are also affected
				   // So the hex is non-stopping if:
				   && (gatestate == EGateState::OPENED || gatestate == EGateState::DESTROYED
					   || (gatestate == EGateState::CLOSED && side == BattleSide::DEFENDER)))
				{
					// drawbridge is open (or will open), negating the "stop" nature of the hex
					continue;
				}

				stoppers.insert(hex);
			}
		}

		for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
		{
			BattleHex tile(I16(i));

			if(!tile.isValid())
				continue;

			const BattleHex first = tile;

			if(stoppers.contains(first))
			{
				mask[i] = true;
				continue;
			}

			if(!wide)
				continue;

			const BattleHex second = CStack::occupiedHex(tile, true, side);

			if(second.isValid() && stoppers.contains(second))
			{
				mask[i] = true;
				continue;
			}
		}

		return mask;
	}

	Distances calcAirReachability(
		const BattleHex & oldpos, // actual stack position now
		const BattleHex & newpos, // hypothetical stack position to calculate reachability from
		BattleSide side,
		bool wide
	) const
	{
		auto distances = Distances{};
		distances.fill(INFINITE_DIST);

		const auto & accessible = accessMask(side, wide, oldpos);

		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			if(!accessible[i])
				continue;

			distances[i] = BattleHex::getDistance(newpos, BattleHex(I16(i)));
		}

		return distances;
	}

	Distances calcLandReachability(
		const BattleHex & oldpos, // actual stack position now
		const BattleHex & newpos, // hypothetical stack position to calculate reachability from
		BattleSide side,
		bool wide,
		int speed
	) const
	{
		auto distances = Distances{};
		auto predecessors = TPredecessors{};
		distances.fill(INFINITE_DIST);
		predecessors.fill(BattleHex::INVALID);

		const auto & accessible = accessMask(side, wide, oldpos);
		const auto & stoppers = stopMask(side, wide, newpos);
		const auto start = newpos.toInt();

		// Start may be occupied by the moving unit itself, so do not require accessible[startIndex].
		distances[start] = 0;

		std::array<BattleHex, GameConstants::BFIELD_SIZE> queue;
		size_t head = 0;
		size_t tail = 0;

		assert(tail < queue.size());
		queue[tail++] = newpos;

		while(head != tail)
		{
			const BattleHex curHex = queue[head++];
			const int cur = curHex.toInt();
			const dtype curDist = distances[cur];

			if(curDist >= speed)
				continue;

			// Walking stack cannot step past obstacles.
			// This preserves the old behavior: the obstacle tile may be reached,
			// but BFS does not expand from it.
			// NOTE: stoppers will always be false for the starting hexes (see stopMask())
			if(stoppers[cur])
				continue;

			const auto nextDist = static_cast<dtype>(curDist + 1);

			for(const BattleHex & neighbour : curHex.getNeighbouringTiles())
			{
				const int ni = neighbour.toInt();

				if(!accessible[ni])
					continue;

				if(nextDist >= distances[ni])
					continue;

				distances[ni] = nextDist;
				predecessors[ni] = curHex;
				queue[tail++] = neighbour;
			}
		}

		return distances;
	}

	const ObstaclePtr moat;
	const bool isWideMoat;
	const bool isNarrowMoat;
	const EGateState gatestate;

	// Can the unit stand on this hex?
	// XXX: technically, accessL1 and accessR1 will always be the same
	//      except in siege battles where bhex 95 & 96 are inaccessible in L1
	const Mask accessL1{}; // accessibility for single-wide left units
	const Mask accessL2{}; // accessibility for double-wide left units
	const Mask accessR1{}; // accessibility for single-wide right units
	const Mask accessR2{}; // accessibility for double-wide right units

	// Does this hex stop the movement of the unit? (e.g. moat)
	const Mask stopL1{}; // stopping hexes for single-wide left units
	const Mask stopL2{}; // stopping hexes for double-wide left units
	const Mask stopR1{}; // stopping hexes for single-wide right units
	const Mask stopR2{}; // stopping hexes for double-wide right units

	const Mask & _accessMask(BattleSide side, bool wide) const
	{
		if(wide)
			return side == BattleSide::ATTACKER ? accessL2 : accessR2;
		return side == BattleSide::ATTACKER ? accessL1 : accessR1;
	}

	Mask accessMask(BattleSide side, bool wide, const BattleHex & oldpos) const
	{
		auto mask = _accessMask(side, wide);

		// reachability should be calculated from the POV of a hypothetical
		// stack position which must have been accessible in the first place
		// no need to mark newpos as accessible (it already is)
		mask[oldpos.toInt()] = true;

		if(wide)
		{
			const auto occupiedHex = CStack::occupiedHex(oldpos, wide, side);

			/*
			 * . . . . . . .     . . . . . .
			 *  . . . . . .     . . . . . .
			 * . . . . . . .     . - R R ◼ .
			 *  . ◼ L L - .     . . . . . .
			 * . . . . . . .     . . . . . .
			 *
			 * In accessL2, LL as well as the hex "-" in front are all unaccessible.
			 * In accessR2, RR and the "-" in front of it are unaccessible.
			 *
			 * However, we are now calculating reachability for a new hypothetical
			 * position of LL (or RR). hence we must set as available the LL hexes,
			 * but also the hex front as well.
			 * Caveat1: if that hex in front was unaccessible because of something else
			 *          (e.g. real obstacle), then it must remain unaccessible
			 *          => set it to whatever value it has in accessL1.
			 * Caveat2: if the hex "behind" the L stack was an inaccessible (e.g. obstacle),
			 *          then we must *not* mark both LL hexes as available: only the primary.
			 *
			 */

			// Handle caveat 1
			const auto hexInFront =
				side == BattleSide::LEFT_SIDE ? oldpos.cloneInDirection(BattleHex::EDir::RIGHT) : oldpos.cloneInDirection(BattleHex::EDir::LEFT);

			mask[hexInFront.toInt()] = _accessMask(side, false)[hexInFront.toInt()];

			// Handle caveat 2
			// (since occupied hex may already be outside the battlfield, check first)
			if(occupiedHex.isAvailable())
			{
				const auto hexBehind =
					side == BattleSide::LEFT_SIDE ? occupiedHex.cloneInDirection(BattleHex::EDir::LEFT) : occupiedHex.cloneInDirection(BattleHex::EDir::RIGHT);

				mask[occupiedHex.toInt()] = _accessMask(side, false)[hexBehind.toInt()];
			}
		}

		return mask;
	}

	const Mask & _stopMask(BattleSide side, bool wide) const
	{
		if(wide)
			return side == BattleSide::ATTACKER ? stopL2 : stopR2;
		return side == BattleSide::ATTACKER ? stopL1 : stopR1;
	}

	Mask stopMask(BattleSide side, bool wide, const BattleHex & newpos) const
	{
		auto mask = _stopMask(side, wide);

		// Primary hex can never be stopping
		mask[newpos.toInt()] = false;

		if(!wide || !moat)
			return mask;

		/*
		 * A wide stack which already stands on the moat does not count
		 * any of its occupied hexes as stopping anymore.
		 *
		 * Layout (no mask):  Layout (no mask, wide moat):
		 *  . . . . ~ . . .   . . . ~ ~ | . .
		 * . . . . ~ | . .   . . . ~ ~ | . .
		 *  . . . ~ . . . .   . . ~ ~ . . . .    Legend:
		 * . . . ~ | . . .   . . ~ ~ | . . .       ~    moat
		 *  . . . @ @ . . .   . . ~ @ @ . . .      |    wall
		 * . . . ~ | . . .   . . ~ ~ | . . .       .    accessible (destroyed wall)
		 *  . . . ~ | . . .   . . ~ ~ | . .        @    gate
		 * . . . . ~ . . .     . . ~ ~ . . .
		 *
		 * Masks:                               Masks (wide moat):
		 * stop2L:           stop2R:            stop2L:           stop2R:
		 *  . . . . ~ > . .   . . . < ~ . . .    . . . ~ ~ | . .   . . < ~ ~ . . .
		 * . . . . ~ | . .   . . . < ~ | . .    . . . ~ ~ | . .   . . < ~ ~ | . .
		 *  . . . ~ > . . .   . . < ~ . . . .    . . ~ ~ > . . .   . < ~ ~ . . . .
		 * . . . ~ | . . .   . . < ~ | . . .    . . ~ ~ | . . .   . < ~ ~ | . . .
		 *  . . . @ @ . . .   . . . @ @ . . .    . . ~ @ @ . . .   . . . @ @ . . .
		 * . . . ~ | . . .   . . < ~ | . . .    . . ~ ~ | . . .   . < ~ ~ | . . .
		 *  . . . ~ | . . .   . . < ~ | . . .    . . ~ ~ | . .     . < ~ ~ | . .
		 * . . . . ~ > . .   . . . < ~ . . .      . . ~ ~ > . .   . . < ~ ~ . . .
		 *
		 * "<" / ">" is the extra stopping hex for wide units
		 *          This is the location outside the moat where their primary
		 *           hex would be if their rear hex is still in the moat.
		 *
		 * The above masks are like this regardless of current unit positions.
		 * I.e. even if there is a unit like this (left diagram):
		 *
		 * stop2L (stored):           stop2L (modified)
		 *  . . . . ~ > . .            . . . . ~ > . .
		 * . . . . ~ | . .            . . . . ~ | . .
		 *  . . o o > . . .            . . o o . . . .   modify the stored graph
		 * . . . ~ | . . .            . . . ~ | . . .    to remove the ">"
		 *  . . . @ @ . . .            . . . @ @ . . .
		 * . . . ~ | . . .            . . . ~ | . . .
		 *  . . . ~ | . . .            . . . ~ | . . .
		 * . . . . ~ > . .            . . . . ~ > . .
		 *
		 * If the unit sits in the moat, some hexes must be modified to non-stopping:
		 * 1. all ~ it occupies
		 * 2. (wide L stacks) the > hex if the primary hex of the stack is on a narrow moatHex
		 * 2. (wide R stacks) the < hex if the primary hex of the stack is on a wide moatHex
		 *
		 * We don't need to modify mask for oldpos (the stored hexes are already stopping as if the stack weren't there)
		 * But we must modify mask for newpos as per the above rules
		 */

		const BattleHex newpos_rear = CStack::occupiedHex(newpos, true, side);

		// Positions of the "~" hexes for narrow and wide moats, respectively
		static const BattleHexArray moatHexes = {11, 28, 44, 61, 77, 111, 129, 146, 164, 181};
		static const BattleHexArray wideMoatHexes = {10, 27, 43, 60, 76, 94, 110, 128, 145, 163, 180};
		static const BattleHexArray allMoatHexes = {10, 11, 27, 28, 43, 44, 60, 61, 76, 77, 94, 110, 111, 128, 129, 145, 163, 164, 180, 181};

		if(side == BattleSide::LEFT_SIDE)
		{
			/*
			 * narrow moats:
			 *
			 * (L.1):
			 * Stack can freely exit the moat:
			 * . . . . ~ > . .    ->   . . . . ~ > . .
			 *  . . o õ > . . .   ->    . . o o . . . .
			 * . . . ~ > . . .    ->   . . . ~ > . . .
			 *
			 * (L.2):
			 * Stack can freely exit the moat:
			 * . . . . ~ > . .    ->   . . . . ~ > . .
			 *  . . . õ o . . .   ->    . . . o o . . .  the "õ" on the rear hex is modified
			 * . . . ~ > . . .    ->   . . . ~ > . . .
			 *
			 * wide moats:
			 *
			 * (LW.1):
			 * Stack can freely exit the moat:
			 * . . . ~ ~ > . .    ->   . . . ~ ~ > . .
			 *  . . õ õ > . . .   ->    . . o o . . . .
			 * . . ~ ~ > . . .    ->   . . ~ ~ > . . .
			 *
			 * (LW.2):
			 * Stack can exit the moat leftward only:
			 * . . . ~ ~ > . .    ->   . . . ~ ~ > . .
			 *  . o õ ~ > . . .   ->    . o o ~ > . . . (same as if small unit)
			 * . . ~ ~ > . . .    ->   . . ~ ~ > . . .
			 *
			 * (LW.3):
			 * Stack can exit the moat rightward only:
			 * . . . ~ ~ > . .    ->   . . . ~ ~ > . .
			 *  . . ~ õ o . . .   ->    . . ~ õ o . . . (same as if small unit)
			 * . . ~ ~ > . . .    ->   . . ~ ~ > . . .
			 *
			 */

			if(moatHexes.contains(newpos))
			{ // (L.1), (LW.1)
				mask[newpos.toInt() - 1] = false;
				mask[newpos.toInt() + 1] = false;
			}
			else if(isNarrowMoat && moatHexes.contains(newpos_rear))
			{ // (L.2)
				mask[newpos_rear.toInt()] = false;
			}

			// (LW.2) already handled (only primary hex needs modification)
			// (LW.3) already handled (only primary hex needs modification)
		}
		else
		{
			/*
			 * narrow moats:
			 *
			 * (R.1):
			 * Stack can freely exit the moat:
			 *   . . . < ~ . . .    ->   . . . < ~ . . .
			 *    . . < õ o . . .   ->    . . . o o . . .
			 *   . . < ~ . . . .    ->   . . < ~ . . . .
			 *
			 * (R.2):
			 * Stack can freely exit the moat:
			 *   . . . < ~ . . .    ->   . . . < ~ . . .
			 *    . . o õ . . . .   ->    . . o o . . . .
			 *   . . < ~ . . . .    ->   . . < ~ . . . .
			 *
			 *
			 * wide moats:
			 *
			 * (RW.1):
			 * Stack can freely exit the moat:
			 *   . . < ~ ~ . . .    ->   . . < ~ ~ . . .
			 *    . < õ õ . . . .   ->    . . o o . . . .
			 *   . < ~ ~ . . . .    ->   . < ~ ~ . . . .
			 *
			 * (RW.2):
			 * Stack can exit the moat rightward only:
			 *   . . < ~ ~ . . .    ->   . . < ~ ~ . . .
			 *    . < ~ õ o . . .   ->    . < ~ o o . . .  the "<" remains
			 *   . < ~ ~ . . . .    ->   . < ~ ~ . . . .
			 *
			 * (RW.3):
			 * Stack can exit the moat leftward only:
			 *   . . < ~ ~ . . .    ->   . . < ~ ~ . . .
			 *    . o õ ~ . . . .   ->    . o õ ~ . . . .  the "õ" on th reat hex remains
			 *   . < ~ ~ . . . .    ->   . < ~ ~ . . . .
			 *
			 */

			if(isNarrowMoat)
			{
				if(moatHexes.contains(newpos))
				{ // (R.1)
					mask[newpos.toInt() - 1] = false;
				}
				else if(moatHexes.contains(newpos_rear))
				{ // (R.2)
					mask[newpos_rear.toInt()] = false;
				}
			}
			else if(wideMoatHexes.contains(newpos))
			{ // (RW.1)
				mask[newpos.toInt() - 1] = false;
				mask[newpos.toInt() + 1] = false;
			}

			// (RW.2) already handled (only primary hex needs modification)
			// (RW.3) already handled (only primary hex needs modification)
		}

		return mask;
	}
};

}
