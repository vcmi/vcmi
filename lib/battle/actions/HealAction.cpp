/*
 * HealAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HealAction.h"

#include "IBattleActionEnvironment.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../DamageEstimationTexts.h"
#include "../Unit.h"
#include "../../bonuses/BonusSelector.h"
#include "../../spells/CSpell.h"
#include "../../spells/ISpellMechanics.h"
#include "../../spells/Problem.h"

static std::shared_ptr<const Bonus> getHealerAbility(const battle::Unit & unit)
{
	return unit.getBonus(Selector::type()(BonusType::HEALER));
}

bool HealAction::isUnitAction() const
{
	return true;
}

void HealAction::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	if(!battle.battleTacticDist() && actor.hasBonusOfType(BonusType::HEALER))
		out.push_back({this, std::nullopt});
}

int HealAction::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	return 12;
}

ActionPreview HealAction::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	if(!isLegal(game, battle, option, context))
		return {"combatBlocked", {}, {}, {}};

	const battle::Unit * target = battle.battleGetUnitByPos(context.hoveredHex);
	const int64_t healed = battle.getFirstAidHealValue(battle.battleGetOwnerHero(context.actor), target);

	MetaString text = MetaString::createFromTextID("core.genrltxt.419"); // Apply first aid to the %s
	target->addNameReplacement(text);
	if(healed > 0)
	{
		text.appendRawString(" (");
		text.append(DamageEstimationTexts::healthGain(healed, "vcmi.battleWindow.healValuePreview"));
		text.appendRawString(")");
	}

	return {"combatHeal", text, {}, {}};
}

std::optional<BattleAction> HealAction::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return BattleAction::makeHeal(context.actor, context.hoveredHex);
}

bool HealAction::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	const auto healerAbility = getHealerAbility(*unit);
	if(!healerAbility || !healerAbility->subtype.hasValue())
	{
		problem.add(MetaString::createFromRawString("This stack can't heal: " + unit->getDescription()));
		return false;
	}

	const battle::Target target = action.getTarget(&battle);
	if(target.empty())
	{
		problem.add(MetaString::createFromRawString("Destination required for heal action."));
		return false;
	}

	const battle::Unit * healed = getDestinationUnit(battle, target.at(0));
	if(!healed || healed->unitSide() != unit->unitSide())
	{
		problem.add(MetaString::createFromRawString("Heal target is not a unit of the healer's side."));
		return false;
	}

	const CSpell * spell = healerAbility->subtype.as<SpellID>().toSpell();
	const spells::BattleCast cast(&battle, unit, spells::Mode::SPELL_LIKE_ATTACK, spell);
	if(!spell->battleMechanics(&cast)->canBeCastAt({battle::Destination(healed)}, problem))
	{
		problem.add(MetaString::createFromRawString("Heal target can't be healed: " + healed->getDescription()));
		return false;
	}

	return true;
}

void HealAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);
	const CSpell * spell = getHealerAbility(unit)->subtype.as<SpellID>().toSpell();

	env.castSpell(unit, spell, spells::Mode::SPELL_LIKE_ATTACK, 0, {battle::Destination(getDestinationUnit(battle, target.at(0)), target.at(0).hexValue)});
}
