/*
 * CBattleInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInfoCallback.h"

#include <vcmi/scripting/Service.h>
#include <vstd/RNG.h>

#include "../CStack.h"
#include "BattleInfo.h"
#include "CObstacleInstance.h"
#include "IGameSettings.h"
#include "../bonuses/BonusParameters.h"
#include "../callback/CallbackDefines.h"
#include "../entities/building/TownFortifications.h"
#include "../GameLibrary.h"
#include "../combatScripts/IDamageCalculatorScript.h"
#include "../scripting/ScriptService.h"
#include "../spells/ObstacleCasterProxy.h"
#include "../spells/ISpellMechanics.h"
#include "../spells/Problem.h"
#include "../spells/CSpell.h"
#include "../mapObjects/CGTownInstance.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../BattleFieldHandler.h"
#include "../Rect.h"
#include "../spells/effects/Effect.h"

static BattleHex lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
{
	static const BattleHex lineToHex[] = {12, 29, 45, 62, 78, 96, 112, 130, 147, 165, 182};

	return lineToHex[line];
}

static std::optional<std::pair<BattleHex, BattleHex>> getLongWeaponLineHexes(const BattleHex & defenderHex, BattleHex::EDir direction)
{
	try
	{
		BattleHex middleHex = defenderHex.cloneInDirection(direction, false);
		BattleHex attackerHex = middleHex.cloneInDirection(direction, false);
		return std::make_pair(middleHex, attackerHex);
	}
	catch(const std::out_of_range &)
	{
		return std::nullopt;
	}
}

/// Checks whether unit with free shooting may target units standing next to it. Unrestricted free
/// shooting, e.g. Bow of the Sharpshooter, takes priority over restricted one, e.g. Steel Elves
static bool canShootAdjacentUnits(const battle::Unit * attacker)
{
	static const auto restricted = Selector::typeSubtype(BonusType::FREE_SHOOTING, BonusCustomSubtype::freeShootingExceptAdjacent);
	static const auto unrestricted = Selector::type()(BonusType::FREE_SHOOTING).And(restricted.Not());

	return !attacker->hasBonus(restricted) || attacker->hasBonus(unrestricted);
}

static bool isLongWeaponMiddleHexClear(const CBattleInfoCallback & callback, const BattleHex & middleHex)
{
	if(!middleHex.isValid())
		return false;

	const auto accessibility = callback.getAccessibility();
	return accessibility[middleHex.toInt()] == EAccessibility::ACCESSIBLE;
}

static bool sameSideOfWall(const BattleHex & pos1, const BattleHex & pos2)
{
	const bool stackLeft = pos1 < lineToWallHex(pos1.getY());
	const bool destLeft = pos2 < lineToWallHex(pos2.getY());

	return stackLeft == destLeft;
}

// parts of wall
static const std::pair<int, EWallPart> wallParts[] =
{
	std::make_pair(50, EWallPart::KEEP),
	std::make_pair(183, EWallPart::BOTTOM_TOWER),
	std::make_pair(182, EWallPart::BOTTOM_WALL),
	std::make_pair(130, EWallPart::BELOW_GATE),
	std::make_pair(78, EWallPart::OVER_GATE),
	std::make_pair(29, EWallPart::UPPER_WALL),
	std::make_pair(12, EWallPart::UPPER_TOWER),
	std::make_pair(95, EWallPart::INDESTRUCTIBLE_PART_OF_GATE),
	std::make_pair(96, EWallPart::GATE),
	std::make_pair(45, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(62, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(112, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(147, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(165, EWallPart::INDESTRUCTIBLE_PART)
};

static EWallPart hexToWallPart(const BattleHex & hex)
{
	si16 hexValue = hex.toInt();
	for(const auto & elem : wallParts)
	{
		if(elem.first == hexValue)
			return elem.second;
	}

	return EWallPart::INVALID; //not found!
}

static BattleHex WallPartToHex(EWallPart part)
{
	for(const auto & elem : wallParts)
	{
		if(elem.second == part)
			return elem.first;
	}

	return BattleHex::INVALID; //not found!
}

ESpellCastProblem CBattleInfoCallback::battleCanCastSpell(const spells::Caster * caster, spells::Mode mode) const
{
	THROW_IF_NOT_BATTLE();
	if(caster == nullptr)
		THROW_INVALID_CALL();

	const PlayerColor player = caster->getCasterOwner();
	const auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return ESpellCastProblem::INVALID;
	if(!battleDoWeKnowAbout(side))
		THROW_INVALID_CALL();

	if(battleTacticDist())
		return ESpellCastProblem::ONGOING_TACTIC_PHASE;

	switch(mode)
	{
	case spells::Mode::HERO:
	{
		const auto * hero = caster->getHeroCaster();

		if(!hero)
			return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
		if(!hero->canCastSpells())
			return ESpellCastProblem::NO_SPELLBOOK;
		if(battleCastSpells(side) >= hero->valOfBonuses(BonusType::HERO_SPELL_CASTS_PER_COMBAT_TURN))
			return ESpellCastProblem::CASTS_PER_TURN_LIMIT;
	}
		break;
	default:
		break;
	}

	//Orb of Inhibition blocks active spellcasting (hero and creature active abilities),
	//but not passive/triggered casts such as SPELL_BEFORE_ATTACK / SPELL_AFTER_ATTACK.
	//Level-0 creature abilities are excluded from this block in BattleSpellMechanics::canBeCast (spell level known there)
	if(mode == spells::Mode::HERO || mode == spells::Mode::CREATURE_ACTIVE)
	{
		const IBonusBearer * casterBonuses = caster->getHeroCaster();
		if(!casterBonuses)
			casterBonuses = battleGetUnitByID(caster->getCasterUnitId());
		if(casterBonuses && casterBonuses->hasBonusOfType(BonusType::BLOCK_ALL_MAGIC))
			return ESpellCastProblem::MAGIC_IS_BLOCKED;
	}

	return ESpellCastProblem::OK;
}

std::pair< BattleHexArray, int > CBattleInfoCallback::getPath(const BattleHex & start, const BattleHex & dest, const battle::Unit * stack) const
{
	auto reachability = getReachability(stack);

	if(reachability.predecessors[dest.toInt()] == -1) //cannot reach destination
	{
		return std::make_pair(BattleHexArray(), 0);
	}

	//making the Path
	BattleHexArray path;
	BattleHex curElem = dest;
	while(curElem != start)
	{
		path.insert(curElem);
		curElem = reachability.predecessors[curElem.toInt()];
	}

	return std::make_pair(path, reachability.distances[dest.toInt()]);
}

bool CBattleInfoCallback::battleIsInsideWalls(const BattleHex & from) const
{
	BattleHex wallPos = lineToWallHex(from.getY());

	if (from < wallPos)
		return false;

	if (wallPos < from)
		return true;

	// edge case - this is the wall. (or drawbridge)
	// since this method is used exclusively to determine behavior of defenders,
	// consider it inside walls, unless this is intact drawbridge - to prevent defenders standing on it and opening the gates
	if (from == BattleHex::GATE_INNER)
		return battleGetGateState() == EGateState::DESTROYED;
	return true;
}

bool CBattleInfoCallback::battleHasPenaltyOnLine(const BattleHex & from, const BattleHex & dest, bool checkWall, bool checkMoat) const
{
	if (!from.isAvailable() || !dest.isAvailable())
		throw std::runtime_error("Invalid hex (" + std::to_string(from.toInt()) + " and " + std::to_string(dest.toInt()) + ") received in battleHasPenaltyOnLine!" );

	if(battleGetFortifications().wallsHealth == 0)
		return false;

	auto isTileBlocked = [&](const BattleHex & tile)
	{
		EWallPart wallPart = battleHexToWallPart(tile);
		if (wallPart == EWallPart::INVALID)
			return false; // there is no wall here
		if (wallPart == EWallPart::INDESTRUCTIBLE_PART_OF_GATE)
			return false; // does not blocks ranged attacks
		if (wallPart == EWallPart::INDESTRUCTIBLE_PART)
			return true; // always blocks ranged attacks

		return isWallPartAttackable(wallPart);
	};
	// Count wall penalty requirement by shortest path, not by arbitrary line, to avoid various OH3 bugs
	auto getShortestPath = [](const BattleHex & from, const BattleHex & dest) -> BattleHexArray
	{
		//Out early
		if(from == dest)
			return {};

		BattleHexArray ret;
		auto next = from;
		//Not a real direction, only to indicate to which side we should search closest tile
		auto direction = from.getX() > dest.getX() ? BattleSide::DEFENDER : BattleSide::ATTACKER;

		while (next != dest)
		{
			next = BattleHex::getClosestTile(direction, dest, next.getNeighbouringTiles());
			ret.insert(next);
		}
		assert(!ret.empty());
		ret.pop_back(); //Remove destination hex
		return ret;
	};

	THROW_IF_NOT_BATTLE();
	auto checkNeeded = !sameSideOfWall(from, dest);
	bool pathHasWall = false;
	bool pathHasMoat = false;

	for(const auto & hex : getShortestPath(from, dest))
	{
		pathHasWall |= isTileBlocked(hex);
		if(!checkMoat)
			continue;

		auto obstacles = battleGetAllObstaclesOnPos(hex, false);

		if(hex.toInt() != BattleHex::GATE_BRIDGE || (battleIsGatePassable()))
			for(const auto & obst : obstacles)
				if(obst->obstacleType ==  CObstacleInstance::MOAT)
					pathHasMoat |= true;
	}

	return checkNeeded && ( (checkWall && pathHasWall) || (checkMoat && pathHasMoat) );
}

bool CBattleInfoCallback::battleHasWallPenalty(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHex & destHex) const
{
	THROW_IF_NOT_BATTLE();
	if(battleGetFortifications().wallsHealth == 0)
		return false;

	const std::string cachingStrNoWallPenalty = "type_NO_WALL_PENALTY";
	static const auto selectorNoWallPenalty = Selector::type()(BonusType::NO_WALL_PENALTY);

	if(shooter->hasBonus(selectorNoWallPenalty, cachingStrNoWallPenalty))
		return false;

	const auto shooterOutsideWalls = shooterPosition < lineToWallHex(shooterPosition.getY());

	return shooterOutsideWalls && battleHasPenaltyOnLine(shooterPosition, destHex, true, false);
}

const CStack* CBattleInfoCallback::battleGetStackByPos(const BattleHex & pos, bool onlyAlive) const
{
	THROW_IF_NOT_BATTLE();
	for(const auto * s : battleGetAllStacks(true))
		if(s->getHexes().contains(pos) && (!onlyAlive || s->alive()))
			return s;

	return nullptr;
}

const battle::Unit * CBattleInfoCallback::battleGetUnitByPos(const BattleHex & pos, bool onlyAlive) const
{
	THROW_IF_NOT_BATTLE();

	auto ret = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return !unit->isGhost()
			&& unit->coversPos(pos)
			&& (!onlyAlive || unit->alive());
	});

	if(!ret.empty())
		return ret.front();
	else
		return nullptr;
}

const battle::Unit * CBattleInfoCallback::battleGetUnitOrCorpseByPos(const BattleHex & pos) const
{
	if(const battle::Unit * unit = battleGetUnitByPos(pos, true))
		return unit;
	return battleGetUnitByPos(pos, false);
}

battle::Units CBattleInfoCallback::battleAliveUnits() const
{
	return battleGetUnitsIf([](const battle::Unit * unit)
	{
		return unit->isValidTarget(false);
	});
}

battle::Units CBattleInfoCallback::battleAliveUnits(BattleSide side) const
{
	return battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget(false) && unit->unitSide() == side;
	});
}

using namespace battle;

static const battle::Unit * takeOneUnit(battle::Units & allUnits, const int turn, BattleSide & sideThatLastMoved, int phase)
{
	const battle::Unit * returnedUnit = nullptr;
	size_t currentUnitIndex = 0;

	for(size_t i = 0; i < allUnits.size(); i++)
	{
		int32_t currentUnitInitiative = -1;
		int32_t returnedUnitInitiative = -1;

		if(returnedUnit)
			returnedUnitInitiative = returnedUnit->getInitiative(turn);

		if(!allUnits[i])
			continue;

		auto currentUnit = allUnits[i];
		currentUnitInitiative = currentUnit->getInitiative(turn);

		switch(phase)
		{
		case BattlePhases::NORMAL: // Faster first, attacker priority, higher slot first
			if(returnedUnit == nullptr || currentUnitInitiative > returnedUnitInitiative)
			{
				returnedUnit = currentUnit;
				currentUnitIndex = i;
			}
			else if(currentUnitInitiative == returnedUnitInitiative)
			{
				if(sideThatLastMoved == BattleSide::NONE && turn <= 0 && currentUnit->unitSide() == BattleSide::ATTACKER
					&& !(returnedUnit->unitSide() == currentUnit->unitSide() && returnedUnit->unitSlot() < currentUnit->unitSlot())) // Turn 0 attacker priority
				{
					returnedUnit = currentUnit;
					currentUnitIndex = i;
				}
				else if(sideThatLastMoved != BattleSide::NONE && currentUnit->unitSide() != sideThatLastMoved
					&& !(returnedUnit->unitSide() == currentUnit->unitSide() && returnedUnit->unitSlot() < currentUnit->unitSlot())) // Alternate equal speeds units
				{
					returnedUnit = currentUnit;
					currentUnitIndex = i;
				}
			}
			break;
		case BattlePhases::WAIT_MORALE: // Slower first, higher slot first
		case BattlePhases::WAIT:
			if(returnedUnit == nullptr || currentUnitInitiative < returnedUnitInitiative)
			{
				returnedUnit = currentUnit;
				currentUnitIndex = i;
			}
			else if(currentUnitInitiative == returnedUnitInitiative && sideThatLastMoved != BattleSide::NONE && currentUnit->unitSide() != sideThatLastMoved
				&& !(returnedUnit->unitSide() == currentUnit->unitSide() && returnedUnit->unitSlot() < currentUnit->unitSlot())) // Alternate equal speeds units
			{
				returnedUnit = currentUnit;
				currentUnitIndex = i;
			}
			break;
		default:
			break;
		}
	}

	if(!returnedUnit)
		return nullptr;

	allUnits[currentUnitIndex] = nullptr;

	return returnedUnit;
}

void CBattleInfoCallback::battleGetTurnOrder(std::vector<battle::Units> & turns, const size_t maxUnits, const int maxTurns, const int turn, BattleSide sideThatLastMoved) const
{
	THROW_IF_NOT_BATTLE();

	if(maxUnits == 0 && maxTurns == 0)
		THROW_INVALID_CALL();

	auto actualTurn = turn > 0 ? turn : 0;

	auto turnsIsFull = [&]() -> bool
	{
		if(maxUnits == 0)
			return false;//no limit

		size_t turnsSize = 0;
		for(const auto & oneTurn : turns)
			turnsSize += oneTurn.size();
		return turnsSize >= maxUnits;
	};

	turns.emplace_back();

	// We'll split creatures with remaining movement to 4 buckets (SIEGE, NORMAL, WAIT_MORALE, WAIT)
	std::array<battle::Units, BattlePhases::NUMBER_OF_PHASES> phases; // Access using BattlePhases enum

	const battle::Unit * activeUnit = battleActiveUnit();

	if(activeUnit)
	{
		//its first turn and active unit hasn't taken any action yet - must be placed at the beginning of queue, no matter what
		if(turn == 0 && activeUnit->willMove())
		{
			turns.back().push_back(activeUnit);
			if(turnsIsFull())
				return;
		}

		//its first or current turn, turn priority for active stack side
		//TODO: what if active stack mind-controlled?
		if(turn <= 0 && sideThatLastMoved == BattleSide::NONE)
			sideThatLastMoved = activeUnit->unitSide();
	}

	auto allUnits = battleGetUnitsIf([](const battle::Unit * unit)
	{
		return !unit->isGhost();
	});

	// If no unit will be EVER! able to move, battle is over.
	if(!vstd::contains_if(allUnits, [](const battle::Unit * unit) { return unit->willMove(100000); })) //little evil, but 100000 should be enough for all effects to disappear
	{
		turns.clear();
		return;
	}

	for(const auto * unit : allUnits)
	{
		if((actualTurn == 0 && !unit->willMove()) //we are considering current round and unit won't move
		|| (actualTurn > 0 && !unit->canMove(turn)) //unit won't be able to move in later rounds
		|| (actualTurn == 0 && unit == activeUnit && !turns.at(0).empty() && unit == turns.front().front())) //it's active unit already added at the beginning of queue
		{
			continue;
		}

		int unitPhase = unit->battleQueuePhase(turn);

		phases[unitPhase].push_back(unit);
	}

	std::ranges::sort(phases[BattlePhases::SIEGE], CMP_stack(BattlePhases::SIEGE, actualTurn, sideThatLastMoved));
	std::copy(phases[BattlePhases::SIEGE].begin(), phases[BattlePhases::SIEGE].end(), std::back_inserter(turns.back()));

	if(turnsIsFull())
		return;

	for(uint8_t phase = BattlePhases::NORMAL; phase < BattlePhases::NUMBER_OF_PHASES; phase++)
		std::ranges::sort(phases[phase], CMP_stack(phase, actualTurn, sideThatLastMoved));

	uint8_t phase = BattlePhases::NORMAL;
	while(!turnsIsFull() && phase < BattlePhases::NUMBER_OF_PHASES)
	{
		const battle::Unit * currentUnit = nullptr;
		if(phases[phase].empty())
			phase++;
		else
		{
			currentUnit = takeOneUnit(phases[phase], actualTurn, sideThatLastMoved, phase);
			if(!currentUnit)
			{
				phase++;
			}
			else
			{
				turns.back().push_back(currentUnit);
				sideThatLastMoved = currentUnit->unitSide();
			}
		}
	}

	if(sideThatLastMoved == BattleSide::NONE)
		sideThatLastMoved = BattleSide::ATTACKER;

	if(!turnsIsFull() && (maxTurns == 0 || turns.size() < maxTurns))
		battleGetTurnOrder(turns, maxUnits, maxTurns, actualTurn + 1, sideThatLastMoved);
}

BattleHexArray CBattleInfoCallback::battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange) const
{
	THROW_IF_NOT_BATTLE();
	if(!unit->getPosition().isValid()) //turrets
		return BattleHexArray();

	auto reachability = getReachability(unit);

	return battleGetAvailableHexes(reachability, unit, obtainMovementRange);
}

BattleHexArray CBattleInfoCallback::battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit, bool obtainMovementRange) const
{
	BattleHexArray ret;

	THROW_IF_NOT_BATTLE();
	if(!unit->getPosition().isValid()) //turrets
		return ret;

	auto unitSpeed = unit->getMovementRange(0);

	// a unit that can't move stays in place during the tactics phase too, as BattleActionProcessor::moveStack requires
	const bool tacticsMovement = battleTacticDist() && battleGetTacticsSide() == unit->unitSide() && unitSpeed > 0;

	for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		// If obstacles or other stacks makes movement impossible, it can't be helped.
		if(!cache.isReachable(i))
			continue;

		if(tacticsMovement && !obtainMovementRange) // if obtainMovementRange requested do not return tactics range
		{
			// Stack has to perform tactic-phase movement -> can enter any reachable tile within given range
			if(!isInTacticRange(i))
				continue;
		}
		else
		{
			// Not tactics phase -> destination must be reachable and within unit range.
			if(cache.distances[i] > static_cast<int>(unitSpeed))
				continue;
		}

		ret.insert(i);
	}

	return ret;
}

BattleHexArray CBattleInfoCallback::battleGetOccupiableHexes(const battle::Unit * unit, bool obtainMovementRange) const
{
	return battleGetOccupiableHexes(battleGetAvailableHexes(unit, obtainMovementRange), unit);
}

BattleHexArray CBattleInfoCallback::battleGetOccupiableHexes(const BattleHexArray & availableHexes, const battle::Unit * unit) const
{
	THROW_IF_NOT_BATTLE();
	if (!unit)
		throw std::runtime_error("Undefined unit in battleGetOccupiableHexes!");

	if (unit->doubleWide())
	{ 
		auto occupiableHexes = BattleHexArray(availableHexes);
		for (auto hex : availableHexes)
			occupiableHexes.insert(unit->occupiedHex(hex));
		return occupiableHexes;
	}
	return availableHexes;
}

BattleHex CBattleInfoCallback::fromWhichHexAttack(const battle::Unit * attacker, const BattleHex & target, const BattleHex::EDir & direction) const
{
	THROW_IF_NOT_BATTLE();
	if (!attacker)
		throw std::runtime_error("Undefined attacker in fromWhichHexAttack!");

	if (!target.isValid() || direction == BattleHex::NONE)
		return BattleHex::INVALID;

	bool isAttacker = attacker->unitSide() == BattleSide::ATTACKER;
	if (attacker->doubleWide())
	{
		// We need to find position of right hex of double-hex creature (or left for defending side)
		// | TOP_LEFT | TOP_RIGHT |  RIGHT  |BOTTOM_RIGHT|BOTTOM_LEFT|  LEFT   |  TOP   | BOTTOM
		// |  o o -   |    - o o  |  - -    |   - -      |    - -    |    - -  |  o o   |   - -
		// |   - x -  |   - x -   | - x o o |  - x -     |   - x -   | o o x - | - x -  |  - x -
		// |    - -   |    - -    |  - -    |   - o o    |  o o -    |    - -  |  - -   |   o o

		try
		{
			switch (direction)
			{
				case BattleHex::TOP_LEFT:
				case BattleHex::LEFT:
				case BattleHex::BOTTOM_LEFT:
					return target.cloneInDirection(direction, false)
						.cloneInDirection(isAttacker ? BattleHex::NONE : BattleHex::LEFT, false);

				case BattleHex::TOP_RIGHT:
				case BattleHex::RIGHT:
				case BattleHex::BOTTOM_RIGHT:
					return target.cloneInDirection(direction, false)
						.cloneInDirection(isAttacker ? BattleHex::RIGHT : BattleHex::NONE, false);

				case BattleHex::TOP:
					return target.cloneInDirection(isAttacker ? BattleHex::TOP_RIGHT : BattleHex::TOP_LEFT, false);

				case BattleHex::BOTTOM:
					return target.cloneInDirection(isAttacker ? BattleHex::BOTTOM_RIGHT : BattleHex::BOTTOM_LEFT, false);

				default:
					return BattleHex::INVALID;
			}
		}
		catch(const std::out_of_range &)
		{
			return BattleHex::INVALID;
		}
	}
	if (direction == BattleHex::TOP || direction == BattleHex::BOTTOM)
		return BattleHex::INVALID;

	try
	{
		return target.cloneInDirection(direction, false);
	}
	catch(const std::out_of_range &)
	{
		return BattleHex::INVALID;
	}
}

BattleHex CBattleInfoCallback::findAttackPosition(const BattleHexArray & availableHexes, const battle::Unit * attacker, const BattleHex & target, const BattleHex::EDir & direction, bool longWeapon) const
{
	THROW_IF_NOT_BATTLE();

	if(!longWeapon)
	{
		const BattleHex fromHex = fromWhichHexAttack(attacker, target, direction);
		return battleCanAttackFromHex(availableHexes, attacker, target, fromHex) ? fromHex : BattleHex::INVALID;
	}

	if(!target.isValid() || direction == BattleHex::NONE || direction == BattleHex::TOP || direction == BattleHex::BOTTOM || !attacker->hasBonusOfType(BonusType::LONG_WEAPON))
		return BattleHex::INVALID;

	const battle::Unit * defender = battleGetUnitByPos(target, true);
	const auto longLine = getLongWeaponLineHexes(target, direction);
	if(!defender || !longLine)
		return BattleHex::INVALID;

	// the attacker strikes with its front or, if two hexes wide, its back hex on the line; staying in place is preferred
	const BattleHex lineHex = longLine->second;
	std::vector<BattleHex> candidates = {attacker->getPosition(), lineHex};
	if(attacker->doubleWide())
		candidates.emplace_back(lineHex.toInt() + (attacker->unitSide() == BattleSide::ATTACKER ? 1 : -1));

	for(const BattleHex & fromHex : candidates)
	{
		if(battleCanAttackFromHex(availableHexes, attacker, target, fromHex)
			&& attacker->getHexes(fromHex).contains(lineHex)
			&& isLongWeaponAttack(attacker, defender, fromHex))
			return fromHex;
	}

	return BattleHex::INVALID;
}

BattleHex CBattleInfoCallback::toWhichHexMove(const battle::Unit * unit, const BattleHex & position) const
{
	return toWhichHexMove(battleGetAvailableHexes(unit, false), unit, position);
}

BattleHex CBattleInfoCallback::toWhichHexMove(const BattleHexArray & availableHexes, const battle::Unit * unit, const BattleHex & position) const
{
	THROW_IF_NOT_BATTLE();

	if (!unit)
		throw std::runtime_error("Undefined unit in toWhichHexMove!");
	if (!position.isValid())
		return BattleHex::INVALID;

	if (availableHexes.contains(position))
		return position;
	if (unit->doubleWide())
	{
		auto headPosition = position.cloneInDirection(unit->headDirection(), false);
		if (availableHexes.contains(headPosition))
			return headPosition;
	}
	return BattleHex::INVALID;
}

bool CBattleInfoCallback::battleCanAttackHex(const BattleHexArray & availableHexes, const battle::Unit * attacker, const BattleHex & position) const
{
	for (auto direction = 0; direction < 8; direction++)
	{
		if (battleCanAttackHex(availableHexes, attacker, position, BattleHex::EDir(direction)))
			return true;
	}
	return false;
}

bool CBattleInfoCallback::battleCanAttackFromHex(const BattleHexArray & availableHexes, const battle::Unit * attacker, const BattleHex & position, const BattleHex & fromHex) const
{
	THROW_IF_NOT_BATTLE();

	//check if the attack is performed from an available hex
	if (!fromHex.isValid() || !availableHexes.contains(fromHex))
		return false;

	//if the movement ends in an obstacle, check if the obstacle allows attacking from that position
	if (attacker->getPosition() != fromHex)
	{
		if (!attacker->hasBonusOfType(BonusType::FLYING))
		{
			for (const auto & obstacle : battleGetAllObstacles())
			{
				if (obstacle->getStoppingTile().contains(fromHex))
					return false;
				if (attacker->doubleWide() && obstacle->getStoppingTile().contains(attacker->occupiedHex(fromHex)))
					return false;
			}
		}
		const battle::Unit * defender = battleGetUnitByPos(position, false); //Do not allow to target corpses when standing on them (a WALK_AND_SPELLCAST action)
		if (defender && defender->isDead() && defender->coversPos(fromHex))
			return false;
	}

	return true;
}

bool CBattleInfoCallback::battleCanAttackHex(const BattleHexArray & availableHexes, const battle::Unit * attacker, const BattleHex & position, const BattleHex::EDir & direction) const
{
	THROW_IF_NOT_BATTLE();

	if (!attacker)
		throw std::runtime_error("Undefined attacker in battleCanAttackHex!");

	return findAttackPosition(availableHexes, attacker, position, direction, false).isValid()
		|| findAttackPosition(availableHexes, attacker, position, direction, true).isValid();
}

bool CBattleInfoCallback::battleCanAttackUnit(const battle::Unit * attacker, const battle::Unit * target) const
{
	THROW_IF_NOT_BATTLE();

	if(battleTacticDist())
		return false;

	if (!attacker)
		throw std::runtime_error("Undefined attacker in battleCanAttackUnit!");

	if(!target || target->isInvincible())
		return false;

	if(attacker == target || !battleMatchOwner(attacker, target))
		return false;

	if (!attacker->isMeleeAttacker())
		return false;

	return target->alive();
}

bool CBattleInfoCallback::battleCanShoot(const battle::Unit * attacker) const
{
	THROW_IF_NOT_BATTLE();

	if(battleTacticDist()) //no shooting during tactics
		return false;

	if (!attacker)
		return false;
	if (attacker->isCatapult()) //catapult cannot attack creatures
		return false;

	if (!attacker->canShoot())
		return false;

	return attacker->canShootBlocked() || !battleIsUnitBlocked(attacker);
}

bool CBattleInfoCallback::battleCanTargetEmptyHex(const battle::Unit * attacker) const
{
	THROW_IF_NOT_BATTLE();

	if(!LIBRARY->engineSettings()->getBoolean(EGameSettings::COMBAT_AREA_SHOT_CAN_TARGET_EMPTY_HEX))
		return false;

	if(attacker->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
	{
		auto bonus = attacker->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
		const CSpell * spell = bonus->subtype.as<SpellID>().toSpell();
		spells::BattleCast cast(this, attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
		BattleHex dummySpellTarget = BattleHex(50); //check arbitrary hex for general spell range since currently there is no general way to access amount of hexes

		if(spell->battleMechanics(&cast)->rangeInHexes(dummySpellTarget).size() > 1)
		{
			return true;
		}
	}

	return false;
}

BattleHexArray CBattleInfoCallback::meleeAttackHexes(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPosition, const BattleHex & defenderPosition) const
{
	BattleHexArray res;

	BattleHex attackerPos = attackerPosition.isValid() ? attackerPosition : attacker->getPosition();
	BattleHex defenderPos = defenderPosition.isValid() ? defenderPosition : defender->getPosition();

	BattleHexArray defenderHexes = defender->getHexes(defenderPos);
	BattleHexArray attackerHexes = attacker->getHexes(attackerPos);

	for (BattleHex defenderHex : defenderHexes)
	{
		if (attackerHexes.contains(defenderHex))
		{
			logGlobal->debug("CBattleInfoCallback::meleeAttackHexes: defender and attacker positions overlap");
			return res;
		}
	}

	const BattleHexArray attackableHxs = attacker->getSurroundingHexes(attackerPos);

	for (BattleHex defenderHex : defenderHexes)
	{
		if (attackableHxs.contains(defenderHex))
			res.insert(defenderHex);
	}

	return res;
}

bool CBattleInfoCallback::isMeleeAttackPossible(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPos, const BattleHex & defenderPos) const
{
	if(defender->isInvincible())
		return false;

	return !meleeAttackHexes(attacker, defender, attackerPos, defenderPos).empty();
}

bool CBattleInfoCallback::isLongWeaponAttack(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPos) const
{
	THROW_IF_NOT_BATTLE();

	if(!attacker)
		throw std::runtime_error("Undefined attacker in isLongWeaponAttack!");
	if(!defender)
		throw std::runtime_error("Undefined defender in isLongWeaponAttack!");

	if(!attacker->hasBonusOfType(BonusType::LONG_WEAPON))
		return false;

	if(isMeleeAttackPossible(attacker, defender, attackerPos))
		return false;

	const BattleHexArray attackerHexes = attacker->getHexes(attackerPos.isValid() ? attackerPos : attacker->getPosition());

	for(const BattleHex & defenderHex : defender->getHexes())
	{
		for(int direction = 0; direction < 6; ++direction)
		{
			const auto longLine = getLongWeaponLineHexes(defenderHex, static_cast<BattleHex::EDir>(direction));
			if(!longLine)
				continue;

			const auto [middleHex, attackerHex] = *longLine;
			if(attackerHexes.contains(attackerHex) && isLongWeaponMiddleHexClear(*this, middleHex))
				return true;
		}
	}

	return false;
}

bool CBattleInfoCallback::battleCanShoot(const battle::Unit * attacker, const BattleHex & dest) const
{
	THROW_IF_NOT_BATTLE();

	if(!dest.isAvailable())
		return false;

	const battle::Unit * defender = battleGetUnitByPos(dest);
	if(!attacker)
		return false;

	bool emptyHexAreaAttack = battleCanTargetEmptyHex(attacker);

	if(!emptyHexAreaAttack)
	{
		if(!defender)
			return false;

		if(defender->isInvincible())
			return false;
	}

	bool attackerIsBerserk = attacker->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE);
	if(emptyHexAreaAttack || (defender->alive() && (attackerIsBerserk || battleMatchOwner(attacker, defender))))
	{
		if(battleCanShoot(attacker))
		{
			// e.g. Steel Elves - unit shoots freely while blocked, but adjacent units can only be attacked in melee
			if(defender && !canShootAdjacentUnits(attacker) && isMeleeAttackPossible(attacker, defender))
				return false;

			auto limitedRangeBonus = attacker->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
			if(limitedRangeBonus == nullptr)
			{
				return true;
			}

			int shootingRange = limitedRangeBonus->val;

			if(defender)
				return isEnemyUnitWithinSpecifiedRange(attacker->getPosition(), defender, shootingRange);
			else
				return isHexWithinSpecifiedRange(attacker->getPosition(), dest, shootingRange);
		}
	}

	return false;
}

DamageEstimation CBattleInfoCallback::calculateDmgRange(const BattleAttackInfo & info) const
{

	const auto * script = LIBRARY->scriptTypes()->getDamageCalculator();

	// core declares one, and there is no rule for how much damage an attack deals without it
	if(!script)
		throw std::runtime_error("No damage calculator script is loaded!");

	DamageAttackInfo payload;

	payload.attacker = info.attacker;
	payload.defender = info.defender;
	// the script is told where the attack happens rather than left to work it out, so that an
	// attack being weighed reads the same as one being dealt
	payload.attackerHex = info.attackerPos.isValid() ? info.attackerPos : info.attacker->getPosition();
	payload.defenderHex = info.defenderPos.isValid() ? info.defenderPos : info.defender->getPosition();
	payload.chargeDistance = info.chargeDistance;
	payload.shooting = info.shooting;
	payload.luckyStrike = info.luckyStrike;
	payload.unluckyStrike = info.unluckyStrike;
	payload.deathBlow = info.deathBlow;
	payload.doubleDamage = info.doubleDamage;
	payload.attackFactorPerPoint = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR);
	payload.attackFactorCap = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP);
	payload.defenseFactorPerPoint = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR);
	payload.defenseFactorCap = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP);

	return script->calculate(*this, payload);
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPosition, DamageEstimation * retaliationDmg) const
{
	THROW_IF_NOT_BATTLE();
	auto reachability = battleGetDistances(attacker, attacker->getPosition());
	int movementRange = attackerPosition.isValid() ? reachability[attackerPosition.toInt()] : 0;
	return battleEstimateDamage(attacker, defender, movementRange, retaliationDmg);
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, int movementRange, DamageEstimation * retaliationDmg) const
{
	THROW_IF_NOT_BATTLE();
	const bool shooting = battleCanShoot(attacker, defender->getPosition());
	const BattleAttackInfo bai(attacker, defender, movementRange, shooting);
	return battleEstimateDamage(bai, retaliationDmg);
}

int64_t CBattleInfoCallback::getFirstAidHealValue(const CGHeroInstance * owner, const battle::Unit * target) const
{
	THROW_IF_NOT_BATTLE();
	if(!owner || !target)
		return 0;

	int64_t base = owner->valOfBonuses(BonusType::MANUAL_CONTROL, BonusSubtypeID(CreatureID(CreatureID::FIRST_AID_TENT)));

	if(base <= 0)
		return 0;

	auto state = target->acquireState();

	return state->heal(base, EHealLevel::HEAL, EHealPower::PERMANENT).healedHealthPoints;
}

SpellEffectValUptr CBattleInfoCallback::getSpellEffectValue(
	const CSpell * spell,
	const spells::Caster * caster,
	const spells::Mode spellMode,
	const battle::Target & aim) const
{
	auto result = std::make_unique<spells::effects::SpellEffectValue>();
	THROW_IF_NOT_BATTLE();
	if(!spell || !caster || aim.empty())
		return result;

	spells::BattleCast params(this, caster, spellMode, spell);
	std::unique_ptr<spells::Mechanics> mech = spell->battleMechanics(&params);
	if(!mech)
		return result;

	const spells::Target spellTarget = mech->canonicalizeTarget(aim);
	const battle::Unit * hoveredUnit = aim.back().unitValue;

	mech->forEachEffect([&](const spells::effects::Effect &e){
		auto effTarget = e.transformTarget(mech.get(), aim, spellTarget);
		// Cure-specific safety net: if empty, but hovering a healable friendly unit, evaluate just that unit
		if(effTarget.empty() && hoveredUnit)
		{
			spells::Target single;
			single.emplace_back(spells::Destination(hoveredUnit));
			*result += e.getHealthChange(mech.get(), single);
			return false;
		}

		if(!effTarget.empty())
			*result += e.getHealthChange(mech.get(), effTarget);
		return false; // continue iterating effects
	});

	return result;
}

DamageEstimation CBattleInfoCallback::estimateSpellLikeAttackDamage(const battle::Unit * shooter,
																	const CSpell * spell,
																	const BattleHex & aimHex) const
{
	THROW_IF_NOT_BATTLE();
	if(!spell || !shooter || !aimHex.isValid())
		return {};

	spells::ProxyCaster proxy(shooter);
	spells::BattleCast params(this, &proxy, spells::Mode::PASSIVE, spell);
	std::unique_ptr<spells::Mechanics> mech = spell->battleMechanics(&params);
	if(!mech)
		return {};

	spells::Target aim;
	aim.emplace_back(aimHex);
	const auto affected = mech->getAffectedStacks(aim);
	if(affected.empty())
		return {};

	DamageEstimation total {};

	for(const battle::Unit * u : affected)
	{
		BattleAttackInfo bai(shooter, u, 0, true);
		DamageEstimation de = calculateDmgRange(bai);

		total.damage.min += de.damage.min;
		total.damage.max += de.damage.max;
		total.kills.min  += de.kills.min;
		total.kills.max  += de.kills.max;
	}

	return total;
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const BattleAttackInfo & bai, DamageEstimation * retaliationDmg) const
{
	THROW_IF_NOT_BATTLE();

	DamageEstimation ret = calculateDmgRange(bai);

	if(retaliationDmg == nullptr)
		return ret;

	*retaliationDmg = DamageEstimation();

	if(bai.shooting) //FIXME: handle RANGED_RETALIATION
		return ret;

	if (!bai.defender->ableToRetaliate())	//FIXME: handle situation when NO_RETALIATION bonus is removed during attack
		return ret;

	if (bai.attacker->hasBonusOfType(BonusType::BLOCKS_RETALIATION) || bai.attacker->isInvincible() || isLongWeaponAttack(bai.attacker, bai.defender, bai.attackerPos))
		return ret;

	//TODO: rewire once more using interval-based fuzzy arithmetic

	const auto & estimateRetaliation = [&](int64_t damage)
	{
		auto retaliationAttack = bai.reverse();
		auto state = retaliationAttack.attacker->acquireState();
		state->damage(damage);
		retaliationAttack.attacker = state.get();
		if (state->alive())
			return calculateDmgRange(retaliationAttack);
		else
			return DamageEstimation();
	};

	DamageEstimation retaliationMin = estimateRetaliation(ret.damage.min);
	DamageEstimation retaliationMax = estimateRetaliation(ret.damage.max);

	retaliationDmg->damage.min = std::min(retaliationMin.damage.min, retaliationMax.damage.min);
	retaliationDmg->damage.max = std::max(retaliationMin.damage.max, retaliationMax.damage.max);

	retaliationDmg->kills.min = std::min(retaliationMin.kills.min, retaliationMax.kills.min);
	retaliationDmg->kills.max = std::max(retaliationMin.kills.max, retaliationMax.kills.max);

	return ret;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::battleGetAllObstaclesOnPos(const BattleHex & tile, bool onlyBlocking, std::optional<BattleSide> perspective) const
{
	auto obstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	THROW_IF_NOT_BATTLE();
	for(auto & obs : battleGetAllObstacles(perspective))
	{
		if(obs->getBlockedTiles().contains(tile)
				|| (!onlyBlocking && obs->getAffectedTiles().contains(tile)))
		{
			obstacles.push_back(obs);
		}
	}
	return obstacles;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::getAllAffectedObstaclesByStack(const battle::Unit * unit, const BattleHexArray & passed) const
{
	auto affectedObstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	THROW_IF_NOT_BATTLE();
	if(unit->alive())
	{
		if(!passed.contains(unit->getPosition()))
			affectedObstacles = battleGetAllObstaclesOnPos(unit->getPosition(), false);
		if(unit->doubleWide())
		{
			BattleHex otherHex = unit->occupiedHex();
			if(otherHex.isValid() && !passed.contains(otherHex))
				for(auto & i : battleGetAllObstaclesOnPos(otherHex, false))
					if(!vstd::contains(affectedObstacles, i))
						affectedObstacles.push_back(i);
		}
		for(const auto & hex : unit->getHexes())
			if(hex == BattleHex::GATE_BRIDGE && battleIsGatePassable())
				for(int i=0; i<affectedObstacles.size(); i++)
					if(affectedObstacles.at(i)->obstacleType == CObstacleInstance::MOAT)
						affectedObstacles.erase(affectedObstacles.begin()+i);
	}
	return affectedObstacles;
}

bool CBattleInfoCallback::handleObstacleTriggersForUnit(SpellCastEnvironment & spellEnv, const battle::Unit & unit, const BattleHexArray & passed) const
{
	if(!unit.alive())
		return false;
	bool movementStopped = false;
	for(auto & obstacle : getAllAffectedObstaclesByStack(&unit, passed))
	{
		//helper info
		const SpellCreatedObstacle * spellObstacle = dynamic_cast<const SpellCreatedObstacle *>(obstacle.get());

		if(spellObstacle)
		{
			auto revealObstacles = [&](const SpellCreatedObstacle & spellObstacle) -> void
			{
				// For the hidden spell created obstacles, e.g. QuickSand, it should be revealed after taking damage
				auto operation = ObstacleChanges::EOperation::UPDATE;
				if (spellObstacle.removeOnTrigger)
					operation = ObstacleChanges::EOperation::REMOVE;

				SpellCreatedObstacle changedObstacle;
				changedObstacle.uniqueID = spellObstacle.uniqueID;
				changedObstacle.revealed = true;

				BattleObstaclesChanged bocp;
				bocp.battleID = getBattle()->getBattleID();
				bocp.change = ObstacleChanges(spellObstacle.uniqueID, operation);
				changedObstacle.toInfo(bocp.change, operation);
				spellEnv.apply(bocp);
			};
			const auto side = unit.unitSide();
			auto shouldReveal = !spellObstacle->hidden || !battleIsObstacleVisibleForSide(*obstacle, side);
			// A neutral obstacle (casterSide == NONE) belongs to no side and must impede units of both sides;
			// treat it as hostile to whichever unit triggered it so its effect still applies (and to avoid an invalid side lookup)
			const bool neutralObstacle = spellObstacle->casterSide != BattleSide::ATTACKER && spellObstacle->casterSide != BattleSide::DEFENDER;
			const auto casterSide = neutralObstacle ? otherSide(side) : spellObstacle->casterSide;
			const auto * hero = neutralObstacle ? nullptr : battleGetFightingHero(casterSide);
			auto caster = spells::ObstacleCasterProxy(getBattle()->getSidePlayer(casterSide), hero, *spellObstacle);

			if(obstacle->triggersEffects() && obstacle->getTrigger().hasValue())
			{
				const auto * sp = obstacle->getTrigger().toSpell();
				auto cast = spells::BattleCast(this, &caster, spells::Mode::PASSIVE, sp);
				spells::detail::ProblemImpl ignored;
				auto target = spells::Target(1, spells::Destination(&unit));
				if(sp->battleMechanics(&cast)->canBeCastAt(target, ignored)) // Obstacles should not be revealed by immune creatures
				{
					if(shouldReveal) { //hidden obstacle triggers effects after revealed
						revealObstacles(*spellObstacle);
						cast.cast(&spellEnv, target);
					}
				}
			}
			else if(shouldReveal)
				revealObstacles(*spellObstacle);
		}

		if(!unit.alive())
			return false;

		if(obstacle->stopsMovement() && !unit.hasBonusOfType(BonusType::FLYING))
			movementStopped = true;
	}

	return unit.alive() && !movementStopped;
}

AccessibilityInfo CBattleInfoCallback::getAccessibility() const
{
	AccessibilityInfo ret;
	ret.fill(EAccessibility::ACCESSIBLE);

	//removing accessibility for side columns of hexes
	for(int y = 0; y < GameConstants::BFIELD_HEIGHT; y++)
	{
		ret[BattleHex(GameConstants::BFIELD_WIDTH - 1, y).toInt()] = EAccessibility::SIDE_COLUMN;
		ret[BattleHex(0, y).toInt()] = EAccessibility::SIDE_COLUMN;
	}

	//special battlefields with logically unavailable tiles
	auto bFieldType = battleGetBattlefieldType();

	if(bFieldType != BattleField::NONE)
	{
		for(const auto & hex : bFieldType.getInfo()->impassableHexes)
			ret[hex.toInt()] = EAccessibility::UNAVAILABLE;
	}

	//gate -> should be before stacks
	if(battleGetFortifications().wallsHealth > 0)
	{
		EAccessibility accessibility = EAccessibility::ACCESSIBLE;
		switch(battleGetGateState())
		{
		case EGateState::CLOSED:
			accessibility = EAccessibility::GATE;
			break;

		case EGateState::BLOCKED:
			accessibility = EAccessibility::UNAVAILABLE;
			break;
		}
		ret[BattleHex::GATE_OUTER] = ret[BattleHex::GATE_INNER] = accessibility;
	}

	//tiles occupied by standing stacks
	for(const auto * unit : battleAliveUnits())
	{
		for(const auto & hex : unit->getHexes())
			if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
				ret[hex.toInt()] = EAccessibility::ALIVE_STACK;
	}

	//obstacles
	for(const auto &obst : battleGetAllObstacles())
	{
		for(const auto & hex : obst->getBlockedTiles())
			ret[hex.toInt()] = EAccessibility::OBSTACLE;
	}

	//walls
	if(battleGetFortifications().wallsHealth > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 62, 112, 147, 165};
		for(const auto & hex : permanentlyLocked)
			ret[hex] = EAccessibility::UNAVAILABLE;

		//TODO likely duplicated logic
		static const std::pair<EWallPart, BattleHex> lockedIfNotDestroyed[] =
		{
			//which part of wall, which hex is blocked if this part of wall is not destroyed
			std::make_pair(EWallPart::BOTTOM_WALL, BattleHex(BattleHex::DESTRUCTIBLE_WALL_4)),
			std::make_pair(EWallPart::BELOW_GATE, BattleHex(BattleHex::DESTRUCTIBLE_WALL_3)),
			std::make_pair(EWallPart::OVER_GATE, BattleHex(BattleHex::DESTRUCTIBLE_WALL_2)),
			std::make_pair(EWallPart::UPPER_WALL, BattleHex(BattleHex::DESTRUCTIBLE_WALL_1))
		};

		for(const auto & elem : lockedIfNotDestroyed)
		{
			if(battleGetWallState(elem.first) != EWallState::DESTROYED)
				ret[elem.second.toInt()] = EAccessibility::DESTRUCTIBLE_WALL;
		}
	}

	return ret;
}

AccessibilityInfo CBattleInfoCallback::getAccessibility(const battle::Unit * stack) const
{
	return getAccessibility(stack->getHexes());
}

AccessibilityInfo CBattleInfoCallback::getAccessibility(const BattleHexArray & accessibleHexes) const
{
	auto ret = getAccessibility();
	for(const auto & hex : accessibleHexes)
		if(hex.isValid())
			ret[hex.toInt()] = EAccessibility::ACCESSIBLE;

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const AccessibilityInfo & accessibility, const ReachabilityInfo::Parameters & params) const
{
	ReachabilityInfo ret;
	ret.accessibility = accessibility;
	ret.params = params;

	ret.predecessors.fill(BattleHex::INVALID);
	ret.distances.fill(ReachabilityInfo::INFINITE_DIST);

	if(!params.startPosition.isValid()) //if got call for arrow turrets
		return ret;

	const BattleHexArray obstacles = getStoppers(params.perspective);
	auto checkParams = params;
	checkParams.ignoreKnownAccessible = true; //Ignore starting hexes obstacles

	std::queue<BattleHex> hexq; //bfs queue

	//first element
	hexq.push(params.startPosition);
	ret.distances[params.startPosition.toInt()] = 0;

	std::array<bool, GameConstants::BFIELD_SIZE> accessibleCache{};
	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
		accessibleCache[hex] = accessibility.accessible(hex, params.doubleWide, params.side, params.canUseGate);

	while(!hexq.empty()) //bfs loop
	{
		const BattleHex curHex = hexq.front();
		hexq.pop();

		//walking stack can't step past the obstacles
		if(isInObstacle(curHex, obstacles, checkParams))
			continue;

		const int costToNeighbour = ret.distances.at(curHex.toInt()) + 1;

		for(const BattleHex & neighbour : curHex.getNeighbouringTiles())
		{
			auto additionalCost = 0;

			if(params.bypassEnemyStacks)
			{
				auto enemyToBypass = params.destructibleEnemyTurns.at(neighbour.toInt());

				if(enemyToBypass >= 0)
				{
					additionalCost = enemyToBypass;
				}
			}

			const int costFoundSoFar = ret.distances[neighbour.toInt()];

			if(accessibleCache[neighbour.toInt()] && costToNeighbour + additionalCost < costFoundSoFar)
			{
				hexq.push(neighbour);
				ret.distances[neighbour.toInt()] = costToNeighbour + additionalCost;
				ret.predecessors[neighbour.toInt()] = curHex;
			}
		}
	}

	return ret;
}

bool CBattleInfoCallback::isInObstacle(
	const BattleHex & hex,
	const BattleHexArray & obstacleHexes,
	const ReachabilityInfo::Parameters & params) const
{

	for(const auto & occupiedHex : battle::Unit::getHexes(hex, params.doubleWide, params.side))
	{
		if(params.ignoreKnownAccessible && params.knownAccessible->contains(occupiedHex))
			continue;

		if(obstacleHexes.contains(occupiedHex))
		{
			if(occupiedHex == BattleHex::GATE_BRIDGE)
			{
				if(battleGetGateState() != EGateState::DESTROYED && !params.canUseGate)
					return true;
			}
			else
				return true;
		}
	}

	return false;
}

BattleHexArray CBattleInfoCallback::getStoppers(BattleSide whichSidePerspective) const
{
	BattleHexArray ret;
	THROW_IF_NOT_BATTLE();

	for(auto &oi : battleGetAllObstacles(whichSidePerspective))
	{
		if(!battleIsObstacleVisibleForSide(*oi, whichSidePerspective))
			continue;

		for(const auto & hex : oi->getStoppingTile())
		{
			if(hex == BattleHex::GATE_BRIDGE && oi->obstacleType == CObstacleInstance::MOAT)
			{
				if(battleGetGateState() == EGateState::OPENED || battleGetGateState() == EGateState::DESTROYED)
					continue; // this tile is disabled by drawbridge on top of it
			}
			ret.insert(hex);
		}
	}
	return ret;
}

BattleHex CBattleInfoCallback::getClosestHexToTargetInRange(const ReachabilityInfo & cache, const Unit & unit, const BattleHex & targetHex) const
{
	if (unit.hasBonusOfType(BonusType::FLYING))
	{
		BattleHexArray reachableHexes = battleGetAvailableHexes(cache, &unit, false);
		return std::ranges::min_element(reachableHexes, [&targetHex](const BattleHex & lhs, const BattleHex & rhs)
		{
			return BattleHex::getDistance(lhs, targetHex) < BattleHex::getDistance(rhs, targetHex);
		})[0];
	}

	BattleHexArray path = getPath(unit.getPosition(), targetHex, &unit).first; //TODO: does not find path through moat
	if(!path.empty())
	{
		int pathHexIndex = path.size() - unit.getMovementRange();
		if(pathHexIndex < 0)
		{
			return targetHex;
		}
		return path[pathHexIndex];
	}

	// FALLBACK: If path is empty (target blocked by obstacles/units),
	// find the reachable hex that is geometrically closest to the target.
	BattleHexArray reachableHexes = battleGetAvailableHexes(cache, &unit, false);
	if (reachableHexes.empty())
		return BattleHex::INVALID;

	return *vstd::minElementByFun(reachableHexes, [&](const BattleHex & h)
	{
		return BattleHex::getDistance(h, targetHex);
	});
}

ForcedAction CBattleInfoCallback::getBerserkForcedAction(const battle::Unit * berserker) const
{
	logGlobal->trace("Handle Berserk effect");
	auto targets = battleGetUnitsIf([&berserker](const battle::Unit * u)
	{
		return u->isValidTarget(false) && !u->isInvincible() && u->unitId() != berserker->unitId();
	});

	if (targets.empty())
	{
		logGlobal->trace("No attackable target found for berserk unit");
		ForcedAction result = {
			EActionType::NO_ACTION,
			berserker->getPosition(),
			nullptr
		};
		return result;
	}

	auto cache = getReachability(berserker);

	if (battleCanShoot(berserker))
	{
		const auto target = std::ranges::min_element(targets, [&berserker](const battle::Unit * lhs, const battle::Unit * rhs)
		{
			return BattleHex::getDistance(berserker->getPosition(), lhs->getPosition()) < BattleHex::getDistance(berserker->getPosition(), rhs->getPosition());
		})[0];
		ForcedAction result = {
			EActionType::SHOOT,
			berserker->getPosition(),
			target
		};
		return result;
	}
	else
	{
		struct TargetData
		{
			const battle::Unit * target;
			BattleHex closestAttackableHex;
			uint32_t distance;
		};

		std::vector<TargetData> targetData;
		targetData.reserve(targets.size());
		for (const battle::Unit * uTarget : targets)
		{
			BattleHexArray attackableHexes = uTarget->getAttackableHexes(berserker);
			auto closestAttackableHex = std::ranges::min_element(attackableHexes, [&cache](const BattleHex & lhs, const BattleHex & rhs)
			{
				return cache.distances[lhs.toInt()] < cache.distances[rhs.toInt()];
			})[0];
			uint32_t distance = cache.distances[closestAttackableHex.toInt()];
			TargetData temp = {uTarget, closestAttackableHex, distance};
			targetData.push_back(temp);
		}

		auto closestUnit = std::ranges::min_element(targetData, [](const TargetData & lhs, const TargetData & rhs)
		{
			return lhs.distance < rhs.distance;
		})[0];

		if (closestUnit.distance <= berserker->getMovementRange())
		{
			ForcedAction result = {
				EActionType::WALK_AND_ATTACK,
				closestUnit.closestAttackableHex,
				closestUnit.target
			};
			return result;
		}
		else if (closestUnit.distance != ReachabilityInfo::INFINITE_DIST && berserker->getMovementRange() > 0)
		{
			BattleHex intermediaryHex = getClosestHexToTargetInRange(cache, *berserker, closestUnit.closestAttackableHex);

			ForcedAction result = {
				EActionType::WALK,
				intermediaryHex,
				closestUnit.target
			};
			return result;
		}
		else
		{
			logGlobal->trace("No target found or unit cannot move");
			ForcedAction result = {
				EActionType::NO_ACTION,
				berserker->getPosition(),
				nullptr
			};
			return result;
		}
	}
}

BattleHex CBattleInfoCallback::getAvailableHex(const Creature * creature, BattleSide side, BattleHex initialPos) const
{
	bool twoHex = creature->isDoubleWide();

	BattleHex pos;
	if (initialPos.isValid())
		pos = initialPos;
	else //summon elementals depending on player side
	{
 		if(side == BattleSide::ATTACKER)
	 		pos = 0; //top left
 		else
 			pos = GameConstants::BFIELD_WIDTH - 1; //top right
 	}

	auto accessibility = getAccessibility();

	BattleHexArray occupyable;
	for(int i = 0; i < accessibility.size(); i++)
		if(accessibility.accessible(i, twoHex, side, side == BattleSide::DEFENDER))
			occupyable.insert(i);

	if(occupyable.empty())
	{
		return BattleHex::INVALID; //all tiles are covered
	}

	return BattleHex::getClosestTile(side, pos, occupyable);
}

si8 CBattleInfoCallback::battleGetTacticDist() const
{
	THROW_IF_NOT_BATTLE();

	//TODO get rid of this method
	if(battleDoWeKnowAbout(battleGetTacticsSide()))
		return battleTacticDist();

	return 0;
}

bool CBattleInfoCallback::isInTacticRange(const BattleHex & dest) const
{
	THROW_IF_NOT_BATTLE();
	auto side = battleGetTacticsSide();
	auto dist = battleGetTacticDist();

	if (side == BattleSide::ATTACKER && dest.getX() > 0 && dest.getX() <= dist)
		return true;

	if (side == BattleSide::DEFENDER && dest.getX() < GameConstants::BFIELD_WIDTH - 1 && dest.getX() >= GameConstants::BFIELD_WIDTH - dist - 1)
		return true;

	return false;
}

ReachabilityInfo CBattleInfoCallback::getReachability(const battle::Unit * unit) const
{
	return getReachability(ReachabilityInfo::Parameters(unit, unit->getPosition()));
}

ReachabilityInfo CBattleInfoCallback::getReachability(const ReachabilityInfo::Parameters & params) const
{
	if(params.perspective != BattleSide::ALL_KNOWING && !battleDoWeKnowAbout(params.perspective))
	{
		// we can't see what the unit's side knows, e.g. hidden obstacles, when it is an enemy unit
		// or a unit that its side lost to Hypnotize, so our own knowledge is used instead
		ReachabilityInfo::Parameters ownParams = params;
		ownParams.perspective = battleGetMySide();
		return getReachability(ownParams);
	}

	if(params.flying)
		return getFlyingReachability(params);
	else
	{
		auto accessibility = getAccessibility(* params.knownAccessible);

		accessibility.destructibleEnemyTurns = std::shared_ptr<const TBattlefieldTurnsArray>(
			& params.destructibleEnemyTurns,
			[](const TBattlefieldTurnsArray *) { }
		);

		return makeBFS(accessibility, params);
	}
}

ReachabilityInfo CBattleInfoCallback::getFlyingReachability(const ReachabilityInfo::Parameters & params) const
{
	ReachabilityInfo ret;
	ret.accessibility = getAccessibility(* params.knownAccessible);

	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		if(ret.accessibility.accessible(i, params.doubleWide, params.side, params.canUseGate))
		{
			ret.predecessors[i] = params.startPosition;
			ret.distances[i] = BattleHex::getDistance(params.startPosition, i);
		}
	}

	return ret;
}

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes(
	const battle::Unit * attacker,
	BattleHex destinationTile,
	BattleHex attackerPos) const
{
	const auto * defender = battleGetUnitByPos(destinationTile, true);

	if(!defender)
		return AttackableTiles(); // can't attack thin air

	return getPotentiallyAttackableHexes(
		attacker,
		defender,
		destinationTile,
		attackerPos,
		defender->getPosition());
}

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes(
	const battle::Unit* attacker,
	const battle::Unit * defender,
	BattleHex destinationTile,
	BattleHex attackerPos,
	BattleHex defenderPos) const
{
	//does not return hex attacked directly
	AttackableTiles at;
	THROW_IF_NOT_BATTLE();

	BattleHex attackOriginHex = (attackerPos.toInt() != BattleHex::INVALID) ? attackerPos : attacker->getPosition(); //real or hypothetical (cursor) position
	
	defenderPos = (defenderPos.toInt() != BattleHex::INVALID) ? defenderPos : defender->getPosition(); //real or hypothetical (cursor) position
	
	if(attacker->hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT))
		at.hostileCreaturePositions.insert(attacker->getSurroundingHexes(attackerPos));

	// If attacker is double-wide and its head is not adjacent to enemy we need to turn around
	if(attacker->doubleWide() && !vstd::contains(defender->getSurroundingHexes(defenderPos), attackOriginHex))
		attackOriginHex = attacker->occupiedHex(attackOriginHex);

	auto attackDirection = BattleHex::mutualPosition(attackOriginHex, defenderPos);

	// If defender is double-wide, attacker always prefers targeting its 'tail', if it is reachable
	if(defender->doubleWide() && BattleHex::mutualPosition(attackOriginHex, defender->occupiedHex(defenderPos)) != BattleHex::NONE)
		attackDirection = BattleHex::mutualPosition(attackOriginHex, defender->occupiedHex(defenderPos));

	if(attackDirection == BattleHex::NONE)
		throw std::runtime_error("!!!");

	const auto & processTargets = [&](const std::vector<int> & additionalTargets) -> BattleHexArray
	{
		BattleHexArray output;

		for (int targetPath : additionalTargets)
		{
			BattleHex target = attackOriginHex;
			std::vector<BattleHex::EDir> path;

			for (int targetPathLeft = targetPath; targetPathLeft != 0; targetPathLeft /= 10)
				path.push_back(static_cast<BattleHex::EDir>((attackDirection + targetPathLeft % 10 - 1) % 6));

			try
			{
				if(attacker->doubleWide() && attacker->coversPos(target.cloneInDirection(path.front())))
					target.moveInDirection(attackDirection);

				for(BattleHex::EDir nextDirection : path)
					target.moveInDirection(nextDirection);
			}
			catch(const std::out_of_range &)
			{
				// Hex out of range, for example outside of battlefield. This is valid situation, so skip this hex
				continue;
			}

			if (target.isValid() && !attacker->coversPos(target))
				output.insert(target);
		}
		return output;
	};

	const auto multihexUnit = attacker->getBonusesOfType(BonusType::MULTIHEX_UNIT_ATTACK);
	const auto multihexEnemy = attacker->getBonusesOfType(BonusType::MULTIHEX_ENEMY_ATTACK);
	const auto multihexAnimation = attacker->getBonusesOfType(BonusType::MULTIHEX_ANIMATION);

	for (const auto & bonus : *multihexUnit)
		at.friendlyCreaturePositions.insert(processTargets(bonus->parameters->toVector()));

	for (const auto & bonus : *multihexEnemy)
		at.hostileCreaturePositions.insert(processTargets(bonus->parameters->toVector()));

	for (const auto & bonus : *multihexAnimation)
		at.overrideAnimationPositions.insert(processTargets(bonus->parameters->toVector()));

	if(attacker->hasBonusOfType(BonusType::THREE_HEADED_ATTACK))
		at.hostileCreaturePositions.insert(processTargets({2,6}));

	if(attacker->hasBonusOfType(BonusType::WIDE_BREATH))
		at.friendlyCreaturePositions.insert(processTargets({ 11, 111, 2, 12, 6, 16 }));

	if(attacker->hasBonusOfType(BonusType::TWO_HEX_ATTACK_BREATH))
		at.friendlyCreaturePositions.insert(processTargets({ 11 }));

	if (attacker->hasBonusOfType(BonusType::PRISM_HEX_ATTACK_BREATH))
		at.friendlyCreaturePositions.insert(processTargets({ 11, 12, 16 }));

	return at;
}

AttackableTiles CBattleInfoCallback::getPotentiallyShootableHexes(const battle::Unit * attacker, const BattleHex & destinationTile, const BattleHex & attackerPos) const
{
	//does not return hex attacked directly
	AttackableTiles at;
	THROW_IF_NOT_BATTLE();

	if(attacker->hasBonusOfType(BonusType::SHOOTS_ALL_ADJACENT) && !attackerPos.getNeighbouringTiles().contains(destinationTile))
	{
		at.hostileCreaturePositions.insert(destinationTile.getNeighbouringTiles());
		at.hostileCreaturePositions.insert(destinationTile);
	}

	return at;
}

battle::Units CBattleInfoCallback::getAttackedBattleUnits(
	const battle::Unit * attacker,
	const  battle::Unit * defender,
	BattleHex destinationTile,
	bool rangedAttack,
	BattleHex attackerPos,
	BattleHex defenderPos) const
{
	battle::Units units;
	THROW_IF_NOT_BATTLE();

	if(attackerPos == BattleHex::INVALID)
		attackerPos = attacker->getPosition();

	if(defenderPos == BattleHex::INVALID)
		defenderPos = defender->getPosition();

	AttackableTiles at;

	if (rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
		at = getPotentiallyAttackableHexes(attacker, defender, destinationTile, attackerPos, defenderPos);

	units = battleGetUnitsIf([this, at, attacker](const battle::Unit * unit)
	{
		if (unit->isGhost() || !unit->alive() || unit->isInvincible())
			return false;

		for (const BattleHex & hex : unit->getHexes())
		{
			if(at.hostileCreaturePositions.contains(hex)
				&& battleGetOwner(unit) != battleGetOwner(attacker))
				return true;
			if (at.friendlyCreaturePositions.contains(hex))
				return true;
		}
		return false;
	});

	return units;
}

std::pair<battle::Units, bool> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, const BattleHex & destinationTile, bool rangedAttack, BattleHex attackerPos) const
{
	std::pair<battle::Units, bool> attackedCres;
	THROW_IF_NOT_BATTLE();

	AttackableTiles at;
	
	if(rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
	{
		try
		{
			at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);
		}
		catch(const std::runtime_error &)
		{
			return attackedCres;
		}
	}

	// a double-wide unit is found through both of its hexes, so the same unit shows up twice
	const auto & addOnce = [&attackedCres](const battle::Unit * unit)
	{
		if(!vstd::contains(attackedCres.first, unit))
			attackedCres.first.push_back(unit);
	};

	for (const BattleHex & tile : at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && battleGetOwner(st) != battleGetOwner(attacker) && !st->isInvincible()) //only hostile stacks - does it work well with Berserk?
			addOnce(st);
	}
	for (const BattleHex & tile : at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && !st->isInvincible()) //friendly stacks can also be damaged by Dragon Breath
			addOnce(st);
	}

	if (at.friendlyCreaturePositions.empty())
	{
		attackedCres.second = !attackedCres.first.empty();
	}
	else
	{
		for (const BattleHex & tile : at.friendlyCreaturePositions)
			for (const auto & st : attackedCres.first)
				if (st->coversPos(tile))
					attackedCres.second = true;
	}

	return attackedCres;
}

static bool isHexInFront(const BattleHex & hex, const BattleHex & testHex, BattleSide side )
{
	static const std::set<BattleHex::EDir> rightDirs { BattleHex::BOTTOM_RIGHT, BattleHex::TOP_RIGHT, BattleHex::RIGHT };
	static const std::set<BattleHex::EDir> leftDirs  { BattleHex::BOTTOM_LEFT, BattleHex::TOP_LEFT, BattleHex::LEFT };

	auto mutualPos = BattleHex::mutualPosition(hex, testHex);

	if (side == BattleSide::ATTACKER)
		return rightDirs.count(mutualPos);
	else
		return leftDirs.count(mutualPos);
}

//TODO: this should apply also to mechanics and cursor interface
bool CBattleInfoCallback::isToReverse(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerHex, BattleHex defenderHex) const
{
	if(!defenderHex.isValid())
		defenderHex = defender->getPosition();

	if(!attackerHex.isValid())
		attackerHex = attacker->getPosition();

	if (attackerHex < 0 ) //turret
		return false;

	if(isHexInFront(attackerHex, defenderHex, attacker->unitSide()))
		return false;

	auto defenderOtherHex = defenderHex;
	auto attackerOtherHex = defenderHex;

	if (defender->doubleWide())
	{
		defenderOtherHex = battle::Unit::occupiedHex(defenderHex, true, defender->unitSide());

		if(isHexInFront(attackerHex, defenderOtherHex, attacker->unitSide()))
			return false;
	}

	if (attacker->doubleWide())
	{
		attackerOtherHex = battle::Unit::occupiedHex(attackerHex, true, attacker->unitSide());

		if(isHexInFront(attackerOtherHex, defenderHex, attacker->unitSide()))
			return false;
	}

	// a bit weird case since here defender is slightly behind attacker, so reversing seems preferable,
	// but this is how H3 handles it which is important, e.g. for direction of dragon breath attacks
	if (attacker->doubleWide() && defender->doubleWide())
	{
		if(isHexInFront(attackerOtherHex, defenderOtherHex, attacker->unitSide()))
			return false;
	}
	return true;
}

ReachabilityInfo::TDistances CBattleInfoCallback::battleGetDistances(const battle::Unit * unit, const BattleHex & assumedPosition) const
{
	ReachabilityInfo::TDistances ret;
	ret.fill(-1);
	THROW_IF_NOT_BATTLE();

	auto reachability = getReachability(unit);

	std::ranges::copy(reachability.distances, ret.begin());

	return ret;
}

bool CBattleInfoCallback::battleHasDistancePenalty(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHex & destHex) const
{
	THROW_IF_NOT_BATTLE();

	const std::string cachingStrNoDistancePenalty = "type_NO_DISTANCE_PENALTY";
	static const auto selectorNoDistancePenalty = Selector::type()(BonusType::NO_DISTANCE_PENALTY);

	if(shooter->hasBonus(selectorNoDistancePenalty, cachingStrNoDistancePenalty))
		return false;

	if(const auto * target = battleGetUnitByPos(destHex, true))
	{
		//If any hex of target creature is within range, there is no penalty
		int range = GameConstants::BATTLE_SHOOTING_PENALTY_DISTANCE;

		auto bonus = shooter->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
		if(bonus != nullptr && bonus->parameters)
			range = bonus->parameters->toNumber();

		if(isEnemyUnitWithinSpecifiedRange(shooterPosition, target, range))
			return false;
	}
	else
	{
		if(BattleHex::getDistance(shooterPosition, destHex) <= GameConstants::BATTLE_SHOOTING_PENALTY_DISTANCE)
			return false;
	}

	return true;
}

bool CBattleInfoCallback::isEnemyUnitWithinSpecifiedRange(const BattleHex & attackerPosition, const battle::Unit * defenderUnit, unsigned int range) const
{
	for(const auto & hex : defenderUnit->getHexes())
		if(BattleHex::getDistance(attackerPosition, hex) <= range)
			return true;
	
	return false;
}

bool CBattleInfoCallback::isHexWithinSpecifiedRange(const BattleHex & attackerPosition, const BattleHex & targetPosition, unsigned int range) const
{
	if(BattleHex::getDistance(attackerPosition, targetPosition) <= range)
		return true;

	return false;
}

BattleHex CBattleInfoCallback::wallPartToBattleHex(EWallPart part) const
{
	THROW_IF_NOT_BATTLE();
	return WallPartToHex(part);
}

EWallPart CBattleInfoCallback::battleHexToWallPart(const BattleHex & hex) const
{
	THROW_IF_NOT_BATTLE();
	return hexToWallPart(hex);
}

bool CBattleInfoCallback::isWallPartPotentiallyAttackable(EWallPart wallPart) const
{
	THROW_IF_NOT_BATTLE();
	return wallPart != EWallPart::INDESTRUCTIBLE_PART && wallPart != EWallPart::INDESTRUCTIBLE_PART_OF_GATE &&
																	wallPart != EWallPart::INVALID;
}

bool CBattleInfoCallback::isWallPartAttackable(EWallPart wallPart) const
{
	THROW_IF_NOT_BATTLE();

	if(isWallPartPotentiallyAttackable(wallPart))
	{
		auto wallState = battleGetWallState(wallPart);
		return (wallState != EWallState::NONE && wallState != EWallState::DESTROYED);
	}
	return false;
}

BattleHexArray CBattleInfoCallback::getAttackableWallParts() const
{
	BattleHexArray attackableBattleHexes;
	THROW_IF_NOT_BATTLE();

	for(const auto & wallPartPair : wallParts)
	{
		if(isWallPartAttackable(wallPartPair.second))
			attackableBattleHexes.insert(wallPartPair.first);
	}

	return attackableBattleHexes;
}

int32_t CBattleInfoCallback::battleGetSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const
{
	THROW_IF_NOT_BATTLE();
	//TODO should be replaced using bonus system facilities (propagation onto battle node)

	int32_t ret = caster->getSpellCost(sp);

	//checking for friendly stacks reducing cost of the spell and
	//enemy stacks increasing it
	int32_t manaReduction = 0;
	int32_t manaIncrease = 0;

	for(const auto * unit : battleAliveUnits())
	{
		if(unit->unitOwner() == caster->tempOwner && unit->hasBonusOfType(BonusType::CHANGES_SPELL_COST_FOR_ALLY))
		{
			vstd::amax(manaReduction, unit->valOfBonuses(BonusType::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if(unit->unitOwner() != caster->tempOwner && unit->hasBonusOfType(BonusType::CHANGES_SPELL_COST_FOR_ENEMY))
		{
			vstd::amax(manaIncrease, unit->valOfBonuses(BonusType::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return std::max(0, ret - manaReduction + manaIncrease);
}

bool CBattleInfoCallback::battleHasShootingPenalty(const battle::Unit * shooter, const BattleHex & destHex) const
{
	return battleHasDistancePenalty(shooter, shooter->getPosition(), destHex) || battleHasWallPenalty(shooter, shooter->getPosition(), destHex);
}

bool CBattleInfoCallback::battleIsUnitBlocked(const battle::Unit * unit) const
{
	THROW_IF_NOT_BATTLE();

	bool isBerserk = unit->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE);
	for(const auto * adjacent : battleAdjacentUnits(unit))
	{
		if(battleMatchOwner(unit, adjacent) || isBerserk)
			return true;
	}
	return false;
}

battle::Units CBattleInfoCallback::battleAdjacentUnits(const battle::Unit * unit) const
{
	THROW_IF_NOT_BATTLE();

	const auto & hexes = unit->getSurroundingHexes();

	const auto & units = battleGetUnitsIf([&hexes](const battle::Unit * testedUnit)
	{
		if (!testedUnit->alive())
			return false;
		const auto & unitHexes = testedUnit->getHexes();
		for (const auto & hex : unitHexes)
			if (hexes.contains(hex))
				return true;
		return false;
	});

	return units;
}

std::vector<SpellID> CBattleInfoCallback::getBeneficialSpells(const battle::Unit * caster, const battle::Unit * subject) const
{
	THROW_IF_NOT_BATTLE();
	//This is complete list. No spells from mods.
	//todo: this should be Spellbook of caster Stack
	static const std::set<SpellID> allPossibleSpells =
	{
		SpellID::AIR_SHIELD,
		SpellID::ANTI_MAGIC,
		SpellID::BLESS,
		SpellID::BLOODLUST,
		SpellID::COUNTERSTRIKE,
		SpellID::CURE,
		SpellID::FIRE_SHIELD,
		SpellID::FORTUNE,
		SpellID::HASTE,
		SpellID::MAGIC_MIRROR,
		SpellID::MIRTH,
		SpellID::PRAYER,
		SpellID::PRECISION,
		SpellID::PROTECTION_FROM_AIR,
		SpellID::PROTECTION_FROM_EARTH,
		SpellID::PROTECTION_FROM_FIRE,
		SpellID::PROTECTION_FROM_WATER,
		SpellID::SHIELD,
		SpellID::SLAYER,
		SpellID::STONE_SKIN
	};
	std::vector<SpellID> beneficialSpells;

	auto getAliveEnemy = [&](const std::function<bool(const CStack *)> & pred) -> const CStack *
	{
		auto stacks = battleGetStacksIf([&](const CStack * stack)
		{
			return pred(stack) && battleMatchOwner(subject, stack) && stack->isValidTarget(false);
		});

		if(stacks.empty())
			return nullptr;
		else
			return stacks.front();
	};

	for(const SpellID& spellID : allPossibleSpells)
	{
		std::stringstream cachingStr;
		cachingStr << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT) << "id_" << spellID.num;

		if(subject->hasBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spellID)), cachingStr.str()))
			continue;

		auto spellPtr = spellID.toSpell();
		spells::Target target;
		target.emplace_back(subject);

		spells::BattleCast cast(this, caster, spells::Mode::CREATURE_ACTIVE, spellPtr);

		auto m = spellPtr->battleMechanics(&cast);
		if (!m->canBeCastAt(target))
			continue;

		switch (spellID.toEnum())
		{
		case SpellID::SHIELD:
		case SpellID::FIRE_SHIELD: // not if all enemy units are shooters
		{
			const auto * walker = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return !stack->canShoot();
			});

			if(!walker)
				continue;
		}
			break;
		case SpellID::AIR_SHIELD: //only against active shooters
		{
			const auto * shooter = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return stack->canShoot();
			});
			if(!shooter)
				continue;
		}
			break;
		case SpellID::ANTI_MAGIC:
		case SpellID::MAGIC_MIRROR:
		case SpellID::PROTECTION_FROM_AIR:
		case SpellID::PROTECTION_FROM_EARTH:
		case SpellID::PROTECTION_FROM_FIRE:
		case SpellID::PROTECTION_FROM_WATER:
		{
			const BattleSide enemySide = otherSide(subject->unitSide());
			//todo: only if enemy has spellbook
			if (!battleHasHero(enemySide)) //only if there is enemy hero
				continue;
		}
			break;
		case SpellID::CURE: //only damaged units
		{
			//do not cast on affected by debuffs
			if(subject->getFirstHPleft() == subject->getMaxHealth())
				continue;
		}
			break;
		case SpellID::BLOODLUST:
		{
			if(subject->canShoot()) //TODO: if can shoot - only if enemy units are adjacent
				continue;
		}
			break;
		case SpellID::PRECISION:
		{
			if(!subject->canShoot())
				continue;
		}
			break;
		case SpellID::SLAYER://only if monsters are present
		{
			const auto * kingMonster = getAliveEnemy([&](const CStack * stack) -> bool //look for enemy, non-shooting stack
			{
				return stack->hasBonusOfType(BonusType::KING);
			});

			if (!kingMonster)
				continue;
		}
			break;
		}
		beneficialSpells.push_back(spellID);
	}

	return beneficialSpells;
}

SpellID CBattleInfoCallback::getRandomCastedSpell(vstd::RNG & rand, const CStack * caster) const
{
	THROW_IF_NOT_BATTLE();

	TConstBonusListPtr bl = caster->getBonusesOfType(BonusType::SPELLCASTER);

	if(bl->empty())
		return SpellID::NONE;

	std::vector<int> weights;

	//spells with 0 weight are non-random, exclude them
	for(const auto & b : *bl)
	{
		if(b->parameters && b->parameters->toNumber() > 0)
			weights.push_back(b->parameters->toNumber());
		else
			weights.push_back(0);
	}

	int64_t itemIndex = RandomGeneratorUtil::nextItemWeighted(weights, rand);

	if(itemIndex < 0)
		return SpellID::NONE;

	auto result = *(bl->begin() + itemIndex);
	return result->subtype.as<SpellID>();
}

int CBattleInfoCallback::battleGetSurrenderCost(const PlayerColor & Player) const
{
	THROW_IF_NOT_BATTLE();
	if(!battleCanSurrender(Player))
		return -1;

	const BattleSide side = playerToSide(Player);
	if(side == BattleSide::NONE)
		return -1;

	int ret = 0;
	double discount = 0;

	for(const auto * unit : battleAliveUnits(side))
		ret += unit->getRawSurrenderCost();

	//H3 - hero pays half of the recruit cost of his remaining army
	double costDivisor = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_SURRENDER_COST_DIVISOR);
	if(costDivisor > 0)
		ret = static_cast<int>(ret / costDivisor);

	if(const CGHeroInstance * h = battleGetFightingHero(side))
		discount += h->valOfBonuses(BonusType::SURRENDER_DISCOUNT);

	ret = static_cast<int>(ret * (100.0 - discount) / 100.0);
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

si8 CBattleInfoCallback::battleMinSpellLevel(BattleSide side) const
{
	const IBonusBearer * node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBonusBearer();

	if(!node)
		return 0;

	auto b = node->getBonusesOfType(BonusType::BLOCK_MAGIC_BELOW);
	if(b->size())
		return b->totalValue();

	return 0;
}

si8 CBattleInfoCallback::battleMaxSpellLevel(BattleSide side) const
{
	const IBonusBearer *node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBonusBearer();

	if(!node)
		return GameConstants::SPELL_LEVELS;

	//We can't "just get value" - it'd be 0 if there are bonuses (and all would be blocked)
	auto b = node->getBonusesOfType(BonusType::BLOCK_MAGIC_ABOVE);
	if(b->size())
		return b->totalValue();

	return GameConstants::SPELL_LEVELS;
}

std::optional<BattleSide> CBattleInfoCallback::battleIsFinished() const
{
	if(!duringBattle())
		throw std::logic_error("battleIsFinished called when no battle!");

	auto units = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->alive() && !unit->isTurret() && !unit->hasBonusOfType(BonusType::SIEGE_WEAPON);
	});

	BattleSideArray<bool> hasUnit = {false, false}; //index is BattleSide

	for(auto & unit : units)
	{
		//todo: move SIEGE_WEAPON check to Unit state
		hasUnit.at(unit->unitSide()) = true;

		if(hasUnit[BattleSide::ATTACKER] && hasUnit[BattleSide::DEFENDER])
			return std::nullopt;
	}
	
	hasUnit = {false, false};

	for(auto & unit : units)
	{
		if(!unit->isClone() && !unit->acquireState()->summoned && !dynamic_cast <const CCommanderInstance *>(unit))
		{
			hasUnit.at(unit->unitSide()) = true;
		}
	}

	if(!hasUnit[BattleSide::ATTACKER] && !hasUnit[BattleSide::DEFENDER])
		return BattleSide::NONE;
	if(!hasUnit[BattleSide::DEFENDER])
		return BattleSide::ATTACKER;
	else
		return BattleSide::DEFENDER;
}

const scripting::Pool & CBattleInfoCallback::getScriptContextPool() const
{
	return getBattle()->getScriptContextPool();
}
