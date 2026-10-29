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
#include "../DamageEstimationTexts.h"
#include "../Unit.h"
#include "../../CStack.h"
#include "../../spells/Problem.h"

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

BattleHexArray MeleeAttackAction::getAttackedHexes(const CBattleInfoCallback & battle, const battle::Unit & attacker, const BattleHex & destinationTile, const BattleHex & attackerPos) const
{
	BattleHexArray attackedHexes;

	// long weapon strikes from a non-adjacent hex, which has no attack direction, so the server applies no splash either
	if(usesLongWeapon())
		return attackedHexes;

	const AttackableTiles at = battle.getPotentiallyAttackableHexes(&attacker, destinationTile, attackerPos);

	for(const BattleHex & tile : at.hostileCreaturePositions)
	{
		const auto * st = battle.battleGetUnitByPos(tile, true);
		if(st && battle.battleGetOwner(st) != battle.battleGetOwner(&attacker) && !st->isInvincible()) //only hostile stacks - does it work well with Berserk?
			attackedHexes.insert(tile);
	}
	for(const BattleHex & tile : at.friendlyCreaturePositions)
	{
		const auto * st = battle.battleGetUnitByPos(tile, true);
		if(st && !st->isInvincible()) //friendly stacks can also be damaged by Dragon Breath
			attackedHexes.insert(tile);
	}
	return attackedHexes;
}

std::vector<ActionApproach> MeleeAttackAction::getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const
{
	const battle::Unit * defender = battle.battleGetUnitByPos(hoveredHex, true);
	if(!defender)
		return {};

	std::vector<ActionApproach> result = getReachableApproaches(battle, actor, hoveredHex, usesLongWeapon());
	std::erase_if(result, [&](const ActionApproach & approach){ return !canStrikeFrom(battle, actor, *defender, approach.from); });
	return result;
}

ActionPreview MeleeAttackAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}, {}};

	static const std::map<BattleHex::EDir, std::string> directionCursors = {
		{BattleHex::TOP_LEFT,     "combatHitSouthEast"},
		{BattleHex::TOP_RIGHT,    "combatHitSouthWest"},
		{BattleHex::RIGHT,        "combatHitWest"     },
		{BattleHex::BOTTOM_RIGHT, "combatHitNorthWest"},
		{BattleHex::BOTTOM_LEFT,  "combatHitNorthEast"},
		{BattleHex::LEFT,         "combatHitEast"     },
		{BattleHex::TOP,          "combatHitSouth"    },
		{BattleHex::BOTTOM,       "combatHitNorth"    }
	};

	const battle::Unit & attacker = *context.actor;
	const battle::Unit & target = *battle.battleGetUnitByPos(context.hoveredHex, true);
	const ActionApproach & approach = context.approach.value();

	const auto estimate = [&](const battle::Unit & defender, DamageEstimation * retaliation)
	{
		BattleAttackInfo attackInfo(&attacker, &defender, battle.battleGetDistances(&attacker, attacker.getPosition())[approach.from.toInt()], false);
		attackInfo.attackerPos = approach.from;

		DamageEstimation estimation = battle.battleEstimateDamage(attackInfo, retaliation);
		vstd::amin(estimation.kills.min, defender.getCount());
		vstd::amin(estimation.kills.max, defender.getCount());
		return estimation;
	};

	// an attack with a long weapon is not retaliated
	DamageEstimation retaliation{};
	DamageEstimation estimation = estimate(target, usesLongWeapon() ? nullptr : &retaliation);
	const bool targetMayBeKilled = estimation.kills.max == target.getCount();

	std::vector<uint32_t> affectedUnits;

	// breath and other multi-hex attacks also strike other units: their kills are added to the estimate
	for(const BattleHex & hex : getAttackedHexes(battle, attacker, context.hoveredHex, approach.from))
	{
		const battle::Unit * splashTarget = battle.battleGetUnitByPos(hex, true);
		if(!splashTarget || splashTarget == &target || splashTarget == &attacker || vstd::contains(affectedUnits, splashTarget->unitId()))
			continue;

		affectedUnits.push_back(splashTarget->unitId());

		const DamageEstimation splash = estimate(*splashTarget, nullptr);
		estimation.kills.min += splash.kills.min;
		estimation.kills.max += splash.kills.max;
	}

	MetaString text = DamageEstimationTexts::meleeAttack(estimation, target);
	text.appendEOL();
	text.append(DamageEstimationTexts::retaliation(retaliation, targetMayBeKilled));

	return {directionCursors.at(approach.direction), text, attacker.getHexes(approach.from), affectedUnits};
}

std::optional<BattleAction> MeleeAttackAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!context.approach)
		return std::nullopt;

	if(usesLongWeapon())
		return BattleAction::makeLongWeaponAttack(context.actor, context.hoveredHex, context.approach->from);

	return BattleAction::makeMeleeAttack(context.actor, context.hoveredHex, context.approach->from, returnsAfterStrike());
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
	const battle::Unit * defender = getDestinationUnit(battle, target.at(1));
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
		problem.add(MetaString::createFromRawString("Unit " + attacker->getDescription() + " can not attack this target: " + defender->getDescription()));
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
	const uint32_t defenderID = getDestinationUnit(battle, target.at(1))->unitId();

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

static const UnitActionButton attackButton{2, SpellID::NONE, ImagePath::builtin("battle/actionAttack"), "vcmi.battle.action.attack"};
static const UnitActionButton returnButton{1, SpellID::NONE, ImagePath::builtin("battle/actionReturn"), "vcmi.battle.action.return"};
static const UnitActionButton longWeaponButton{5, SpellID::NONE, ImagePath::builtin("battle/actionLongWeapon"), "vcmi.battle.action.attackLongWeapon"};

static bool offersMeleeAttack(const CBattleInfoCallback & battle, const battle::Unit & actor)
{
	return battle.battleTacticDist() == 0 && actor.isMeleeAttacker();
}

void WalkAndAttackAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(offersMeleeAttack(battle, actor))
		out.push_back({this, actor.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE) ? returnButton : attackButton});
}

int WalkAndAttackAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	// a returning unit prefers to return, a unit with a long weapon to use it
	return actor.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE) ? 5 : 7;
}

void WalkAndAttackNoReturnAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(offersMeleeAttack(battle, actor) && actor.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE))
		out.push_back({this, attackButton});
}

int WalkAndAttackNoReturnAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 7;
}

void LongWeaponAttackAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(offersMeleeAttack(battle, actor) && actor.hasBonusOfType(BonusType::LONG_WEAPON))
		out.push_back({this, longWeaponButton});
}

int LongWeaponAttackAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 6;
}
