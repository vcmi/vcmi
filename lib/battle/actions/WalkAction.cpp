/*
 * WalkAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WalkAction.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../Unit.h"
#include "../../spells/Problem.h"

bool WalkAction::isUnitAction() const
{
	return true;
}

bool WalkAction::isTacticsAction() const
{
	return true;
}

void WalkAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(battle.battleTacticDist() > 0)
		out.push_back({this, std::nullopt});
	else if(actor.canMove() && actor.getMovementRange(0)) //probably no reason to try move war machines or bound stacks
		out.push_back({this, UnitActionButton{0, SpellID::NONE, ImagePath::builtin("battle/actionMove"), "vcmi.battle.action.move"}});
}

int WalkAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 10;
}

bool WalkAction::isLegal(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	// the hexes of the actor itself are valid destinations, but hovering the actor or any other unit selects another option
	if(battle.battleGetUnitByPos(context.hoveredHex, true))
		return false;

	return BattleActionType::isLegal(game, battle, option, context);
}

ActionPreview WalkAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}};

	const bool flying = context.actor->hasBonusOfType(BonusType::FLYING);

	MetaString text = MetaString::createFromTextID(flying ? "core.genrltxt.295" : "core.genrltxt.294"); // Fly %s here / Move %s here
	context.actor->addNameReplacement(text);

	return {flying ? "combatFly" : "combatMove", text, context.actor->getHexes(battle.toWhichHexMove(context.actor, context.hoveredHex))};
}

BattleAction WalkAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return BattleAction::makeMove(context.actor, battle.toWhichHexMove(context.actor, context.hoveredHex));
}

bool WalkAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	const battle::Target target = action.getTarget(&battle);
	if(target.empty())
	{
		problem.add(MetaString::createFromRawString("Destination required for move action."));
		return false;
	}

	if(!battle.toWhichHexMove(unit, target.front().hexValue).isValid())
	{
		problem.add(MetaString::createFromRawString("Given destination is not reachable!"));
		return false;
	}

	return true;
}

void WalkAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);

	env.moveUnit(unit, battle.toWhichHexMove(&unit, action.getTarget(&battle).at(0).hexValue));
}
