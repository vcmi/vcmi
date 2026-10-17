/*
 * BattleAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleAction.h"
#include "Unit.h"
#include "CBattleInfoCallback.h"
#include "actions/BattleActionType.h"

static const int32_t INVALID_UNIT_ID = -1000;

BattleAction::BattleAction():
	side(BattleSide::NONE),
	stackNumber(-1),
	actionType(EActionType::NO_ACTION)
{
}

BattleAction BattleAction::makeHeal(const battle::Unit * healer, const battle::Unit * healed)
{
	BattleAction ba;
	ba.side = healer->unitSide();
	ba.actionType = EActionType::STACK_HEAL;
	ba.stackNumber = healer->unitId();
	ba.aimToUnit(healed);
	return ba;
}

BattleAction BattleAction::makeHeal(const battle::Unit * healer)
{
	BattleAction ba;
	ba.side = healer->unitSide();
	ba.actionType = EActionType::STACK_HEAL;
	ba.stackNumber = healer->unitId();
	return ba;
}

BattleAction BattleAction::makeHeal(const battle::Unit * healer, const BattleHex & destination)
{
	BattleAction ba;
	ba.side = healer->unitSide();
	ba.actionType = EActionType::STACK_HEAL;
	ba.stackNumber = healer->unitId();
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeCatapultShot(const battle::Unit * catapult)
{
	BattleAction ba;
	ba.side = catapult->unitSide();
	ba.actionType = EActionType::CATAPULT;
	ba.stackNumber = catapult->unitId();
	return ba;
}

BattleAction BattleAction::makeNoAction(const battle::Unit * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::NO_ACTION;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeBadMorale(const battle::Unit * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::BAD_MORALE;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeDefend(const battle::Unit * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::DEFEND;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeMeleeAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom, bool returnAfterAttack)
{
	BattleAction ba;
	ba.side = stack->unitSide(); //FIXME: will it fail if stack mind controlled?
	ba.actionType = returnAfterAttack ? EActionType::WALK_AND_ATTACK : EActionType::WALK_AND_ATTACK_NO_RETURN;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(attackFrom);
	ba.aimToHex(destination);
	return ba;
}

/// This is for cases where target hex does not matter (for example berserk spell) and we did not calculate it (for example we only calculated closest attackable hex of two hex unit)
BattleAction BattleAction::makeMeleeAttack(const battle::Unit * stack, const battle::Unit * target, const BattleHex & attackFrom, bool returnAfterAttack)
{
	BattleAction ba;
	ba.side = stack->unitSide(); //FIXME: will it fail if stack mind controlled?
	ba.actionType = returnAfterAttack ? EActionType::WALK_AND_ATTACK : EActionType::WALK_AND_ATTACK_NO_RETURN;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(attackFrom);
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeLongWeaponAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::LONG_WEAPON_ATTACK;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(attackFrom);
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeWait(const battle::Unit * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::WAIT;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeShotAttack(const battle::Unit * shooter, const battle::Unit * target)
{
	BattleAction ba;
	ba.side = shooter->unitSide();
	ba.actionType = EActionType::SHOOT;
	ba.stackNumber = shooter->unitId();
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeShotAttack(const battle::Unit * shooter, const BattleHex & destination)
{
	BattleAction ba;
	ba.side = shooter->unitSide();
	ba.actionType = EActionType::SHOOT;
	ba.stackNumber = shooter->unitId();
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeCreatureSpellcast(const battle::Unit * stack, const battle::Target & target, const SpellID & spellID)
{
	BattleAction ba;
	ba.actionType = EActionType::MONSTER_SPELL;
	ba.spell = spellID;
	ba.setTarget(target);
	ba.side = stack->unitSide();
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeHeroSpellcast(BattleSide side, const battle::Target & target, const SpellID & spellID)
{
	BattleAction ba;
	ba.actionType = EActionType::HERO_SPELL;
	ba.spell = spellID;
	ba.setTarget(target);
	ba.side = side;
	return ba;
}

BattleAction BattleAction::makeRandomSpellcast(const battle::Unit * stack, const BattleHex & destination)
{
	BattleAction ba;
	ba.actionType = EActionType::RANDOM_SPELL;
	ba.side = stack->unitSide();
	ba.stackNumber = stack->unitId();
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeWalkAndCast(const battle::Unit * stack, const BattleHex & castFrom, const battle::Unit * target, const SpellID & spellID)
{
	BattleAction ba;
	ba.actionType = EActionType::WALK_AND_CAST;
	ba.spell = spellID;
	ba.side = stack->unitSide();
	ba.stackNumber = stack->unitId();
	ba.aimToHex(castFrom);
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeMove(const battle::Unit * stack, const BattleHex & dest)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::WALK;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(dest);
	return ba;
}

BattleAction BattleAction::makeEndOFTacticPhase(BattleSide side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::END_TACTIC_PHASE;
	return ba;
}

BattleAction BattleAction::makeSurrender(BattleSide side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::SURRENDER;
	return ba;
}

BattleAction BattleAction::makeRetreat(BattleSide side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::RETREAT;
	return ba;
}

std::string BattleAction::toString() const
{
	std::stringstream targetStream;

	for(const DestinationInfo & info : target)
	{
		if(info.unitValue == INVALID_UNIT_ID)
		{
			targetStream << info.hexValue;
		}
		else
		{
			targetStream << info.unitValue;
			targetStream << "@";
			targetStream << info.hexValue;
		}
		targetStream << ",";
	}

	boost::format fmt("{BattleAction: side '%d', stackNumber '%d', actionType '%s', actionSubtype '%d', target {%s}}");
	fmt % static_cast<int>(side) % stackNumber % static_cast<int>(actionType) % spell.getNum() % targetStream.str();
	return fmt.str();
}

void BattleAction::aimToHex(const BattleHex & destination)
{
	DestinationInfo info;
	info.hexValue = destination;
	info.unitValue = INVALID_UNIT_ID;

	target.push_back(info);
}

void BattleAction::aimToUnit(const battle::Unit * destination)
{
	DestinationInfo info;
	info.hexValue = destination->getPosition();
	info.unitValue = destination->unitId();

	target.push_back(info);
}

battle::Target BattleAction::getTarget(const CBattleInfoCallback * cb) const
{
	battle::Target ret;

	for(const auto & destination : target)
	{
		if(destination.unitValue == INVALID_UNIT_ID)
		{
			ret.emplace_back(destination.hexValue);
			continue;
		}

		// an action may name a unit that is not in the battle; its destination has no unit and no hex
		const battle::Unit * unit = cb->battleGetUnitByID(destination.unitValue);
		ret.push_back(unit ? battle::Destination(unit) : battle::Destination());
	}

	return ret;
}

void BattleAction::setTarget(const battle::Target & target_)
{
	target.clear();
	for(const auto & destination : target_)
	{
		if(destination.unitValue == nullptr)
			aimToHex(destination.hexValue);
		else
			aimToUnit(destination.unitValue);
	}
}

bool BattleAction::isUnitAction() const
{
	const auto * type = BattleActionType::find(*this);
	return type && type->isUnitAction();
}

bool BattleAction::isBattleEndAction() const
{
	const auto * type = BattleActionType::find(*this);
	return type && type->isBattleEndAction();
}

bool BattleAction::isTacticsAction() const
{
	const auto * type = BattleActionType::find(*this);
	return type && type->isTacticsAction();
}

std::ostream & operator<<(std::ostream & os, const BattleAction & ba)
{
	os << ba.toString();
	return os;
}
