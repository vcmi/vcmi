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

#include "../CStack.h"
#include "BattleInfo.h"
#include "../NetPacks.h"
#include "../spells/CSpellHandler.h"
#include "../mapObjects/CGTownInstance.h"
#include "../BattleFieldHandler.h"
#include "../CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace SiegeStuffThatShouldBeMovedToHandlers // <=== TODO
{
/*
 *Here are 2 explanations how below algorithm should work in H3, looks like they are not 100% accurate as it results in one damage number, not min/max range:
 *
 *1. http://heroes.thelazy.net/wiki/Arrow_tower
 *
 *2. All towns' turrets do the same damage. If Fort, Citadel or Castle is built damage of the Middle turret is 15, and 7,5 for others.
 *Buildings increase turrets' damage, but only those buildings that are new in town view, not upgrades to the existing. So, every building save:
 *- dwellings' upgrades
 *- Mage Guild upgrades
 *- Horde buildings
 *- income upgrades
 *- some special ones
 *increases middle Turret damage by 3, and 1,5 for the other two.
 *Damage is almost always the maximum one (right click on the Turret), sometimes +1/2 points, and it does not depend on the target. Nothing can influence it, except the mentioned above (but it will be roughly double if the defender has Armorer or Air Shield).
 *Maximum damage for Castle, Conflux is 120, Necropolis, Inferno, Fortress 125, Stronghold, Turret, and Dungeon 130 (for all three Turrets).
 *Artillery allows the player to control the Turrets.
 */
static void retrieveTurretDamageRange(const CGTownInstance * town, const battle::Unit * turret, double & outMinDmg, double & outMaxDmg)//does not match OH3 yet, but damage is somewhat close
{
	assert(turret->creatureIndex() == CreatureID::ARROW_TOWERS);
	assert(town);
	assert(turret->getPosition() >= -4 && turret->getPosition() <= -2);

	const float multiplier = (turret->getPosition() == -2) ? 1.0f : 0.5f;

	//Revised - Where do below values come from?
	/*int baseMin = 6;
	int baseMax = 10;*/

	const int baseDamage = 15;

	outMinDmg = multiplier * (baseDamage + town->getTownLevel() * 3);
	outMaxDmg = outMinDmg;
}

static BattleHex lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
{
	static const BattleHex lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

	return lineToHex[line];
}

static bool sameSideOfWall(BattleHex pos1, BattleHex pos2)
{
	const int wallInStackLine = lineToWallHex(pos1.getY());
	const int wallInDestLine = lineToWallHex(pos2.getY());

	const bool stackLeft = pos1 < wallInStackLine;
	const bool destLeft = pos2 < wallInDestLine;

	return stackLeft == destLeft;
}

// parts of wall
static const std::pair<int, EWallPart::EWallPart> wallParts[] =
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

static EWallPart::EWallPart hexToWallPart(BattleHex hex)
{
	for(auto & elem : wallParts)
	{
		if(elem.first == hex)
			return elem.second;
	}

	return EWallPart::INVALID; //not found!
}

static BattleHex WallPartToHex(EWallPart::EWallPart part)
{
	for(auto & elem : wallParts)
	{
		if(elem.second == part)
			return elem.first;
	}

	return BattleHex::INVALID; //not found!
}
}

using namespace SiegeStuffThatShouldBeMovedToHandlers;

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastSpell(const spells::Caster * caster, spells::Mode mode) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	if(caster == nullptr)
	{
		logGlobal->error("CBattleInfoCallback::battleCanCastSpell: no spellcaster.");
		return ESpellCastProblem::INVALID;
	}
	const PlayerColor player = caster->getCasterOwner();
	const auto side = playerToSide(player);
	if(!side)
		return ESpellCastProblem::INVALID;
	if(!battleDoWeKnowAbout(side.get()))
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
		if(battleCastSpells(side.get()) > 0)
			return ESpellCastProblem::CASTS_PER_TURN_LIMIT;

		auto hero = dynamic_cast<const CGHeroInstance *>(caster);

		if(!hero)
			return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
		if(hero->hasBonusOfType(Bonus::BLOCK_ALL_MAGIC))
			return ESpellCastProblem::MAGIC_IS_BLOCKED;
	}
		break;
	default:
		break;
	}

	return ESpellCastProblem::OK;
}

bool CBattleInfoCallback::battleHasWallPenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(!battleGetSiegeLevel())
		return false;

	const std::string cachingStrNoWallPenalty = "type_NO_WALL_PENALTY";
	static const auto selectorNoWallPenalty = Selector::type()(Bonus::NO_WALL_PENALTY);

	if(shooter->hasBonus(selectorNoWallPenalty, cachingStrNoWallPenalty))
		return false;

	const int wallInStackLine = lineToWallHex(shooterPosition.getY());
	const int wallInDestLine = lineToWallHex(destHex.getY());

	const bool stackLeft = shooterPosition < wallInStackLine;
	const bool destRight = destHex > wallInDestLine;

	if (stackLeft && destRight) //shooting from outside to inside
	{
		int row = (shooterPosition + destHex) / (2 * GameConstants::BFIELD_WIDTH);
		if (shooterPosition > destHex && ((destHex % GameConstants::BFIELD_WIDTH - shooterPosition % GameConstants::BFIELD_WIDTH) < 2)) //shooting up high
			row -= 2;
		const int wallPos = lineToWallHex(row);
		if (!isWallPartPotentiallyAttackable(battleHexToWallPart(wallPos))) return true;
	}

	return false;
}

si8 CBattleInfoCallback::battleCanTeleportTo(const battle::Unit * stack, BattleHex destHex, int telportLevel) const
{
	RETURN_IF_NOT_BATTLE(false);
	if (!getAccesibility(stack).accessible(destHex, stack))
		return false;

	const ui8 siegeLevel = battleGetSiegeLevel();

	//check for wall
	//advanced teleport can pass wall of fort|citadel, expert - of castle
	if ((siegeLevel > CGTownInstance::NONE && telportLevel < 2) || (siegeLevel >= CGTownInstance::CASTLE && telportLevel < 3))
		return sameSideOfWall(stack->getPosition(), destHex);

	return true;
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
			if(stack->hasBonusOfType(Bonus::SPELLCASTER) && data.creatureSpellToCast != -1)
			{
				const CSpell *spell = SpellID(data.creatureSpellToCast).toSpell();
				PossiblePlayerBattleAction act = getCasterAction(spell, stack, spells::Mode::CREATURE_ACTIVE);
				allowedActionList.push_back(act);
			}
			if(stack->hasBonusOfType(Bonus::RANDOM_SPELLCASTER))
				allowedActionList.push_back(PossiblePlayerBattleAction::RANDOM_GENIE_SPELL);
		}
		if(stack->canShoot())
			allowedActionList.push_back(PossiblePlayerBattleAction::SHOOT);
		if(stack->hasBonusOfType(Bonus::RETURN_AFTER_STRIKE))
			allowedActionList.push_back(PossiblePlayerBattleAction::ATTACK_AND_RETURN);

		allowedActionList.push_back(PossiblePlayerBattleAction::ATTACK); //all active stacks can attack
		allowedActionList.push_back(PossiblePlayerBattleAction::WALK_AND_ATTACK); //not all stacks can always walk, but we will check this elsewhere

		if(stack->canMove() && stack->Speed(0, true)) //probably no reason to try move war machines or bound stacks
			allowedActionList.push_back(PossiblePlayerBattleAction::MOVE_STACK);

		auto siegedTown = battleGetDefendedTown();
		if(siegedTown && siegedTown->hasFort() && stack->hasBonusOfType(Bonus::CATAPULT)) //TODO: check shots
			allowedActionList.push_back(PossiblePlayerBattleAction::CATAPULT);
		if(stack->hasBonusOfType(Bonus::HEALER))
			allowedActionList.push_back(PossiblePlayerBattleAction::HEAL);
	}

	return allowedActionList;
}

PossiblePlayerBattleAction CBattleInfoCallback::getCasterAction(const CSpell * spell, const spells::Caster * caster, spells::Mode mode) const
{
	RETURN_IF_NOT_BATTLE(PossiblePlayerBattleAction::INVALID);
	PossiblePlayerBattleAction spellSelMode = PossiblePlayerBattleAction::ANY_LOCATION;

	const CSpell::TargetInfo ti(spell, caster->getSpellSchoolLevel(spell), mode);

	if(ti.massive || ti.type == spells::AimType::NO_TARGET)
		spellSelMode = PossiblePlayerBattleAction::NO_LOCATION;
	else if(ti.type == spells::AimType::LOCATION && ti.clearAffected)
		spellSelMode = PossiblePlayerBattleAction::FREE_LOCATION;
	else if(ti.type == spells::AimType::CREATURE)
		spellSelMode = PossiblePlayerBattleAction::AIMED_SPELL_CREATURE;
	else if(ti.type == spells::AimType::OBSTACLE)
		spellSelMode = PossiblePlayerBattleAction::OBSTACLE;

	return spellSelMode;
}

std::set<BattleHex> CBattleInfoCallback::battleGetAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	std::set<BattleHex> attackedHexes;
	RETURN_IF_NOT_BATTLE(attackedHexes);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (BattleHex tile : at.hostileCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		if(battleGetStackByPos(tile, true)) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

SpellID CBattleInfoCallback::battleGetRandomStackSpell(CRandomGenerator & rand, const CStack * stack, ERandomSpell mode) const
{
	switch (mode)
	{
	case RANDOM_GENIE:
		return getRandomBeneficialSpell(rand, stack); //target
		break;
	case RANDOM_AIMED:
		return getRandomCastedSpell(rand, stack); //caster
		break;
	default:
		logGlobal->error("Incorrect mode of battleGetRandomSpell (%d)", static_cast<int>(mode));
		return SpellID::NONE;
	}
}

const CStack* CBattleInfoCallback::battleGetStackByPos(BattleHex pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	for(auto s : battleGetAllStacks(true))
		if(vstd::contains(s->getHexes(), pos) && (!onlyAlive || s->alive()))
			return s;

	return nullptr;
}

const battle::Unit * CBattleInfoCallback::battleGetUnitByPos(BattleHex pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	auto ret = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return !unit->isGhost()
			&& vstd::contains(battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()), pos)
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

battle::Units CBattleInfoCallback::battleAliveUnits(ui8 side) const
{
	return battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget(false) && unit->unitSide() == side;
	});
}

//T is battle::Unit descendant
template <typename T>
const T * takeOneUnit(std::vector<const T*> & all, const int turn, int8_t & lastMoved, int phase)
{
	const T * returnedUnit = nullptr;
	size_t currentUnitIndex = 0;

	for(size_t i = 0; i < all.size(); i++)
	{
		int32_t currentUnitSpeed = -1;
		int32_t returnedUnitSpeed = -1;
		if(returnedUnit)
			returnedUnitSpeed = returnedUnit->getInitiative(turn);
		if(all[i])
		{
			currentUnitSpeed = all[i]->getInitiative(turn);
			switch(phase)
			{
			case 1: // Faster first, attacker priority, higher slot first
				if(returnedUnit == nullptr || currentUnitSpeed > returnedUnitSpeed)
				{
					returnedUnit = all[i];
					currentUnitIndex = i;
				}
				else if(currentUnitSpeed == returnedUnitSpeed)
				{
					if(lastMoved == -1 && turn <= 0 && all[i]->unitSide() == BattleSide::ATTACKER
						&& !(returnedUnit->unitSide() == all[i]->unitSide() && returnedUnit->unitSlot() < all[i]->unitSlot())) // Turn 0 attacker priority
					{
						returnedUnit = all[i];
						currentUnitIndex = i;
					}
					else if(lastMoved != -1 && all[i]->unitSide() != lastMoved
						&& !(returnedUnit->unitSide() == all[i]->unitSide() && returnedUnit->unitSlot() < all[i]->unitSlot())) // Alternate equal speeds units
					{
						returnedUnit = all[i];
						currentUnitIndex = i;
					}
				}
				break;
			case 2: // Slower first, higher slot first
			case 3:
				if(returnedUnit == nullptr || currentUnitSpeed < returnedUnitSpeed)
				{
					returnedUnit = all[i];
					currentUnitIndex = i;
				}
				else if(currentUnitSpeed == returnedUnitSpeed && lastMoved != -1 && all[i]->unitSide() != lastMoved
					&& !(returnedUnit->unitSide() == all[i]->unitSide() && returnedUnit->unitSlot() < all[i]->unitSlot())) // Alternate equal speeds units
				{
					returnedUnit = all[i];
					currentUnitIndex = i;
				}
				break;
			default:
				break;
			}
		}
	}

	if(!returnedUnit)
		return nullptr;
	all[currentUnitIndex] = nullptr;
	return returnedUnit;
}

void CBattleInfoCallback::battleGetTurnOrder(std::vector<battle::Units> & out, const size_t maxUnits, const int maxTurns, const int turn, int8_t lastMoved) const
{
	RETURN_IF_NOT_BATTLE();

	if(maxUnits == 0 && maxTurns == 0)
	{
		logGlobal->error("Attempt to get infinite battle queue");
		return;
	}

	auto actualTurn = turn > 0 ? turn : 0;

	auto outputFull = [&]() -> bool
	{
		if(maxUnits == 0)
			return false;//no limit

		size_t outSize = 0;
		for(const auto & oneTurn : out)
			outSize += oneTurn.size();
		return outSize >= maxUnits;
	};

	out.emplace_back();

	//We'll split creatures with remaining movement to 4 buckets
	// [0] - turrets/catapult,
	// [1] - normal (unmoved) creatures, other war machines,
	// [2] - waited cres that had morale,
	// [3] - rest of waited cres
	std::array<battle::Units, 4> phase;

	const battle::Unit * active = battleActiveUnit();

	if(active)
	{
		//its first turn and active unit hasn't taken any action yet - must be placed at the beginning of queue, no matter what
		if(turn == 0 && active->willMove() && !active->waited())
		{
			out.back().push_back(active);
			if(outputFull())
				return;
		}

		//its first or current turn, turn priority for active stack side
		//TODO: what if active stack mind-controlled?
		if(turn <= 0 && lastMoved < 0)
			lastMoved = active->unitSide();
	}

	auto all = battleGetUnitsIf([](const battle::Unit * unit)
	{
		return !unit->isGhost();
	});


	if(!vstd::contains_if(all, [](const battle::Unit * unit) { return unit->willMove(100000); })) //little evil, but 100000 should be enough for all effects to disappear
	{
		//No unit will be able to move, battle is over.
		out.clear();
		return;
	}

	for(auto one : all)
	{
		if((actualTurn == 0 && !one->willMove()) //we are considering current round and unit won't move
		|| (actualTurn > 0 && !one->canMove(turn)) //unit won't be able to move in later rounds
		|| (actualTurn == 0 && one == active && !out.at(0).empty() && one == out.front().front())) //it's active unit already added at the beginning of queue
		{
			continue;
		}

		int p = one->battleQueuePhase(turn);

		phase[p].push_back(one);
	}

	boost::sort(phase[0], CMP_stack(0, actualTurn, lastMoved));
	std::copy(phase[0].begin(), phase[0].end(), std::back_inserter(out.back()));

	if(outputFull())
		return;

	for(int i = 1; i < 4; i++)
		boost::sort(phase[i], CMP_stack(i, actualTurn, lastMoved));

	int pi = 1;
	while(!outputFull() && pi < 4)
	{
		const battle::Unit * current = nullptr;
		if(phase[pi].empty())
			pi++;
		else
		{
			current = takeOneUnit(phase[pi], actualTurn, lastMoved, pi);
			if(!current)
			{
				pi++;
			}
			else
			{
				out.back().push_back(current);
				lastMoved = current->unitSide();
			}
		}
	}

	if(lastMoved < 0)
		lastMoved = BattleSide::ATTACKER;

	if(!outputFull() && (maxTurns == 0 || out.size() < maxTurns))
		battleGetTurnOrder(out, maxUnits, maxTurns, actualTurn + 1, lastMoved);
}

std::vector<BattleHex> CBattleInfoCallback::battleGetAvailableHexes(const battle::Unit * unit) const
{

	RETURN_IF_NOT_BATTLE(std::vector<BattleHex>());
	if(!unit->getPosition().isValid()) //turrets
		return std::vector<BattleHex>();

	auto reachability = getReachability(unit);

	return battleGetAvailableHexes(reachability, unit);
}

std::vector<BattleHex> CBattleInfoCallback::battleGetAvailableHexes(const ReachabilityInfo & cache, const battle::Unit * unit) const
{
	std::vector<BattleHex> ret;

	RETURN_IF_NOT_BATTLE(ret);
	if(!unit->getPosition().isValid()) //turrets
		return ret;

	auto unitSpeed = unit->Speed(0, true);

	const bool tacticPhase = battleTacticDist() && battleGetTacticsSide() == unit->unitSide();

	for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		// If obstacles or other stacks makes movement impossible, it can't be helped.
		if(!cache.isReachable(i))
			continue;

		if(tacticPhase)
		{
			//Stack has to perform tactic-phase movement -> can enter any reachable tile within given range
			if(!isInTacticRange(i))
				continue;
		}
		else
		{
			//Not tactics phase -> destination must be reachable and within unit range.
			if(cache.distances[i] > (int)unitSpeed)
				continue;
		}

		ret.push_back(i);
	}

	return ret;
}


std::vector<BattleHex> CBattleInfoCallback::battleGetAvailableHexes(const battle::Unit * unit, bool addOccupiable, std::vector<BattleHex> * attackable) const
{
	std::vector<BattleHex> ret = battleGetAvailableHexes(unit);

	if(ret.empty())
		return ret;

	if(addOccupiable && unit->doubleWide())
	{
		std::vector<BattleHex> occupiable;

		for(auto hex : ret)
			occupiable.push_back(unit->occupiedHex(hex));

		vstd::concatenate(ret, occupiable);
	}


	if(attackable)
	{
		auto meleeAttackable = [&](BattleHex hex) -> bool
		{
			// Return true if given hex has at least one available neighbour.
			// Available hexes are already present in ret vector.
			auto availableNeighbor = boost::find_if(ret, [=] (BattleHex availableHex)
			{
				return BattleHex::mutualPosition(hex, availableHex) >= 0;
			});
			return availableNeighbor != ret.end();
		};
		for(auto otherSt : battleAliveUnits(otherSide(unit->unitSide())))
		{
			if(!otherSt->isValidTarget(false))
				continue;

			std::vector<BattleHex> occupied = otherSt->getHexes();

			if(battleCanShoot(unit, otherSt->getPosition()))
			{
				attackable->insert(attackable->end(), occupied.begin(), occupied.end());
				continue;
			}

			for(BattleHex he : occupied)
			{
				if(meleeAttackable(he))
					attackable->push_back(he);
			}
		}
	}

	//adding occupiable likely adds duplicates to ret -> clean it up
	boost::sort(ret);
	ret.erase(boost::unique(ret).end(), ret.end());
	return ret;
}

bool CBattleInfoCallback::battleCanAttack(const CStack * stack, const CStack * target, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist())
		return false;

	if (!stack || !target)
		return false;

	if(!battleMatchOwner(stack, target))
		return false;

	auto &id = stack->getCreature()->idNumber;
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

	//forgetfulness
	TConstBonusListPtr forgetfulList = attacker->getBonuses(Selector::type()(Bonus::FORGETFULL));
	if(!forgetfulList->empty())
	{
		int forgetful = forgetfulList->valOfBonuses(Selector::type()(Bonus::FORGETFULL));

		//advanced+ level
		if(forgetful > 1)
			return false;
	}

	return attacker->canShoot()	&& (!battleIsUnitBlocked(attacker)
			|| attacker->hasBonusOfType(Bonus::FREE_SHOOTING));
}

bool CBattleInfoCallback::battleCanShoot(const battle::Unit * attacker, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	const battle::Unit * defender = battleGetUnitByPos(dest);
	if(!attacker || !defender)
		return false;

	if(battleMatchOwner(attacker, defender) && defender->alive())
	{
		if(battleCanShoot(attacker))
		{
			auto shooterBonus = attacker->getBonus(Selector::type()(Bonus::SHOOTER));
			if(shooterBonus->additionalInfo == CAddInfo::NONE)
			{
				return true;
			}

			int shootingRange = shooterBonus->additionalInfo[0];
			int distanceBetweenHexes = BattleHex::getDistance(attacker->getPosition(), dest);

			if(distanceBetweenHexes <= shootingRange)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

TDmgRange CBattleInfoCallback::calculateDmgRange(const BattleAttackInfo & info) const
{
	auto battleBonusValue = [&](const IBonusBearer * bearer, CSelector selector) -> int
	{
		auto noLimit = Selector::effectRange()(Bonus::NO_LIMIT);
		auto limitMatches = info.shooting
							? Selector::effectRange()(Bonus::ONLY_DISTANCE_FIGHT)
							: Selector::effectRange()(Bonus::ONLY_MELEE_FIGHT);

		//any regular bonuses or just ones for melee/ranged
		return bearer->getBonuses(selector, noLimit.Or(limitMatches))->totalValue();
	};

	const IBonusBearer * attackerBonuses = info.attacker;
	const IBonusBearer * defenderBonuses = info.defender;

	double additiveBonus = 1.0 + info.additiveBonus;
	double multBonus = 1.0 * info.multBonus;
	double minDmg = 0.0;
	double maxDmg = 0.0;

	minDmg = info.attacker->getMinDamage(info.shooting);
	maxDmg = info.attacker->getMaxDamage(info.shooting);

	minDmg *= info.attacker->getCount(),
	maxDmg *= info.attacker->getCount();

	if(info.attacker->creatureIndex() == CreatureID::ARROW_TOWERS)
	{
		SiegeStuffThatShouldBeMovedToHandlers::retrieveTurretDamageRange(battleGetDefendedTown(), info.attacker, minDmg, maxDmg);
		TDmgRange unmodifiableTowerDamage = std::make_pair(int64_t(minDmg), int64_t(maxDmg));
		return unmodifiableTowerDamage;
	}

	const std::string cachingStrSiedgeWeapon = "type_SIEGE_WEAPON";
	static const auto selectorSiedgeWeapon = Selector::type()(Bonus::SIEGE_WEAPON);

	if(attackerBonuses->hasBonus(selectorSiedgeWeapon, cachingStrSiedgeWeapon) && info.attacker->creatureIndex() != CreatureID::ARROW_TOWERS) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		auto retrieveHeroPrimSkill = [&](int skill) -> int
		{
			std::shared_ptr<const Bonus> b = attackerBonuses->getBonus(Selector::sourceTypeSel(Bonus::HERO_BASE_SKILL).And(Selector::typeSubtype(Bonus::PRIMARY_SKILL, skill)));
			return b ? b->val : 0; //if there is no hero or no info on his primary skill, return 0
		};


		minDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
	}

	double attackDefenceDifference = 0.0;

	double multAttackReduction = 1.0 - battleBonusValue(attackerBonuses, Selector::type()(Bonus::GENERAL_ATTACK_REDUCTION)) / 100.0;
	attackDefenceDifference += info.attacker->getAttack(info.shooting) * multAttackReduction;

	double multDefenceReduction = 1.0 - battleBonusValue(attackerBonuses, Selector::type()(Bonus::ENEMY_DEFENCE_REDUCTION)) / 100.0;
	attackDefenceDifference -= info.defender->getDefense(info.shooting) * multDefenceReduction;

	const std::string cachingStrSlayer = "type_SLAYER";
	static const auto selectorSlayer = Selector::type()(Bonus::SLAYER);

	//slayer handling //TODO: apply only ONLY_MELEE_FIGHT / DISTANCE_FIGHT?
	auto slayerEffects = attackerBonuses->getBonuses(selectorSlayer, cachingStrSlayer);

	if(std::shared_ptr<const Bonus> slayerEffect = slayerEffects->getFirst(Selector::all))
	{
		std::vector<int32_t> affectedIds;
		const auto spLevel = slayerEffect->val;
		const CCreature * defenderType = info.defender->unitType();
		bool isAffected = false;

		for(const auto & b : defenderType->getBonusList())
		{
			if((b->type == Bonus::KING3 && spLevel >= 3) || //expert
				(b->type == Bonus::KING2 && spLevel >= 2) || //adv +
				(b->type == Bonus::KING1 && spLevel >= 0)) //none or basic +
			{
				isAffected = true;
				break;
			}
		}

		if(isAffected)
		{
			attackDefenceDifference += SpellID(SpellID::SLAYER).toSpell()->getLevelPower(spLevel);
			if(info.attacker->hasBonusOfType(Bonus::SPECIAL_PECULIAR_ENCHANT, SpellID::SLAYER))
			{
				ui8 attackerTier = info.attacker->unitType()->level;
				ui8 specialtyBonus = std::max(5 - attackerTier, 0);
				attackDefenceDifference += specialtyBonus;
	}
		}
	}

	//bonus from attack/defense skills
	if(attackDefenceDifference < 0) //decreasing dmg
	{
		const double defenseMultiplier = VLC->modh->settings.DEFENSE_POINT_DMG_MULTIPLIER;
		const double defenseMultiplierCap = VLC->modh->settings.DEFENSE_POINTS_DMG_MULTIPLIER_CAP;

		const double dec = std::min(defenseMultiplier * (-attackDefenceDifference), defenseMultiplierCap);
		multBonus *= 1.0 - dec;
	}
	else //increasing dmg
	{
		const double attackMultiplier = VLC->modh->settings.ATTACK_POINT_DMG_MULTIPLIER;
		const double attackMultiplierCap = VLC->modh->settings.ATTACK_POINTS_DMG_MULTIPLIER_CAP;

		const double inc = std::min(attackMultiplier * attackDefenceDifference, attackMultiplierCap);
		additiveBonus += inc;
	}

	const std::string cachingStrJousting = "type_JOUSTING";
	static const auto selectorJousting = Selector::type()(Bonus::JOUSTING);

	const std::string cachingStrChargeImmunity = "type_CHARGE_IMMUNITY";
	static const auto selectorChargeImmunity = Selector::type()(Bonus::CHARGE_IMMUNITY);

	//applying jousting bonus
	if(info.chargedFields > 0 && attackerBonuses->hasBonus(selectorJousting, cachingStrJousting) && !defenderBonuses->hasBonus(selectorChargeImmunity, cachingStrChargeImmunity))
		additiveBonus += info.chargedFields * 0.05;

	//handling secondary abilities and artifacts giving premies to them
	const std::string cachingStrArchery = "type_SECONDARY_SKILL_PREMYs_ARCHERY";
	static const auto selectorArchery = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARCHERY);

	const std::string cachingStrOffence = "type_SECONDARY_SKILL_PREMYs_OFFENCE";
	static const auto selectorOffence = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::OFFENCE);

	const std::string cachingStrArmorer = "type_SECONDARY_SKILL_PREMYs_ARMORER";
	static const auto selectorArmorer = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARMORER);

	if(info.shooting)
		additiveBonus += attackerBonuses->valOfBonuses(selectorArchery, cachingStrArchery) / 100.0;
	else
		additiveBonus += attackerBonuses->valOfBonuses(selectorOffence, cachingStrOffence) / 100.0;

	multBonus *= (std::max(0, 100 - defenderBonuses->valOfBonuses(selectorArmorer, cachingStrArmorer))) / 100.0;

	//handling hate effect
	//assume that unit have only few HATE features and cache them all
	const std::string cachingStrHate = "type_HATE";
	static const auto selectorHate = Selector::type()(Bonus::HATE);

	auto allHateEffects = attackerBonuses->getBonuses(selectorHate, cachingStrHate);

	additiveBonus += allHateEffects->valOfBonuses(Selector::subtype()(info.defender->creatureIndex())) / 100.0;

	const std::string cachingStrMeleeReduction = "type_GENERAL_DAMAGE_REDUCTIONs_0";
	static const auto selectorMeleeReduction = Selector::typeSubtype(Bonus::GENERAL_DAMAGE_REDUCTION, 0);

	const std::string cachingStrRangedReduction = "type_GENERAL_DAMAGE_REDUCTIONs_1";
	static const auto selectorRangedReduction = Selector::typeSubtype(Bonus::GENERAL_DAMAGE_REDUCTION, 1);

	//handling spell effects
	if(!info.shooting) //eg. shield
	{
		multBonus *= (100 - defenderBonuses->valOfBonuses(selectorMeleeReduction, cachingStrMeleeReduction)) / 100.0;
	}
	else //eg. air shield
	{
		multBonus *= (100 - defenderBonuses->valOfBonuses(selectorRangedReduction, cachingStrRangedReduction)) / 100.0;
	}

	if(info.shooting)
	{
		//todo: set actual percentage in spell bonus configuration instead of just level; requires non trivial backward compatibility handling

		//get list first, total value of 0 also counts
		TConstBonusListPtr forgetfulList = attackerBonuses->getBonuses(Selector::type()(Bonus::FORGETFULL),"type_FORGETFULL");

		if(!forgetfulList->empty())
		{
			int forgetful = forgetfulList->valOfBonuses(Selector::all);

			//none of basic level
			if(forgetful == 0 || forgetful == 1)
				multBonus *= 0.5;
			else
				logGlobal->warn("Attempt to calculate shooting damage with adv+ FORGETFULL effect");
		}
	}

	const std::string cachingStrForcedMinDamage = "type_ALWAYS_MINIMUM_DAMAGE";
	static const auto selectorForcedMinDamage = Selector::type()(Bonus::ALWAYS_MINIMUM_DAMAGE);

	const std::string cachingStrForcedMaxDamage = "type_ALWAYS_MAXIMUM_DAMAGE";
	static const auto selectorForcedMaxDamage = Selector::type()(Bonus::ALWAYS_MAXIMUM_DAMAGE);

	TConstBonusListPtr curseEffects = attackerBonuses->getBonuses(selectorForcedMinDamage, cachingStrForcedMinDamage);
	TConstBonusListPtr blessEffects = attackerBonuses->getBonuses(selectorForcedMaxDamage, cachingStrForcedMaxDamage);

	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();
	double curseMultiplicativePenalty = curseEffects->size() ? (*std::max_element(curseEffects->begin(), curseEffects->end(), &Bonus::compareByAdditionalInfo<std::shared_ptr<Bonus>>))->additionalInfo[0] : 0;

	if(curseMultiplicativePenalty) //curse handling (partial, the rest is below)
	{
		multBonus *= 1.0 - curseMultiplicativePenalty/100;
	}

	const std::string cachingStrAdvAirShield = "isAdvancedAirShield";
	auto isAdvancedAirShield = [](const Bonus* bonus)
	{
		return bonus->source == Bonus::SPELL_EFFECT
				&& bonus->sid == SpellID::AIR_SHIELD
				&& bonus->val >= SecSkillLevel::ADVANCED;
	};

	if(info.shooting)
	{
		//wall / distance penalty + advanced air shield
		BattleHex attackerPos = info.attackerPos.isValid() ? info.attackerPos : info.attacker->getPosition();
		BattleHex defenderPos = info.defenderPos.isValid() ? info.defenderPos : info.defender->getPosition();

		const bool distPenalty = battleHasDistancePenalty(attackerBonuses, attackerPos, defenderPos);
		const bool obstaclePenalty = battleHasWallPenalty(attackerBonuses, attackerPos, defenderPos);

		if(distPenalty || defenderBonuses->hasBonus(isAdvancedAirShield, cachingStrAdvAirShield))
			multBonus *= 0.5;

		if(obstaclePenalty)
			multBonus *= 0.5; //cumulative
	}
	else
	{
		const std::string cachingStrNoMeleePenalty = "type_NO_MELEE_PENALTY";
		static const auto selectorNoMeleePenalty = Selector::type()(Bonus::NO_MELEE_PENALTY);

		if(info.attacker->isShooter() && !attackerBonuses->hasBonus(selectorNoMeleePenalty, cachingStrNoMeleePenalty))
			multBonus *= 0.5;
	}

	// psychic elementals versus mind immune units 50%
	if(info.attacker->creatureIndex() == CreatureID::PSYCHIC_ELEMENTAL)
	{
		const std::string cachingStrMindImmunity = "type_MIND_IMMUNITY";
		static const auto selectorMindImmunity = Selector::type()(Bonus::MIND_IMMUNITY);

		if(defenderBonuses->hasBonus(selectorMindImmunity, cachingStrMindImmunity))
			multBonus *= 0.5;
	}

	// TODO attack on petrified unit 50%
	// blinded unit retaliates

	minDmg *= additiveBonus * multBonus;
	maxDmg *= additiveBonus * multBonus;

	if(curseEffects->size()) //curse handling (rest)
	{
		minDmg += curseBlessAdditiveModifier;
		maxDmg = minDmg;
	}
	else if(blessEffects->size()) //bless handling
	{
		maxDmg += curseBlessAdditiveModifier;
		minDmg = maxDmg;
	}

	TDmgRange returnedVal = std::make_pair(int64_t(minDmg), int64_t(maxDmg));

	//damage cannot be less than 1
	vstd::amax(returnedVal.first, 1);
	vstd::amax(returnedVal.second, 1);

	return returnedVal;
}

TDmgRange CBattleInfoCallback::battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));
	const bool shooting = battleCanShoot(attacker, defender->getPosition());
	const BattleAttackInfo bai(attacker, defender, shooting);
	return battleEstimateDamage(bai, retaliationDmg);
}

TDmgRange CBattleInfoCallback::battleEstimateDamage(const BattleAttackInfo & bai, TDmgRange * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));

	TDmgRange ret = calculateDmgRange(bai);

	if(retaliationDmg)
	{
		if(bai.shooting)
		{
			//FIXME: handle RANGED_RETALIATION
			retaliationDmg->first = retaliationDmg->second = 0;
		}
		else
		{
			//TODO: rewrite using boost::numeric::interval
			//TODO: rewire once more using interval-based fuzzy arithmetic

			int64_t TDmgRange::* pairElems[] = {&TDmgRange::first, &TDmgRange::second};
			for (int i=0; i<2; ++i)
			{
				auto retaliationAttack = bai.reverse();
				int64_t dmg = ret.*pairElems[i];
				auto state = retaliationAttack.attacker->acquireState();
				state->damage(dmg);
				retaliationAttack.attacker = state.get();
				retaliationDmg->*pairElems[!i] = calculateDmgRange(retaliationAttack).*pairElems[!i];
			}
		}
	}

	return ret;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking) const
{
	std::vector<std::shared_ptr<const CObstacleInstance>> obstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	RETURN_IF_NOT_BATTLE(obstacles);
	for(auto & obs : battleGetAllObstacles())
	{
		if(vstd::contains(obs->getBlockedTiles(), tile)
				|| (!onlyBlocking && vstd::contains(obs->getAffectedTiles(), tile)))
		{
			obstacles.push_back(obs);
		}
	}
	return obstacles;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::getAllAffectedObstaclesByStack(const battle::Unit * unit) const
{
	std::vector<std::shared_ptr<const CObstacleInstance>> affectedObstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	RETURN_IF_NOT_BATTLE(affectedObstacles);
	if(unit->alive())
	{
		affectedObstacles = battleGetAllObstaclesOnPos(unit->getPosition(), false);
		if(unit->doubleWide())
		{
			BattleHex otherHex = unit->occupiedHex(unit->getPosition());
			if(otherHex.isValid())
				for(auto & i : battleGetAllObstaclesOnPos(otherHex, false))
					affectedObstacles.push_back(i);
		}
		for(auto hex : unit->getHexes())
			if(hex == ESiegeHex::GATE_BRIDGE)
				if(battleGetGateState() == EGateState::OPENED || battleGetGateState() == EGateState::DESTROYED)
					for(int i=0; i<affectedObstacles.size(); i++)
						if(affectedObstacles.at(i)->obstacleType == CObstacleInstance::MOAT)
							affectedObstacles.erase(affectedObstacles.begin()+i);
	}
	return affectedObstacles;
}

AccessibilityInfo CBattleInfoCallback::getAccesibility() const
{
	AccessibilityInfo ret;
	ret.fill(EAccessibility::ACCESSIBLE);

	//removing accessibility for side columns of hexes
	for(int y = 0; y < GameConstants::BFIELD_HEIGHT; y++)
	{
		ret[BattleHex(GameConstants::BFIELD_WIDTH - 1, y)] = EAccessibility::SIDE_COLUMN;
		ret[BattleHex(0, y)] = EAccessibility::SIDE_COLUMN;
	}

	//special battlefields with logically unavailable tiles
	auto bFieldType = battleGetBattlefieldType();

	if(bFieldType != BattleField::NONE)
	{
		std::vector<BattleHex> impassableHexes = bFieldType.getInfo()->impassableHexes;

		for(auto hex : impassableHexes)
			ret[hex] = EAccessibility::UNAVAILABLE;
	}

	//gate -> should be before stacks
	if(battleGetSiegeLevel() > 0)
	{
		EAccessibility accessability = EAccessibility::ACCESSIBLE;
		switch(battleGetGateState())
		{
		case EGateState::CLOSED:
			accessability = EAccessibility::GATE;
			break;

		case EGateState::BLOCKED:
			accessability = EAccessibility::UNAVAILABLE;
			break;
		}
		ret[ESiegeHex::GATE_OUTER] = ret[ESiegeHex::GATE_INNER] = accessability;
	}

	//tiles occupied by standing stacks
	for(auto unit : battleAliveUnits())
	{
		for(auto hex : unit->getHexes())
			if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
				ret[hex] = EAccessibility::ALIVE_STACK;
	}

	//obstacles
	for(const auto &obst : battleGetAllObstacles())
	{
		for(auto hex : obst->getBlockedTiles())
			ret[hex] = EAccessibility::OBSTACLE;
	}

	//walls
	if(battleGetSiegeLevel() > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 62, 112, 147, 165};
		for(auto hex : permanentlyLocked)
			ret[hex] = EAccessibility::UNAVAILABLE;

		//TODO likely duplicated logic
		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] =
		{
			//which part of wall, which hex is blocked if this part of wall is not destroyed
			std::make_pair(2, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_4)),
			std::make_pair(3, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_3)),
			std::make_pair(4, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_2)),
			std::make_pair(5, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_1))
		};

		for(auto & elem : lockedIfNotDestroyed)
		{
			if(battleGetWallState(elem.first) != EWallState::DESTROYED)
				ret[elem.second] = EAccessibility::DESTRUCTIBLE_WALL;
		}
	}

	return ret;
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const battle::Unit * stack) const
{
	return getAccesibility(battle::Unit::getHexes(stack->getPosition(), stack->doubleWide(), stack->unitSide()));
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const std::vector<BattleHex> & accessibleHexes) const
{
	auto ret = getAccesibility();
	for(auto hex : accessibleHexes)
		if(hex.isValid())
			ret[hex] = EAccessibility::ACCESSIBLE;

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const AccessibilityInfo &accessibility, const ReachabilityInfo::Parameters & params) const
{
	ReachabilityInfo ret;
	ret.accessibility = accessibility;
	ret.params = params;

	ret.predecessors.fill(BattleHex::INVALID);
	ret.distances.fill(ReachabilityInfo::INFINITE_DIST);

	if(!params.startPosition.isValid()) //if got call for arrow turrets
		return ret;

	const std::set<BattleHex> obstacles = getStoppers(params.perspective);

	std::queue<BattleHex> hexq; //bfs queue

	//first element
	hexq.push(params.startPosition);
	ret.distances[params.startPosition] = 0;

	std::array<bool, GameConstants::BFIELD_SIZE> accessibleCache;
	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
		accessibleCache[hex] = accessibility.accessible(hex, params.doubleWide, params.side);

	while(!hexq.empty()) //bfs loop
	{
		const BattleHex curHex = hexq.front();
		hexq.pop();

		//walking stack can't step past the obstacles
		if(curHex != params.startPosition && isInObstacle(curHex, obstacles, params))
			continue;

		const int costToNeighbour = ret.distances[curHex.hex] + 1;
		for(BattleHex neighbour : BattleHex::neighbouringTilesCache[curHex.hex])
		{
			if(neighbour.isValid())
			{
				const int costFoundSoFar = ret.distances[neighbour.hex];

				if(accessibleCache[neighbour.hex] && costToNeighbour < costFoundSoFar)
				{
					hexq.push(neighbour);
					ret.distances[neighbour.hex] = costToNeighbour;
					ret.predecessors[neighbour.hex] = curHex;
				}
			}
		}
	}

	return ret;
}


bool CBattleInfoCallback::isInObstacle(
	BattleHex hex,
	const std::set<BattleHex> & obstacles,
	const ReachabilityInfo::Parameters & params) const
{
	auto occupiedHexes = battle::Unit::getHexes(hex, params.doubleWide, params.side);

	for(auto occupiedHex : occupiedHexes)
	{
		if(vstd::contains(obstacles, occupiedHex))
		{
			if(occupiedHex == ESiegeHex::GATE_BRIDGE)
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

std::set<BattleHex> CBattleInfoCallback::getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const
{
	std::set<BattleHex> ret;
	RETURN_IF_NOT_BATTLE(ret);

	for(auto &oi : battleGetAllObstacles(whichSidePerspective))
	{
		if(battleIsObstacleVisibleForSide(*oi, whichSidePerspective))
		{
			range::copy(oi->getStoppingTile(), vstd::set_inserter(ret));
		}
	}

	return ret;
}

std::pair<const battle::Unit *, BattleHex> CBattleInfoCallback::getNearestStack(const battle::Unit * closest) const
{
	auto reachability = getReachability(closest);
	auto avHexes = battleGetAvailableHexes(reachability, closest);

	// I hate std::pairs with their undescriptive member names first / second
	struct DistStack
	{
		uint32_t distanceToPred;
		BattleHex destination;
		const battle::Unit * stack;
	};

	std::vector<DistStack> stackPairs;

	std::vector<const battle::Unit *> possible = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget(false) && unit != closest;
	});

	for(const battle::Unit * st : possible)
	{
		for(BattleHex hex : avHexes)
			if(CStack::isMeleeAttackPossible(closest, st, hex))
			{
				DistStack hlp = {reachability.distances[hex], hex, st};
				stackPairs.push_back(hlp);
			}
	}

	if (stackPairs.size())
	{
		auto comparator = [](DistStack lhs, DistStack rhs) { return lhs.distanceToPred < rhs.distanceToPred; };
		auto minimal = boost::min_element(stackPairs, comparator);
		return std::make_pair(minimal->stack, minimal->destination);
	}
	else
		return std::make_pair<const battle::Unit * , BattleHex>(nullptr, BattleHex::INVALID);
}

BattleHex CBattleInfoCallback::getAvaliableHex(CreatureID creID, ui8 side, int initialPos) const
{
	bool twoHex = VLC->creh->objects[creID]->isDoubleWide();

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

	auto accessibility = getAccesibility();

	std::set<BattleHex> occupyable;
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

bool CBattleInfoCallback::isInTacticRange(BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto side = battleGetTacticsSide();
	auto dist = battleGetTacticDist();

	return ((!side && dest.getX() > 0 && dest.getX() <= dist)
			|| (side && dest.getX() < GameConstants::BFIELD_WIDTH - 1 && dest.getX() >= GameConstants::BFIELD_WIDTH - dist - 1));
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

ReachabilityInfo CBattleInfoCallback::getReachability(const ReachabilityInfo::Parameters &params) const
{
	if(params.flying)
		return getFlyingReachability(params);
	else
		return makeBFS(getAccesibility(params.knownAccessible), params);
}

ReachabilityInfo CBattleInfoCallback::getFlyingReachability(const ReachabilityInfo::Parameters &params) const
{
	ReachabilityInfo ret;
	ret.accessibility = getAccesibility(params.knownAccessible);

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

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes (const  battle::Unit* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	//does not return hex attacked directly
	AttackableTiles at;
	RETURN_IF_NOT_BATTLE(at);

	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos : attacker->getPosition(); //real or hypothetical (cursor) position

	auto defender = battleGetUnitByPos(destinationTile, true);
	if (!defender)
		return at; // can't attack thin air

	//FIXME: dragons or cerbers can rotate before attack, making their base hex different (#1124)
	bool reverse = isToReverse(destinationTile, attacker, defender);
	if(reverse && attacker->doubleWide())
	{
		hex = attacker->occupiedHex(hex); //the other hex stack stands on
	}
	if(attacker->hasBonusOfType(Bonus::ATTACKS_ALL_ADJACENT))
	{
		boost::copy(attacker->getSurroundingHexes(attackerPos), vstd::set_inserter(at.hostileCreaturePositions));
	}
	if(attacker->hasBonusOfType(Bonus::THREE_HEADED_ATTACK))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		for(BattleHex tile : hexes)
		{
			if((BattleHex::mutualPosition(tile, destinationTile) > -1 && BattleHex::mutualPosition(tile, hex) > -1)) //adjacent both to attacker's head and attacked tile
			{
				auto st = battleGetUnitByPos(tile, true);
				if(st && battleMatchOwner(st, attacker)) //only hostile stacks - does it work well with Berserk?
					at.hostileCreaturePositions.insert(tile);
			}
		}
	}
	if(attacker->hasBonusOfType(Bonus::WIDE_BREATH))
	{
		std::vector<BattleHex> hexes = destinationTile.neighbouringTiles();
		for(int i = 0; i<hexes.size(); i++)
		{
			if(hexes.at(i) == hex)
			{
				hexes.erase(hexes.begin() + i);
				i = 0;
			}
		}
		for(BattleHex tile : hexes)
		{
			//friendly stacks can also be damaged by Dragon Breath
			auto st = battleGetUnitByPos(tile, true);
			if(st && st != attacker)
				at.friendlyCreaturePositions.insert(tile);
		}
	}
	else if(attacker->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH))
	{
		auto direction = BattleHex::mutualPosition(hex, destinationTile);
		if(direction != BattleHex::NONE) //only adjacent hexes are subject of dragon breath calculation
		{
			BattleHex nextHex = destinationTile.cloneInDirection(direction, false);

			if (nextHex.isValid())
			{
				//friendly stacks can also be damaged by Dragon Breath
				auto st = battleGetUnitByPos(nextHex, true);
				if(st != nullptr)
					at.friendlyCreaturePositions.insert(nextHex);
			}
		}
	}
	return at;
}

AttackableTiles CBattleInfoCallback::getPotentiallyShootableHexes(const  battle::Unit * attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	//does not return hex attacked directly
	AttackableTiles at;
	RETURN_IF_NOT_BATTLE(at);

	if(attacker->hasBonusOfType(Bonus::SHOOTS_ALL_ADJACENT) && !vstd::contains(attackerPos.neighbouringTiles(), destinationTile))
	{
		std::vector<BattleHex> targetHexes = destinationTile.neighbouringTiles();
		targetHexes.push_back(destinationTile);
		boost::copy(targetHexes, vstd::set_inserter(at.hostileCreaturePositions));
	}

	return at;
}

std::vector<const battle::Unit*> CBattleInfoCallback::getAttackedBattleUnits(const battle::Unit* attacker, BattleHex destinationTile, bool rangedAttack, BattleHex attackerPos) const
{
	std::vector<const battle::Unit*> units;
	RETURN_IF_NOT_BATTLE(units);

	AttackableTiles at;

	if (rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
		at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	units = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		if (unit->isGhost() || !unit->alive())
			return false;

		for (BattleHex hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
		{
			if (vstd::contains(at.hostileCreaturePositions, hex))
				return true;
			if (vstd::contains(at.friendlyCreaturePositions, hex))
				return true;
		}
		return false;
	});

	return units;
}

std::set<const CStack*> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, bool rangedAttack, BattleHex attackerPos) const
{
	std::set<const CStack*> attackedCres;
	RETURN_IF_NOT_BATTLE(attackedCres);

	AttackableTiles at;

	if(rangedAttack)
		at = getPotentiallyShootableHexes(attacker, destinationTile, attackerPos);
	else
		at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (BattleHex tile : at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.insert(st);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.insert(st);
		}
	}
	return attackedCres;
}

static bool isHexInFront(BattleHex hex, BattleHex testHex, BattleSide::Type side )
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
bool CBattleInfoCallback::isToReverse (BattleHex attackerHex, const battle::Unit * attacker, const battle::Unit * defender) const
{
	if (attackerHex < 0 ) //turret
		return false;

	BattleHex defenderHex = defender->getPosition();

	if (isHexInFront(attackerHex, defenderHex, BattleSide::Type(attacker->unitSide())))
		return false;

	if (defender->doubleWide())
	{
		if (isHexInFront(attackerHex,defender->occupiedHex(), BattleSide::Type(attacker->unitSide())))
			return false;
	}

	if (attacker->doubleWide())
	{
		if (isHexInFront(attacker->occupiedHex(), defenderHex, BattleSide::Type(attacker->unitSide())))
			return false;
	}

	// a bit weird case since here defender is slightly behind attacker, so reversing seems preferable,
	// but this is how H3 handles it which is important, e.g. for direction of dragon breath attacks
	if (attacker->doubleWide() && defender->doubleWide())
	{
		if (isHexInFront(attacker->occupiedHex(), defender->occupiedHex(), BattleSide::Type(attacker->unitSide())))
			return false;
	}
	return true;
}

ReachabilityInfo::TDistances CBattleInfoCallback::battleGetDistances(const battle::Unit * unit, BattleHex assumedPosition) const
{
	ReachabilityInfo::TDistances ret;
	ret.fill(-1);
	RETURN_IF_NOT_BATTLE(ret);

	auto reachability = getReachability(unit);

	boost::copy(reachability.distances, ret.begin());

	return ret;
}

bool CBattleInfoCallback::battleHasDistancePenalty(const IBonusBearer * shooter, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);

	const std::string cachingStrNoDistancePenalty = "type_NO_DISTANCE_PENALTY";
	static const auto selectorNoDistancePenalty = Selector::type()(Bonus::NO_DISTANCE_PENALTY);

	if(shooter->hasBonus(selectorNoDistancePenalty, cachingStrNoDistancePenalty))
		return false;

	if(auto target = battleGetUnitByPos(destHex, true))
	{
		//If any hex of target creature is within range, there is no penalty
		for(auto hex : target->getHexes())
			if(BattleHex::getDistance(shooterPosition, hex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
				return false;
		//TODO what about two-hex shooters?
	}
	else
	{
		if(BattleHex::getDistance(shooterPosition, destHex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
			return false;
	}

	return true;
}

BattleHex CBattleInfoCallback::wallPartToBattleHex(EWallPart::EWallPart part) const
{
	RETURN_IF_NOT_BATTLE(BattleHex::INVALID);
	return WallPartToHex(part);
}

EWallPart::EWallPart CBattleInfoCallback::battleHexToWallPart(BattleHex hex) const
{
	RETURN_IF_NOT_BATTLE(EWallPart::INVALID);
	return hexToWallPart(hex);
}

bool CBattleInfoCallback::isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const
{
	RETURN_IF_NOT_BATTLE(false);
	return wallPart != EWallPart::INDESTRUCTIBLE_PART && wallPart != EWallPart::INDESTRUCTIBLE_PART_OF_GATE &&
																	wallPart != EWallPart::INVALID;
}

std::vector<BattleHex> CBattleInfoCallback::getAttackableBattleHexes() const
{
	std::vector<BattleHex> attackableBattleHexes;
	RETURN_IF_NOT_BATTLE(attackableBattleHexes);

	for(auto & wallPartPair : wallParts)
	{
		if(isWallPartPotentiallyAttackable(wallPartPair.second))
		{
			auto wallState = static_cast<EWallState::EWallState>(battleGetWallState(static_cast<int>(wallPartPair.second)));
			if(wallState == EWallState::INTACT || wallState == EWallState::DAMAGED)
			{
				attackableBattleHexes.push_back(BattleHex(wallPartPair.first));
			}
		}
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

	for(auto unit : battleAliveUnits())
	{
		if(unit->unitOwner() == caster->tempOwner && unit->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ALLY))
		{
			vstd::amax(manaReduction, unit->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if(unit->unitOwner() != caster->tempOwner && unit->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ENEMY))
		{
			vstd::amax(manaIncrease, unit->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return ret - manaReduction + manaIncrease;
}

bool CBattleInfoCallback::battleHasShootingPenalty(const battle::Unit * shooter, BattleHex destHex) const
{
	return battleHasDistancePenalty(shooter, shooter->getPosition(), destHex) || battleHasWallPenalty(shooter, shooter->getPosition(), destHex);
}

bool CBattleInfoCallback::battleIsUnitBlocked(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(unit->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	for(auto adjacent : battleAdjacentUnits(unit))
	{
		if(adjacent->unitOwner() != unit->unitOwner()) //blocked by enemy stack
			return true;
	}
	return false;
}

std::set<const battle::Unit *> CBattleInfoCallback::battleAdjacentUnits(const battle::Unit * unit) const
{
	std::set<const battle::Unit *> ret;
	RETURN_IF_NOT_BATTLE(ret);

	for(auto hex : unit->getSurroundingHexes())
	{
		if(auto neighbour = battleGetUnitByPos(hex, true))
			ret.insert(neighbour);
	}

	return ret;
}

SpellID CBattleInfoCallback::getRandomBeneficialSpell(CRandomGenerator & rand, const CStack * subject) const
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

	auto getAliveEnemy = [=](const std::function<bool(const CStack *)> & pred) -> const CStack *
	{
		auto stacks = battleGetStacksIf([=](const CStack * stack)
		{
			return pred(stack) && stack->owner != subject->owner && stack->isValidTarget(false);
		});

		if(stacks.empty())
			return nullptr;
		else
			return stacks.front();
	};

	for(const SpellID& spellID : allPossibleSpells)
	{
		std::stringstream cachingStr;
		cachingStr << "source_" << Bonus::SPELL_EFFECT << "id_" << spellID.num;

		if(subject->hasBonus(Selector::source(Bonus::SPELL_EFFECT, spellID), Selector::all, cachingStr.str())
		 //TODO: this ability has special limitations
		|| !(spellID.toSpell()->canBeCast(this, spells::Mode::CREATURE_ACTIVE, subject)))
			continue;

		switch (spellID)
		{
		case SpellID::SHIELD:
		case SpellID::FIRE_SHIELD: // not if all enemy units are shooters
		{
			auto walker = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return !stack->canShoot();
			});

			if (!walker)
				continue;
		}
			break;
		case SpellID::AIR_SHIELD: //only against active shooters
		{
			auto shooter = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return stack->canShoot();
			});
			if (!shooter)
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
			const ui8 enemySide = 1 - subject->side;
			//todo: only if enemy has spellbook
			if (!battleHasHero(enemySide)) //only if there is enemy hero
				continue;
		}
			break;
		case SpellID::CURE: //only damaged units
		{
			//do not cast on affected by debuffs
			if(!subject->canBeHealed())
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
			auto kingMonster = getAliveEnemy([&](const CStack * stack) -> bool //look for enemy, non-shooting stack
			{
				const auto isKing = Selector::type()(Bonus::KING1)
									.Or(Selector::type()(Bonus::KING2))
									.Or(Selector::type()(Bonus::KING3));

				return stack->hasBonus(isKing);
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

SpellID CBattleInfoCallback::getRandomCastedSpell(CRandomGenerator & rand,const CStack * caster) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);

	TConstBonusListPtr bl = caster->getBonuses(Selector::type()(Bonus::SPELLCASTER));
	if (!bl->size())
		return SpellID::NONE;
	int totalWeight = 0;
	for(auto b : *bl)
	{
		totalWeight += std::max(b->additionalInfo[0], 1); //minimal chance to cast is 1
	}
	int randomPos = rand.nextInt(totalWeight - 1);
	for(auto b : *bl)
	{
		randomPos -= std::max(b->additionalInfo[0], 1);
		if(randomPos < 0)
		{
			return SpellID(b->subtype);
		}
	}

	return SpellID::NONE;
}

int CBattleInfoCallback::battleGetSurrenderCost(PlayerColor Player) const
{
	RETURN_IF_NOT_BATTLE(-3);
	if(!battleCanSurrender(Player))
		return -1;

	const auto sideOpt = playerToSide(Player);
	if(!sideOpt)
		return -1;
	const auto side = sideOpt.get();

	int ret = 0;
	double discount = 0;

	for(auto unit : battleAliveUnits(side))
		ret += unit->getRawSurrenderCost();

	if(const CGHeroInstance * h = battleGetFightingHero(side))
		discount += h->valOfBonuses(Bonus::SURRENDER_DISCOUNT);

	ret = static_cast<int>(ret * (100.0 - discount) / 100.0);
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

si8 CBattleInfoCallback::battleMinSpellLevel(ui8 side) const
{
	const IBonusBearer * node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBattleNode();

	if(!node)
		return 0;

	auto b = node->getBonuses(Selector::type()(Bonus::BLOCK_MAGIC_BELOW));
	if(b->size())
		return b->totalValue();

	return 0;
}

si8 CBattleInfoCallback::battleMaxSpellLevel(ui8 side) const
{
	const IBonusBearer *node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBattleNode();

	if(!node)
		return GameConstants::SPELL_LEVELS;

	//We can't "just get value" - it'd be 0 if there are bonuses (and all would be blocked)
	auto b = node->getBonuses(Selector::type()(Bonus::BLOCK_MAGIC_ABOVE));
	if(b->size())
		return b->totalValue();

	return GameConstants::SPELL_LEVELS;
}

boost::optional<int> CBattleInfoCallback::battleIsFinished() const
{
	auto units = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->alive() && !unit->isTurret() && !unit->hasBonusOfType(Bonus::SIEGE_WEAPON);
	});

	std::array<bool, 2> hasUnit = {false, false}; //index is BattleSide

	for(auto & unit : units)
	{
		//todo: move SIEGE_WEAPON check to Unit state
		hasUnit.at(unit->unitSide()) = true;

		if(hasUnit[0] && hasUnit[1])
			return boost::none;
	}
	
	hasUnit = {false, false};

	for(auto & unit : units)
	{
		if(!unit->isClone() && !unit->acquireState()->summoned && !dynamic_cast <const CCommanderInstance *>(unit))
		{
			hasUnit.at(unit->unitSide()) = true;
		}
	}

	if(!hasUnit[0] && !hasUnit[1])
		return 2;
	if(!hasUnit[1])
		return 0;
	else
		return 1;
}

VCMI_LIB_NAMESPACE_END
