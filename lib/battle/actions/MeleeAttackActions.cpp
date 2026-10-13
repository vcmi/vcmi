/*
 * MeleeAttackActions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MeleeAttackActions.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../Unit.h"
#include "../../spells/Problem.h"

static const battle::Unit * getDefender(const CBattleInfoCallback & battle, const battle::Destination & destination)
{
	if(destination.unitValue)
		return destination.unitValue;

	return battle.battleGetUnitByPos(destination.hexValue, true);
}

bool MeleeAttackAction::isUnitAction() const
{
	return true;
}

bool MeleeAttackAction::usesLongWeapon() const
{
	return false;
}

bool MeleeAttackAction::returnsAfterStrike() const
{
	return false;
}

bool MeleeAttackAction::canStrikeFrom(const CBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & attackerPos) const
{
	if(usesLongWeapon())
		return battle.isLongWeaponAttack(&attacker, &defender, attackerPos);

	return battle.isMeleeAttackPossible(&attacker, &defender, attackerPos);
}

bool MeleeAttackAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * attacker = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, attacker, problem))
		return false;

	const battle::Target target = action.getTarget(&battle);
	if(target.size() < 2)
	{
		problem.add(MetaString::createFromRawString("Two destinations required for attack action."));
		return false;
	}

	const BattleHex attackFrom = target.at(0).hexValue;
	const battle::Unit * defender = getDefender(battle, target.at(1));
	if(!defender)
	{
		problem.add(MetaString::createFromRawString("Invalid target to attack"));
		return false;
	}

	// a berserk unit attacks the nearest unit of either side
	const bool canAttackDefender = attacker->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)
		? defender != attacker && defender->alive() && !defender->isInvincible()
		: battle.battleCanAttackUnit(attacker, defender);

	if(!canAttackDefender)
	{
		problem.add(MetaString::createFromRawString("Unit can not attack this target: " + defender->getDescription()));
		return false;
	}

	// obstacles on the way are not checked: a unit stopped by one, such as a hidden mine, has still made its action
	if(!battle.battleGetAvailableHexes(attacker, false).contains(attackFrom))
	{
		problem.add(MetaString::createFromRawString("Stack failed attack - unable to reach target!"));
		return false;
	}

	if(!canStrikeFrom(battle, *attacker, *defender, attackFrom))
	{
		problem.add(MetaString::createFromRawString("Attack cannot be performed!"));
		return false;
	}

	return true;
}

void MeleeAttackAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & attacker = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);
	const BattleHex targetHex = target.at(1).hexValue;
	const uint32_t defenderID = getDefender(battle, target.at(1))->unitId();

	const BattleHex startPosition = attacker.getPosition();
	const int speedBeforeAttack = attacker.getMovementRange(0);

	const UnitMoveResult movement = env.moveUnit(attacker, target.at(0).hexValue);

	// the unit stepped on quicksand or a mine: the attack is cancelled, and the action counts as made
	if(!movement.completed)
		return;

	// handlers of the movement events may have killed, removed or displaced either unit
	const battle::Unit * defender = battle.battleGetUnitByID(defenderID);
	if(!attacker.alive() || !defender || !defender->alive() || !canStrikeFrom(battle, attacker, *defender, attacker.getPosition()))
		return;

	env.meleeAttack(attacker, *defender, targetHex, movement.distance, usesLongWeapon());

	if(returnsAfterStrike()
		&& attacker.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE)
		&& !attacker.hasBonusOfType(BonusType::NOT_ACTIVE)
		&& !attacker.hasBonusOfType(BonusType::BIND_EFFECT)
		&& startPosition != attacker.getPosition()
		&& attacker.alive())
	{
		// speed lost during the attack shortens the way back
		const int speedAfterAttack = attacker.getMovementRange(0);
		const auto path = battle.getPath(attacker.getPosition(), startPosition, &attacker).first;
		const size_t maxReachableIndex = std::max(0, speedBeforeAttack - speedAfterAttack);
		if(maxReachableIndex < path.size())
			env.moveUnit(attacker, path[maxReachableIndex]);
	}

	// attacking without moving still triggers the obstacle the unit stands on (e.g. moat damage);
	// units that moved into the obstacle were already charged during the movement above
	if(movement.distance == 0)
		env.triggerObstaclesUnder(attacker);
}

bool WalkAndAttackAction::returnsAfterStrike() const
{
	return true;
}

bool LongWeaponAttackAction::usesLongWeapon() const
{
	return true;
}
