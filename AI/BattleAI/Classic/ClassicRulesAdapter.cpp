/*
 * ClassicRulesAdapter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicRulesAdapter.h"

#include "../../../lib/battle/Unit.h"
#include "../../../lib/CStack.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusParameters.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "ClassicBattleStateView.h"

namespace
{
bool isArrowTower(const battle::Unit * unit)
{
	return unit && unit->creatureId() == CreatureID(CreatureID::ARROW_TOWERS);
}
}

int32_t ClassicRulesAdapter::attack(const battle::Unit * unit, bool ranged)
{
	// VCMI deliberately forces tower attack to zero because its combat script
	// applies tower damage independently of attack/defence. The SoD tactical AI
	// reads the CRTRAITS-facing value instead.
	return isArrowTower(unit) ? 10 : unit->getAttack(ranged);
}

int32_t ClassicRulesAdapter::defense(const battle::Unit * unit)
{
	return isArrowTower(unit) ? 5 : unit->getDefense(false);
}

int32_t ClassicRulesAdapter::minDamage(const battle::Unit * unit, bool ranged)
{
	return isArrowTower(unit) ? 2 : unit->getMinDamage(ranged);
}

int32_t ClassicRulesAdapter::maxDamage(const battle::Unit * unit, bool ranged)
{
	return isArrowTower(unit) ? 4 : unit->getMaxDamage(ranged);
}

uint32_t ClassicRulesAdapter::movementRangeAfterBindCleanup(
	const CBattleInfoCallback & battle,
	const battle::Unit * unit)
{
	if(!unit->hasBonusOfType(BonusType::BIND_EFFECT))
		return unit->getMovementRange();
	if(unit->hasBonusOfType(BonusType::SIEGE_WEAPON))
		return 0;
	const battle::Unit * activeUnit = battle.battleActiveUnit();
	if(activeUnit && activeUnit->unitId() == unit->unitId())
		return unit->getMovementRange();

	const battle::Units adjacent = battle.battleAdjacentUnits(unit);
	for(const auto & bind : *unit->getBonusesOfType(BonusType::BIND_EFFECT))
	{
		if(!bind->parameters)
			return 0;
		const battle::Unit * binder = battle.battleGetUnitByID(bind->parameters->toNumber());
		if(binder && binder->alive() && vstd::contains(adjacent, binder))
			return 0;
	}
	return std::max(0, unit->getInitiative(0));
}

bool ClassicRulesAdapter::failedSiege(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	BattleSide side)
{
	const CGTownInstance * town = battle->battleGetDefendedTown();
	if(side != BattleSide::ATTACKER
	   || !town
	   || town->fortLevel() != CGTownInstance::CASTLE)
		return false;

	constexpr std::array<EWallPart, 4> BREACH_PARTS = {
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::BOTTOM_WALL,
		EWallPart::UPPER_WALL
	};
	for(EWallPart part : BREACH_PARTS)
	{
		if(battle->battleGetWallState(part) <= EWallState::DESTROYED)
			return false;
	}

	ClassicBattleStateView view(battle);
	for(const CStack * stack : view.orderedStacks())
	{
		// The original immobilized-creature flag excludes siege objects.
		// Temporary Bind is unrelated to this creature flag.
		if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON))
			continue;
		if(stack->unitSide() == BattleSide::ATTACKER)
		{
			if(stack->isShooter()
			   || stack->hasBonusOfType(BonusType::FLYING)
			   || battle->battleCanShoot(stack))
				return false;
		}
		else if(!battle->battleIsInsideWalls(stack->getPosition()))
			return false;
	}
	return true;
}
