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
	actionType(BattleActionID::SKIP_TURN)
{
}

BattleAction BattleAction::makeUnitAction(const battle::Unit * unit, BattleActionID type)
{
	BattleAction ba;
	ba.side = unit->unitSide();
	ba.stackNumber = unit->unitId();
	ba.actionType = type;
	return ba;
}

BattleAction BattleAction::makeSideAction(BattleSide side, BattleActionID type)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = type;
	return ba;
}

BattleAction BattleAction::makeHeal(const battle::Unit * healer, const battle::Unit * healed)
{
	BattleAction ba = makeUnitAction(healer, BattleActionID::HEAL);
	ba.aimToUnit(healed);
	return ba;
}

BattleAction BattleAction::makeHeal(const battle::Unit * healer)
{
	return makeUnitAction(healer, BattleActionID::HEAL);
}

BattleAction BattleAction::makeCatapultShot(const battle::Unit * catapult)
{
	return makeUnitAction(catapult, BattleActionID::CATAPULT);
}

BattleAction BattleAction::makeSkipTurn(const battle::Unit * stack)
{
	return makeUnitAction(stack, BattleActionID::SKIP_TURN);
}

BattleAction BattleAction::makeBadMorale(const battle::Unit * stack)
{
	return makeUnitAction(stack, BattleActionID::BAD_MORALE);
}

BattleAction BattleAction::makeDefend(const battle::Unit * stack)
{
	return makeUnitAction(stack, BattleActionID::DEFEND);
}

BattleAction BattleAction::makeWait(const battle::Unit * stack)
{
	return makeUnitAction(stack, BattleActionID::WAIT);
}

BattleAction BattleAction::makeMeleeAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::MELEE_ATTACK);
	ba.aimToHex(attackFrom);
	ba.aimToHex(destination);
	return ba;
}

/// This is for cases where target hex does not matter (for example berserk spell) and we did not calculate it (for example we only calculated closest attackable hex of two hex unit)
BattleAction BattleAction::makeMeleeAttack(const battle::Unit * stack, const battle::Unit * target, const BattleHex & attackFrom)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::MELEE_ATTACK);
	ba.aimToHex(attackFrom);
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeLongWeaponAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::LONG_WEAPON_ATTACK);
	ba.aimToHex(attackFrom);
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeRangedAttack(const battle::Unit * shooter, const battle::Unit * target)
{
	BattleAction ba = makeUnitAction(shooter, BattleActionID::RANGED_ATTACK);
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeRangedAttack(const battle::Unit * shooter, const BattleHex & destination)
{
	BattleAction ba = makeUnitAction(shooter, BattleActionID::RANGED_ATTACK);
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeUnitSpell(const battle::Unit * stack, const battle::Target & target, const SpellID & spellID)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::UNIT_SPELL);
	ba.spell = spellID;
	ba.setTarget(target);
	return ba;
}

BattleAction BattleAction::makeHeroSpell(BattleSide side, const battle::Target & target, const SpellID & spellID)
{
	BattleAction ba = makeSideAction(side, BattleActionID::HERO_SPELL);
	ba.spell = spellID;
	ba.setTarget(target);
	return ba;
}

BattleAction BattleAction::makeRandomSpell(const battle::Unit * stack, const BattleHex & destination)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::RANDOM_SPELL);
	ba.aimToHex(destination);
	return ba;
}

BattleAction BattleAction::makeAdjacentSpell(const battle::Unit * stack, const BattleHex & castFrom, const battle::Unit * target, const SpellID & spellID)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::ADJACENT_SPELL);
	ba.spell = spellID;
	ba.aimToHex(castFrom);
	ba.aimToUnit(target);
	return ba;
}

BattleAction BattleAction::makeMove(const battle::Unit * stack, const BattleHex & dest)
{
	BattleAction ba = makeUnitAction(stack, BattleActionID::MOVE);
	ba.aimToHex(dest);
	return ba;
}

BattleAction BattleAction::makeEndTacticsPhase(BattleSide side)
{
	return makeSideAction(side, BattleActionID::END_TACTICS_PHASE);
}

BattleAction BattleAction::makeSurrender(BattleSide side)
{
	return makeSideAction(side, BattleActionID::SURRENDER);
}

BattleAction BattleAction::makeRetreat(BattleSide side)
{
	return makeSideAction(side, BattleActionID::RETREAT);
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
