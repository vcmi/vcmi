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

VCMI_LIB_NAMESPACE_BEGIN

static const int32_t INVALID_UNIT_ID = -1000;

BattleAction::BattleAction():
	side(-1),
	stackNumber(-1),
	actionType(EActionType::INVALID),
	actionSubtype(-1)
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

BattleAction BattleAction::makeDefend(const battle::Unit * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::DEFEND;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeMeleeAttack(const battle::Unit * stack, BattleHex destination, BattleHex attackFrom, bool returnAfterAttack)
{
	BattleAction ba;
	ba.side = stack->unitSide(); //FIXME: will it fail if stack mind controlled?
	ba.actionType = EActionType::WALK_AND_ATTACK;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(attackFrom);
	ba.aimToHex(destination);
	if(returnAfterAttack && stack->hasBonusOfType(Bonus::RETURN_AFTER_STRIKE))
		ba.aimToHex(stack->getPosition());
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

BattleAction BattleAction::makeCreatureSpellcast(const battle::Unit * stack, const battle::Target & target, SpellID spellID)
{
	BattleAction ba;
	ba.actionType = EActionType::MONSTER_SPELL;
	ba.actionSubtype = spellID;
	ba.setTarget(target);
	ba.side = stack->unitSide();
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeMove(const battle::Unit * stack, BattleHex dest)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = EActionType::WALK;
	ba.stackNumber = stack->unitId();
	ba.aimToHex(dest);
	return ba;
}

BattleAction BattleAction::makeEndOFTacticPhase(ui8 side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::END_TACTIC_PHASE;
	return ba;
}

BattleAction BattleAction::makeSurrender(ui8 side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::SURRENDER;
	return ba;
}

BattleAction BattleAction::makeRetreat(ui8 side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = EActionType::RETREAT;
	return ba;
}

std::string BattleAction::toString() const
{
	std::stringstream actionTypeStream;
	actionTypeStream << actionType;

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
	fmt % static_cast<int>(side) % stackNumber % actionTypeStream.str() % actionSubtype % targetStream.str();
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

	for(auto & destination : target)
	{
		if(destination.unitValue == INVALID_UNIT_ID)
			ret.emplace_back(destination.hexValue);
		else
			ret.emplace_back(cb->battleGetUnitByID(destination.unitValue));
	}

	return ret;
}

void BattleAction::setTarget(const battle::Target & target_)
{
    target.clear();
	for(auto & destination : target_)
	{
		if(destination.unitValue == nullptr)
			aimToHex(destination.hexValue);
		else
			aimToUnit(destination.unitValue);
	}
}


std::ostream & operator<<(std::ostream & os, const BattleAction & ba)
{
	os << ba.toString();
	return os;
}

VCMI_LIB_NAMESPACE_END
