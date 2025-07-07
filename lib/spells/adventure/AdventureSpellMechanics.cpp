/*
 * AdventureSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureSpellMechanics.h"

#include "../CSpellHandler.h"
#include "../Problem.h"

#include "../../mapObjects/CGHeroInstance.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

AdventureSpellMechanics::AdventureSpellMechanics(const CSpell * s)
	: IAdventureSpellMechanics(s)
{
}

bool AdventureSpellMechanics::canBeCast(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!owner->isAdventure())
		return false;

	const auto * heroCaster = dynamic_cast<const CGHeroInstance *>(caster);

	if(heroCaster)
	{
		if(heroCaster->isGarrisoned())
			return false;

		const auto level = heroCaster->getSpellSchoolLevel(owner);
		const auto cost = owner->getCost(level);

		if(!heroCaster->canCastThisSpell(owner))
			return false;

		if(heroCaster->mana < cost)
			return false;
	}

	return canBeCastImpl(problem, cb, caster);
}

bool AdventureSpellMechanics::canBeCastAt(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return canBeCast(problem, cb, caster) && canBeCastAtImpl(problem, cb, caster, pos);
}

bool AdventureSpellMechanics::canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
{
	return true;
}

bool AdventureSpellMechanics::canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return true;
}

bool AdventureSpellMechanics::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	spells::detail::ProblemImpl problem;

	if(!canBeCastAt(problem, env->getCb(), parameters.caster, parameters.pos))
		return false;

	ESpellCastResult result = beginCast(env, parameters);

	if(result == ESpellCastResult::OK)
		performCast(env, parameters);

	return result != ESpellCastResult::ERROR;
}

ESpellCastResult AdventureSpellMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(owner->hasEffects())
	{
		//todo: cumulative effects support
		const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

		std::vector<Bonus> bonuses;

		owner->getEffects(bonuses, schoolLevel, false, parameters.caster->getEnchantPower(owner));

		for(const Bonus & b : bonuses)
		{
			GiveBonus gb;
			gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
			gb.bonus = b;
			env->apply(gb);
		}

		return ESpellCastResult::OK;
	}
	else
	{
		//There is no generic algorithm of adventure cast
		env->complain("Unimplemented adventure spell");
		return ESpellCastResult::ERROR;
	}
}

ESpellCastResult AdventureSpellMechanics::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	return ESpellCastResult::OK;
}

void AdventureSpellMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	// no-op, only for implementation in derived classes
}

void AdventureSpellMechanics::performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto level = parameters.caster->getSpellSchoolLevel(owner);
	const auto cost = owner->getCost(level);

	AdvmapSpellCast asc;
	asc.casterID = ObjectInstanceID(parameters.caster->getCasterUnitId());
	asc.spellID = owner->id;
	env->apply(asc);

	ESpellCastResult result = applyAdventureEffects(env, parameters);

	switch(result)
	{
		case ESpellCastResult::OK:
			parameters.caster->spendMana(env, cost);
			endCast(env, parameters);
			break;
		default:
			break;
	}
}

VCMI_LIB_NAMESPACE_END
