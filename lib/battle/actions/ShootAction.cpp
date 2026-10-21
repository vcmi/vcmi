/*
 * ShootAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ShootAction.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../BattleAttackInfo.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../DamageEstimationTexts.h"
#include "../../bonuses/BonusSelector.h"
#include "../../spells/CSpell.h"
#include "../../spells/ISpellMechanics.h"
#include "../../spells/Problem.h"

static std::shared_ptr<const Bonus> getAreaShotAbility(const battle::Unit & shooter)
{
	return shooter.getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
}

static const CSpell * getAreaShotSpell(const battle::Unit & shooter)
{
	return getAreaShotAbility(shooter)->subtype.as<SpellID>().toSpell();
}

bool ShootAction::isUnitAction() const
{
	return true;
}

void ShootAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(battle.battleCanShoot(&actor))
		out.push_back({this, UnitActionButton{3, SpellID::NONE, ImagePath::builtin("battle/actionShoot"), "vcmi.battle.action.shoot"}});
}

int ShootAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	// an area shot at an empty hex is only chosen when no other option is legal there
	if(target == nullptr || !battle.battleMatchOwner(&actor, target) || !target->alive())
		return 100;

	return 4;
}

ActionPreview ShootAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}, {}};

	const battle::Unit & shooter = *context.actor;
	const int shotsLeft = shooter.acquireState()->shots.available();
	const std::string cursor = battle.battleHasShootingPenalty(&shooter, context.hoveredHex) ? "combatShootPenalty" : "combatShoot";
	const battle::Unit * target = battle.battleGetUnitByPos(context.hoveredHex, true);

	ActionPreview result{cursor, {}, {}, {}};

	if(!target)
	{
		const CSpell * spell = getAreaShotSpell(shooter);
		const DamageEstimation estimation = battle.estimateSpellLikeAttackDamage(&shooter, spell, context.hoveredHex);
		result.statusText = DamageEstimationTexts::rangedAttack(estimation, spell->getNameTextID(), shotsLeft);
	}
	else
	{
		DamageEstimation estimation = battle.battleEstimateDamage(BattleAttackInfo(&shooter, target, 0, true));
		vstd::amin(estimation.kills.max, target->getCount());
		vstd::amin(estimation.kills.min, target->getCount());
		result.statusText = DamageEstimationTexts::rangedAttack(estimation, *target, shotsLeft);
	}

	if(const auto ability = getAreaShotAbility(shooter); ability && ability->subtype.hasValue())
	{
		const CSpell * spell = ability->subtype.as<SpellID>().toSpell();
		spells::BattleCast cast(&battle, &shooter, spells::Mode::SPELL_LIKE_ATTACK, spell);
		cast.setSpellLevel(ability->val);
		addSpellArea(result, *spell->battleMechanics(&cast), context.hoveredHex);
	}

	return result;
}

std::optional<BattleAction> ShootAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return BattleAction::makeShotAttack(context.actor, context.hoveredHex);
}

bool ShootAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	const battle::Target target = action.getTarget(&battle);
	if(target.empty())
	{
		problem.add(MetaString::createFromRawString("Destination required for shot action."));
		return false;
	}

	// refuses a hex without a living, vulnerable enemy unless the shooter can target empty hexes
	const BattleHex destination = target.front().hexValue;
	if(!battle.battleCanShoot(unit, destination))
	{
		problem.add(MetaString::createFromRawString("Cannot shoot!"));
		return false;
	}

	const battle::Unit * destinationUnit = battle.battleGetUnitByPos(destination, true);
	if(battle.battleCanTargetEmptyHex(unit) && (!destinationUnit || destinationUnit->isInvincible()))
	{
		const CSpell * spell = getAreaShotSpell(*unit);
		spells::BattleCast cast(&battle, unit, spells::Mode::SPELL_LIKE_ATTACK, spell);
		spells::Target spellTarget;
		spellTarget.emplace_back(destination);

		if(!spell->battleMechanics(&cast)->canBeCastAt(spellTarget, problem))
			return false;
	}

	return true;
}

void ShootAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);

	env.rangedAttack(unit, action.getTarget(&battle).at(0).hexValue);
	env.triggerObstaclesUnder(unit);
}
