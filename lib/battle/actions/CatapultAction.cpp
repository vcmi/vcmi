/*
 * CatapultAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CatapultAction.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../Unit.h"
#include "../../bonuses/BonusSelector.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../spells/ISpellMechanics.h"
#include "../../spells/Problem.h"

static std::shared_ptr<const Bonus> getCatapultAbility(const battle::Unit & unit)
{
	return unit.getBonus(Selector::type()(BonusType::CATAPULT));
}

bool CatapultAction::isUnitAction() const
{
	return true;
}

void CatapultAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	const auto * siegedTown = battle.battleGetDefendedTown();
	if(!battle.battleTacticDist() && siegedTown && siegedTown->fortificationsLevel().wallsHealth > 0 && actor.hasBonusOfType(BonusType::CATAPULT))
		out.push_back({this, std::nullopt});
}

int CatapultAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 11;
}

ActionPreview CatapultAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}};

	return {"combatShootCatapult", {}, {}};
}

BattleAction CatapultAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	BattleAction action = BattleAction::makeCatapultShot(context.actor);
	action.aimToHex(context.hoveredHex);
	return action;
}

bool CatapultAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	const auto catapultAbility = getCatapultAbility(*unit);
	if(!catapultAbility || !catapultAbility->subtype.hasValue())
	{
		problem.add(MetaString::createFromRawString("This stack can't shoot at walls: " + unit->getDescription()));
		return false;
	}

	const battle::Target target = action.getTarget(&battle);
	const bool canHitWall = target.empty() ? !battle.getAttackableWallParts().empty() : battle.isWallPartAttackable(battle.battleHexToWallPart(target.front().hexValue));
	if(!canHitWall)
	{
		problem.add(MetaString::createFromRawString("No attackable wall part to shoot at."));
		return false;
	}

	return true;
}

void CatapultAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const auto catapultAbility = getCatapultAbility(unit);
	const CSpell * spell = catapultAbility->subtype.as<SpellID>().toSpell();
	const int shotLevel = unit.valOfBonuses(Selector::typeSubtype(BonusType::CATAPULT_EXTRA_SHOTS, catapultAbility->subtype));

	env.castSpell(unit, spell, spells::Mode::SPELL_LIKE_ATTACK, shotLevel, action.getTarget(&battle));
}
