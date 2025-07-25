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
#include "DamageCalculator.h"
#include "IGameSettings.h"
#include "PossiblePlayerBattleAction.h"
#include "../entities/building/TownFortifications.h"
#include "../GameLibrary.h"
#include "../spells/ObstacleCasterProxy.h"
#include "../spells/ISpellMechanics.h"
#include "../spells/Problem.h"
#include "../spells/CSpellHandler.h"
#include "../mapObjects/CGTownInstance.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../BattleFieldHandler.h"
#include "../Rect.h"

VCMI_LIB_NAMESPACE_BEGIN

static BattleHex lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
{
	static const BattleHex lineToHex[] = {12, 29, 45, 62, 78, 96, 112, 130, 147, 165, 182};

	return lineToHex[line];
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
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	if(caster == nullptr)
	{
		logGlobal->error("CBattleInfoCallback::battleCanCastSpell: no spellcaster.");
		return ESpellCastProblem::INVALID;
	}
	const PlayerColor player = caster->getCasterOwner();
	const auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return ESpellCastProblem::INVALID;
	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->warn("You can't check if enemy can cast given spell!");
		return ESpellCastProblem::INVALID;
	}

	if(battleTacticDist())
		return ESpellCastProblem::ONGOING_TACTIC_PHASE;

	switch(mode)
	{
	case spells::Mode::HERO:
	{
		const auto * hero = caster->getHeroCaster();

		if(!hero)
			return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
		if(!hero->hasSpellbook())
			return ESpellCastProblem::NO_SPELLBOOK;
		if(hero->hasBonusOfType(BonusType::BLOCK_ALL_MAGIC))
			return ESpellCastProblem::MAGIC_IS_BLOCKED;
		if(battleCastSpells(side) >= hero->valOfBonuses(BonusType::HERO_SPELL_CASTS_PER_COMBAT_TURN))
			return ESpellCastProblem::CASTS_PER_TURN_LIMIT;
	}
		break;
	default:
		break;
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

	RETURN_IF_NOT_BATTLE(false);
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
	RETURN_IF_NOT_BATTLE(false);
	if(battleGetFortifications().wallsHealth == 0)
		return false;

	const std::string cachingStrNoWallPenalty = "type_NO_WALL_PENALTY";
	static const auto selectorNoWallPenalty = Selector::type()(BonusType::NO_WALL_PENALTY);

	if(shooter->hasBonus(selectorNoWallPenalty, cachingStrNoWallPenalty))
		return false;

	const auto shooterOutsideWalls = shooterPosition < lineToWallHex(shooterPosition.getY());

	return shooterOutsideWalls && battleHasPenaltyOnLine(shooterPosition, destHex, true, false);
}

std::vector<PossiblePlayerBattleAction> CBattleInfoCallback::getClientActionsForStack(const CStack * stack, const BattleClientInterfaceData & data)
{
	RETURN_IF_NOT_BATTLE(std::vector<PossiblePlayerBattleAction>());
	std::vector<PossiblePlayerBattleAction> allowedActionList;
	if(data.tacticsMode) //would "if(battleGetTacticDist() > 0)" work?
	{
		allowedActionList.push_back(PossiblePlayerBattleAction::MOVE_TACTICS);
		allowedActionList.push_back(PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK);
	}
	else
	{
		if(stack->canCast()) //TODO: check for battlefield effects that prevent casting?
		{
			if(stack->hasBonusOfType(BonusType::SPELLCASTER))
			{
				for(const auto & spellID : data.creatureSpellsToCast)
				{
					const CSpell *spell = spellID.toSpell();
					PossiblePlayerBattleAction act = getCasterAction(spell, stack, spells::Mode::CREATURE_ACTIVE);
					allowedActionList.push_back(act);
				}
			}
			if(stack->hasBonusOfType(BonusType::RANDOM_SPELLCASTER))
				allowedActionList.push_back(PossiblePlayerBattleAction::RANDOM_GENIE_SPELL);
		}
		if(stack->canShoot())
			allowedActionList.push_back(PossiblePlayerBattleAction::SHOOT);
		if(stack->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE))
			allowedActionList.push_back(PossiblePlayerBattleAction::ATTACK_AND_RETURN);

		allowedActionList.push_back(PossiblePlayerBattleAction::ATTACK); //all active stacks can attack
		allowedActionList.push_back(PossiblePlayerBattleAction::WALK_AND_ATTACK); //not all stacks can always walk, but we will check this elsewhere

		if(stack->canMove() && stack->getMovementRange(0)) //probably no reason to try move war machines or bound stacks
			allowedActionList.push_back(PossiblePlayerBattleAction::MOVE_STACK);

		const auto * siegedTown = battleGetDefendedTown();
		if(siegedTown && siegedTown->fortificationsLevel().wallsHealth > 0 && stack->hasBonusOfType(BonusType::CATAPULT)) //TODO: check shots
			allowedActionList.push_back(PossiblePlayerBattleAction::CATAPULT);
		if(stack->hasBonusOfType(BonusType::HEALER))
			allowedActionList.push_back(PossiblePlayerBattleAction::HEAL);
	}

	return allowedActionList;
}

PossiblePlayerBattleAction CBattleInfoCallback::getCasterAction(const CSpell * spell, const spells::Caster * caster, spells::Mode mode) const
{
	RETURN_IF_NOT_BATTLE(PossiblePlayerBattleAction::INVALID);
	auto spellSelMode = PossiblePlayerBattleAction::ANY_LOCATION;

	const CSpell::TargetInfo ti(spell, caster->getSpellSchoolLevel(spell), mode);

	if(ti.massive || ti.type == spells::AimType::NO_TARGET)
		spellSelMode = PossiblePlayerBattleAction::NO_LOCATION;
	else if(ti.type == spells::AimType::LOCATION && ti.clearAffected)
		spellSelMode = PossiblePlayerBattleAction::FREE_LOCATION;
	else if(ti.type == spells::AimType::CREATURE)
		spellSelMode = PossiblePlayerBattleAction::AIMED_SPELL_CREATURE;
	else if(ti.type == spells::AimType::OBSTACLE)
		spellSelMode = PossiblePlayerBattleAction::OBSTACLE;

	return PossiblePlayerBattleAction(spellSelMode, spell->id);
}

BattleHexArray CBattleInfoCallback::battleGetAttackedHexes(const battle::Unit * attacker, const BattleHex & destinationTile, const BattleHex & attackerPos) const
{
	BattleHexArray attackedHexes;
	RETURN_IF_NOT_BATTLE(attackedHexes);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (const BattleHex & tile : at.hostileCreaturePositions)
	{
		const auto * st = battleGetUnitByPos(tile, true);
		if(st && battleGetOwner(st) != battleGetOwner(attacker)) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	for (const BattleHex & tile : at.friendlyCreaturePositions)
	{
		if(battleGetUnitByPos(tile, true)) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

const CStack* CBattleInfoCallback::battleGetStackByPos(const BattleHex & pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	for(const auto * s : battleGetAllStacks(true))
		if(s->getHexes().contains(pos) && (!onlyAlive || s->alive()))
			return s;

	return nullptr;
}

const battle::Unit * CBattleInfoCallback::battleGetUnitByPos(const BattleHex & pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

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
	RETURN_IF_NOT_BATTLE();

	if(maxUnits == 0 && maxTurns == 0)
	{
		logGlobal->error("Attempt to get infinite battle queue");
		return;
	}

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

	boost::sort(phases[BattlePhases::SIEGE], CMP_stack(BattlePhases::SIEGE, actualTurn, sideThatLastMoved));
	std::copy(phases[BattlePhases::SIEGE].begin(), phases[BattlePhases::SIEGE].end(), std::back_inserter(turns.back()));

	if(turnsIsFull())
		return;

	for(uint8_t phase = BattlePhases::NORMAL; phase < BattlePhases::NUMBER_OF_PHASES; phase++)
		boost::sort(phases[phase], CMP_stack(phase, actualTurn, sideThatLastMoved));

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

	RETURN_IF_NOT_BATTLE(BattleHexArray());
	if(!unit->getPosition().isValid()) //turrets
		return BattleHexArray();

	auto reachability = getReachability(unit);

	return battleGetAvailableHexes(reachability, unit, obtainMovementRange);
}

BattleHexArray CBattleInfoCallback::battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit, bool obtainMovementRange) const
{
	BattleHexArray ret;

	RETURN_IF_NOT_BATTLE(ret);
	if(!unit->getPosition().isValid()) //turrets
		return ret;

	auto unitSpeed = unit->getMovementRange(0);

	const bool tacticsPhase = battleTacticDist() && battleGetTacticsSide() == unit->unitSide();

	for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		// If obstacles or other stacks makes movement impossible, it can't be helped.
		if(!cache.isReachable(i))
			continue;

		if(tacticsPhase && !obtainMovementRange) // if obtainMovementRange requested do not return tactics range
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

BattleHexArray CBattleInfoCallback::battleGetAvailableHexes(const battle::Unit * unit, bool obtainMovementRange, bool addOccupiable, BattleHexArray * attackable) const
{
	BattleHexArray ret = battleGetAvailableHexes(unit, obtainMovementRange);

	if(ret.empty())
		return ret;

	if(addOccupiable && unit->doubleWide())
	{
		BattleHexArray occupiable;

		for(const auto & hex : ret)
			occupiable.insert(unit->occupiedHex(hex));

		ret.insert(occupiable);
	}


	if(attackable)
	{
		auto meleeAttackable = [&](const BattleHex & hex) -> bool
		{
			// Return true if given hex has at least one available neighbour.
			// Available hexes are already present in ret vector.
			auto availableNeighbour = boost::find_if(ret, [=] (const BattleHex & availableHex)
			{
				return BattleHex::mutualPosition(hex, availableHex) >= 0;
			});
			return availableNeighbour != ret.end();
		};
		for(const auto * otherSt : battleAliveUnits(otherSide(unit->unitSide())))
		{
			if(!otherSt->isValidTarget(false))
				continue;

			const BattleHexArray & occupied = otherSt->getHexes();

			if(battleCanShoot(unit, otherSt->getPosition()))
			{
				attackable->insert(occupied);
				continue;
			}

			for(const BattleHex & he : occupied)
			{
				if(meleeAttackable(he))
					attackable->insert(he);
			}
		}
	}

	return ret;
}

bool CBattleInfoCallback::battleCanAttack(const battle::Unit * stack, const battle::Unit * target, const BattleHex & dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist())
		return false;

	if (!stack || !target)
		return false;

	if(target->isInvincible())
		return false;

	if(!battleMatchOwner(stack, target))
		return false;

	if (stack->getPosition() != dest)
	{
		for (const auto & obstacle : battleGetAllObstacles())
		{
			if (obstacle->getStoppingTile().contains(dest))
				return false;

			if (stack->doubleWide() && obstacle->getStoppingTile().contains(stack->occupiedHex(dest)))
				return false;
		}
	}

	auto id = stack->unitType()->getId();
	if (id == CreatureID::FIRST_AID_TENT || id == CreatureID::CATAPULT)
		return false;

	return target->alive();
}

bool CBattleInfoCallback::battleCanShoot(const battle::Unit * attacker) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist()) //no shooting during tactics
		return false;

	if (!attacker)
		return false;
	if (attacker->creatureIndex() == CreatureID::CATAPULT) //catapult cannot attack creatures
		return false;

	if (!attacker->canShoot())
		return false;

	return attacker->canShootBlocked() || !battleIsUnitBlocked(attacker);
}

bool CBattleInfoCallback::battleCanTargetEmptyHex(const battle::Unit * attacker) const
{
	RETURN_IF_NOT_BATTLE(false);

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

bool CBattleInfoCallback::battleCanShoot(const battle::Unit * attacker, const BattleHex & dest) const
{
	RETURN_IF_NOT_BATTLE(false);

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

	if(emptyHexAreaAttack || (battleMatchOwner(attacker, defender) && defender->alive()))
	{
		if(battleCanShoot(attacker))
		{
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
	DamageCalculator calculator(*this, info);

	return calculator.calculateDmgRange();
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, const BattleHex & attackerPosition, DamageEstimation * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE({});
	auto reachability = battleGetDistances(attacker, attacker->getPosition());
	int movementRange = attackerPosition.isValid() ? reachability[attackerPosition.toInt()] : 0;
	return battleEstimateDamage(attacker, defender, movementRange, retaliationDmg);
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const battle::Unit * attacker, const battle::Unit * defender, int movementRange, DamageEstimation * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE({});
	const bool shooting = battleCanShoot(attacker, defender->getPosition());
	const BattleAttackInfo bai(attacker, defender, movementRange, shooting);
	return battleEstimateDamage(bai, retaliationDmg);
}

DamageEstimation CBattleInfoCallback::battleEstimateDamage(const BattleAttackInfo & bai, DamageEstimation * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE({});

	DamageEstimation ret = calculateDmgRange(bai);

	if(retaliationDmg == nullptr)
		return ret;

	*retaliationDmg = DamageEstimation();

	if(bai.shooting) //FIXME: handle RANGED_RETALIATION
		return ret;

	if (!bai.defender->ableToRetaliate())
		return ret;

	if (bai.attacker->hasBonusOfType(BonusType::BLOCKS_RETALIATION) || bai.attacker->isInvincible())
		return ret;

	//TODO: rewrite using boost::numeric::interval
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

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::battleGetAllObstaclesOnPos(const BattleHex & tile, bool onlyBlocking) const
{
	auto obstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	RETURN_IF_NOT_BATTLE(obstacles);
	for(auto & obs : battleGetAllObstacles())
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
	RETURN_IF_NOT_BATTLE(affectedObstacles);
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
				bocp.changes.emplace_back(spellObstacle.uniqueID, operation);
				changedObstacle.toInfo(bocp.changes.back(), operation);
				spellEnv.apply(bocp);
			};
			const auto side = unit.unitSide();
			auto shouldReveal = !spellObstacle->hidden || !battleIsObstacleVisibleForSide(*obstacle, side);
			const auto * hero = battleGetFightingHero(spellObstacle->casterSide);
			auto caster = spells::ObstacleCasterProxy(getBattle()->getSidePlayer(spellObstacle->casterSide), hero, *spellObstacle);

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

		if(obstacle->stopsMovement())
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
		accessibleCache[hex] = accessibility.accessible(hex, params.doubleWide, params.side);

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
				if(battleGetGateState() != EGateState::DESTROYED && params.side == BattleSide::ATTACKER)
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
	RETURN_IF_NOT_BATTLE(ret);

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

std::pair<const battle::Unit *, BattleHex> CBattleInfoCallback::getNearestStack(const battle::Unit * closest) const
{
	auto reachability = getReachability(closest);
	auto avHexes = battleGetAvailableHexes(reachability, closest, false);

	// I hate std::pairs with their undescriptive member names first / second
	struct DistStack
	{
		uint32_t distanceToPred;
		BattleHex destination;
		const battle::Unit * stack;
	};

	std::vector<DistStack> stackPairs;

	battle::Units possible = battleGetUnitsIf([closest](const battle::Unit * unit)
	{
		return unit->isValidTarget(false) && unit != closest;
	});

	for(const battle::Unit * st : possible)
	{
		for(const BattleHex & hex : avHexes)
			if(CStack::isMeleeAttackPossible(closest, st, hex))
			{
				DistStack hlp = {reachability.distances[hex.toInt()], hex, st};
				stackPairs.push_back(hlp);
			}
	}

	if(!stackPairs.empty())
	{
		auto comparator = [](DistStack lhs, DistStack rhs) { return lhs.distanceToPred < rhs.distanceToPred; };
		auto minimal = boost::min_element(stackPairs, comparator);
		return std::make_pair(minimal->stack, minimal->destination);
	}
	else
		return std::make_pair<const battle::Unit * , BattleHex>(nullptr, BattleHex::INVALID);
}

BattleHex CBattleInfoCallback::getAvailableHex(const CreatureID & creID, BattleSide side, int initialPos) const
{
	bool twoHex = LIBRARY->creatures()->getById(creID)->isDoubleWide();

	int pos;
	if (initialPos > -1)
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
		if(accessibility.accessible(i, twoHex, side))
			occupyable.insert(i);

	if(occupyable.empty())
	{
		return BattleHex::INVALID; //all tiles are covered
	}

	return BattleHex::getClosestTile(side, pos, occupyable);
}

si8 CBattleInfoCallback::battleGetTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);

	//TODO get rid of this method
	if(battleDoWeKnowAbout(battleGetTacticsSide()))
		return battleTacticDist();

	return 0;
}

bool CBattleInfoCallback::isInTacticRange(const BattleHex & dest) const
{
	RETURN_IF_NOT_BATTLE(false);
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
	ReachabilityInfo::Parameters params(unit, unit->getPosition());

	if(!battleDoWeKnowAbout(unit->unitSide()))
	{
		//Stack is held by enemy, we can't use his perspective to check for reachability.
		// Happens ie. when hovering enemy stack for its range. The arg could be set properly, but it's easier to fix it here.
		params.perspective = battleGetMySide();
	}

	return getReachability(params);
}

ReachabilityInfo CBattleInfoCallback::getReachability(const ReachabilityInfo::Parameters & params) const
{
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
		if(ret.accessibility.accessible(i, params.doubleWide, params.side))
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
	RETURN_IF_NOT_BATTLE(at);

	BattleHex attackOriginHex = (attackerPos.toInt() != BattleHex::INVALID) ? attackerPos : attacker->getPosition(); //real or hypothetical (cursor) position
	
	defenderPos = (defenderPos.toInt() != BattleHex::INVALID) ? defenderPos : defender->getPosition(); //real or hypothetical (cursor) position
	
	if(attacker->hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT))
		at.hostileCreaturePositions.insert(attacker->getSurroundingHexes(attackerPos));

	// If attacker is double-wide and its head is not adjacent to enemy we need to turn around
	if(attacker->doubleWide() && !vstd::contains(defender->getSurroundingHexes(defenderPos), attackOriginHex))
		attackOriginHex = attacker->occupiedHex(attackOriginHex);

	if (!vstd::contains(defender->getSurroundingHexes(defenderPos), attackOriginHex))
		throw std::runtime_error("Atempt to attack from invalid position!");

	auto attackDirection = BattleHex::mutualPosition(attackOriginHex, defenderPos);

	// If defender is double-wide, attacker always prefers targeting its 'tail', if it is reachable
	if(defender->doubleWide() && BattleHex::mutualPosition(attackOriginHex, defender->occupiedHex(defenderPos)) != BattleHex::NONE)
		attackDirection = BattleHex::mutualPosition(attackOriginHex, defender->occupiedHex(defenderPos));

	if (attackDirection == BattleHex::NONE)
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
		at.friendlyCreaturePositions.insert(processTargets(bonus->additionalInfo));

	for (const auto & bonus : *multihexEnemy)
		at.hostileCreaturePositions.insert(processTargets(bonus->additionalInfo));

	for (const auto & bonus : *multihexAnimation)
		at.overrideAnimationPositions.insert(processTargets(bonus->additionalInfo));

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
	RETURN_IF_NOT_BATTLE(at);

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
	RETURN_IF_NOT_BATTLE(units);

	if(attackerPos == BattleHex::INVALID)
		attackerPos = attacker->getPosition();

	if(defenderPos == BattleHex::INVALID)
		defenderPos = defender->getPosition();

	AttackableTiles at;

	if (rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
		at = getPotentiallyAttackableHexes(attacker, defender, destinationTile, attackerPos, defenderPos);

	units = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		if (unit->isGhost() || !unit->alive())
			return false;

		for (const BattleHex & hex : unit->getHexes())
		{
			if (at.hostileCreaturePositions.contains(hex))
				return true;
			if (at.friendlyCreaturePositions.contains(hex))
				return true;
		}
		return false;
	});

	return units;
}

std::pair<std::set<const CStack*>, bool> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, const BattleHex & destinationTile, bool rangedAttack, BattleHex attackerPos) const
{
	std::pair<std::set<const CStack*>, bool> attackedCres;
	RETURN_IF_NOT_BATTLE(attackedCres);

	AttackableTiles at;
	
	if(rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
		at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (const BattleHex & tile : at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && battleGetOwner(st) != battleGetOwner(attacker)) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.first.insert(st);
		}
	}
	for (const BattleHex & tile : at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.first.insert(st);
		}
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
	RETURN_IF_NOT_BATTLE(ret);

	auto reachability = getReachability(unit);

	boost::copy(reachability.distances, ret.begin());

	return ret;
}

bool CBattleInfoCallback::battleHasDistancePenalty(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHex & destHex) const
{
	RETURN_IF_NOT_BATTLE(false);

	const std::string cachingStrNoDistancePenalty = "type_NO_DISTANCE_PENALTY";
	static const auto selectorNoDistancePenalty = Selector::type()(BonusType::NO_DISTANCE_PENALTY);

	if(shooter->hasBonus(selectorNoDistancePenalty, cachingStrNoDistancePenalty))
		return false;

	if(const auto * target = battleGetUnitByPos(destHex, true))
	{
		//If any hex of target creature is within range, there is no penalty
		int range = GameConstants::BATTLE_SHOOTING_PENALTY_DISTANCE;

		auto bonus = shooter->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
		if(bonus != nullptr && bonus->additionalInfo != CAddInfo::NONE)
			range = bonus->additionalInfo[0];

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
	RETURN_IF_NOT_BATTLE(BattleHex::INVALID);
	return WallPartToHex(part);
}

EWallPart CBattleInfoCallback::battleHexToWallPart(const BattleHex & hex) const
{
	RETURN_IF_NOT_BATTLE(EWallPart::INVALID);
	return hexToWallPart(hex);
}

bool CBattleInfoCallback::isWallPartPotentiallyAttackable(EWallPart wallPart) const
{
	RETURN_IF_NOT_BATTLE(false);
	return wallPart != EWallPart::INDESTRUCTIBLE_PART && wallPart != EWallPart::INDESTRUCTIBLE_PART_OF_GATE &&
																	wallPart != EWallPart::INVALID;
}

bool CBattleInfoCallback::isWallPartAttackable(EWallPart wallPart) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(isWallPartPotentiallyAttackable(wallPart))
	{
		auto wallState = battleGetWallState(wallPart);
		return (wallState != EWallState::NONE && wallState != EWallState::DESTROYED);
	}
	return false;
}

BattleHexArray CBattleInfoCallback::getAttackableBattleHexes() const
{
	BattleHexArray attackableBattleHexes;
	RETURN_IF_NOT_BATTLE(attackableBattleHexes);

	for(const auto & wallPartPair : wallParts)
	{
		if(isWallPartAttackable(wallPartPair.second))
			attackableBattleHexes.insert(wallPartPair.first);
	}

	return attackableBattleHexes;
}

int32_t CBattleInfoCallback::battleGetSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const
{
	RETURN_IF_NOT_BATTLE(-1);
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
	RETURN_IF_NOT_BATTLE(false);

	for(const auto * adjacent : battleAdjacentUnits(unit))
	{
		if(adjacent->unitOwner() != unit->unitOwner()) //blocked by enemy stack
			return true;
	}
	return false;
}

battle::Units CBattleInfoCallback::battleAdjacentUnits(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE({});

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

SpellID CBattleInfoCallback::getRandomBeneficialSpell(vstd::RNG & rand, const battle::Unit * caster, const battle::Unit * subject) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);
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
			return pred(stack) && stack->unitOwner() != subject->unitOwner() && stack->isValidTarget(false);
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
		spells::detail::ProblemImpl problem;

		if (!m->canBeCastAt(target, problem))
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

	if(!beneficialSpells.empty())
	{
		return *RandomGeneratorUtil::nextItem(beneficialSpells, rand);
	}
	else
	{
		return SpellID::NONE;
	}
}

SpellID CBattleInfoCallback::getRandomCastedSpell(vstd::RNG & rand,const CStack * caster) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);

	TConstBonusListPtr bl = caster->getBonusesOfType(BonusType::SPELLCASTER);
	if (!bl->size())
		return SpellID::NONE;

	if(bl->size() == 1 && bl->front()->additionalInfo[0] > 0) // there is one random spell -> select it
		return bl->front()->subtype.as<SpellID>();

	int totalWeight = 0;
	for(const auto & b : *bl)
	{
		totalWeight += std::max(b->additionalInfo[0], 0); //spells with 0 weight are non-random, exclude them
	}

	if (totalWeight == 0)
		return SpellID::NONE;

	int randomPos = rand.nextInt(totalWeight - 1);
	for(const auto & b : *bl)
	{
		randomPos -= std::max(b->additionalInfo[0], 0);
		if(randomPos < 0)
		{
			return b->subtype.as<SpellID>();
		}
	}

	return SpellID::NONE;
}

int CBattleInfoCallback::battleGetSurrenderCost(const PlayerColor & Player) const
{
	RETURN_IF_NOT_BATTLE(-3);
	if(!battleCanSurrender(Player))
		return -1;

	const BattleSide side = playerToSide(Player);
	if(side == BattleSide::NONE)
		return -1;

	int ret = 0;
	double discount = 0;

	for(const auto * unit : battleAliveUnits(side))
		ret += unit->getRawSurrenderCost();

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

VCMI_LIB_NAMESPACE_END
