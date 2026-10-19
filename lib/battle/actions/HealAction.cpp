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

bool HealAction::canHeal(const CBattleInfoCallback & battle, const battle::Unit & healer, const CSpell * spell, const battle::Unit & healed, spells::Problem & problem) const
{
	if(healed.unitSide() != healer.unitSide())
	{
		problem.add(MetaString::createFromRawString("Heal target is not a unit of the healer's side."));
		return false;
	}

	const spells::BattleCast cast(&battle, &healer, spells::Mode::SPELL_LIKE_ATTACK, spell);
	const auto mechanics = spell->battleMechanics(&cast);
	// canBeCastAt accepts immune units, on which H3 lets spells be wasted, but first aid must not waste the turn of the tent
	if(!mechanics->canBeCastAt({battle::Destination(&healed)}, problem) || !mechanics->isReceptive(&healed))
	{
		problem.add(MetaString::createFromRawString("Heal target can't be healed: " + healed.getDescription()));
		return false;
	}
	return true;
}

const battle::Unit * HealAction::findMostWounded(const CBattleInfoCallback & battle, const battle::Unit & healer, const CSpell * spell) const
{
	const battle::Unit * result = nullptr;
	int64_t mostLost = 0;
	for(const auto * unit : battle.battleGetUnitsIf([](const battle::Unit * u){ return u->alive(); }))
	{
		const int64_t lost = unit->getMaxHealth() - unit->getFirstHPleft();
		spells::detail::ProblemImpl ignored;
		if(lost > mostLost && canHeal(battle, healer, spell, *unit, ignored))
		{
			mostLost = lost;
			result = unit;
		}
	}
	return result;
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
		return true;

	const battle::Unit * healed = getDestinationUnit(battle, target.at(0));
	if(!healed)
	{
		problem.add(MetaString::createFromRawString("Heal target is not a unit."));
		return false;
	}

	return canHeal(battle, *unit, healerAbility->subtype.as<SpellID>().toSpell(), *healed, problem);
}

void HealAction::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const battle::Unit & unit = getActor(battle, action);
	const battle::Target target = action.getTarget(&battle);
	const CSpell * spell = getHealerAbility(unit)->subtype.as<SpellID>().toSpell();

	if(!target.empty())
	{
		env.castSpell(unit, spell, spells::Mode::SPELL_LIKE_ATTACK, 0, {battle::Destination(getDestinationUnit(battle, target.at(0)), target.at(0).hexValue)});
		return;
	}

	const battle::Unit * healed = findMostWounded(battle, unit, spell);
	if(healed)
		env.castSpell(unit, spell, spells::Mode::SPELL_LIKE_ATTACK, 0, {battle::Destination(healed)});
}
