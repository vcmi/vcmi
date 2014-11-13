/*
 * SpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SpellMechanics.h"

#include "mapObjects/CGHeroInstance.h"
#include "BattleState.h"

#include "NetPacks.h"

///DefaultSpellMechanics

std::set<const CStack *> DefaultSpellMechanics::getAffectedStacks(SpellTargetingContext & ctx) const
{
	
}


ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	//by default use general algorithm
	return owner->isImmuneBy(obj);
}

bool DefaultSpellMechanics::adventureCast(SpellCastContext& context) const
{
	return false; //there is no general algorithm for castind adventure spells
}

bool DefaultSpellMechanics::battleCast(SpellCastContext& context) const
{
	return false; //todo; DefaultSpellMechanics::battleCast
}

///OffenciveSpellMechnics
bool OffenciveSpellMechnics::battleCast(SpellCastContext& context) const
{
	assert(owner->isOffensiveSpell());
	
	//todo:OffenciveSpellMechnics::battleCast
}


///CloneMechanics
ESpellCastProblem::ESpellCastProblem CloneMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack * obj) const
{
	//can't clone already cloned creature
	if (vstd::contains(obj->state, EBattleStackState::CLONED))
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	//TODO: how about stacks casting Clone?
	//currently Clone casted by stack is assumed Expert level
	ui8 schoolLevel;
	if (caster)
	{
		schoolLevel = caster->getSpellSchoolLevel(owner);
	}
	else
	{
		schoolLevel = 3;
	}

	if (schoolLevel < 3)
	{
		int maxLevel = (std::max(schoolLevel, (ui8)1) + 4);
		int creLevel = obj->getCreature()->level;
		if (maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, any level for expert
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	//use default algorithm only if there is no mechanics-related problem		
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

///DispellHelpfulMechanics
ESpellCastProblem::ESpellCastProblem DispellHelpfulMechanics::isImmuneByStack(const CGHeroInstance* caster,  const CStack* obj) const
{
	TBonusListPtr spellBon = obj->getSpellBonuses();
	bool hasPositiveSpell = false;
	for(const Bonus * b : *spellBon)
	{
		if(SpellID(b->sid).toSpell()->isPositive())
		{
			hasPositiveSpell = true;
			break;
		}
	}
	if(!hasPositiveSpell)
	{
		return ESpellCastProblem::NO_SPELLS_TO_DISPEL;
	}
	
	//use default algorithm only if there is no mechanics-related problem		
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

///HypnotizeMechanics
ESpellCastProblem::ESpellCastProblem HypnotizeMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack* obj) const
{
	if(nullptr != caster) //do not resist hypnotize casted after attack, for example
	{
		//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
		ui64 subjectHealth = (obj->count - 1) * obj->MaxHealth() + obj->firstHPleft;
		//apply 'damage' bonus for hypnotize, including hero specialty
		ui64 maxHealth = owner->calculateBonus(caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER)
			* owner->power + owner->getPower(caster->getSpellSchoolLevel(owner)), caster, obj);
		if (subjectHealth > maxHealth)
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}			
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);
}


///SpecialRisingSpellMechanics
ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack* obj) const
{
	// following does apply to resurrect and animate dead(?) only
	// for sacrifice health calculation and health limit check don't matter

	if(obj->count >= obj->baseAmount)
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	
	if (caster) //FIXME: Archangels can cast immune stack
	{
		auto maxHealth = owner->calculateHealedHP (caster, obj);
		if (maxHealth < obj->MaxHealth()) //must be able to rise at least one full creature
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}	
	
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

	

