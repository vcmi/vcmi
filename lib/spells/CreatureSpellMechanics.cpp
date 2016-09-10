/*
 * CreatureSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CreatureSpellMechanics.h"

#include "../NetPacks.h"
#include "../BattleState.h"

///AcidBreathDamageMechanics
void AcidBreathDamageMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//todo: this should be effectValue
	//calculating dmg to display
	ctx.setDamageToDisplay(parameters.effectPower);

	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = owner->id;
		bsa.damageAmount = parameters.effectPower; //damage times the number of attackers
		bsa.stackAttacked = (attackedCre)->ID;
		bsa.attackerID = -1;
		(attackedCre)->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);
	}
}

ESpellCastProblem::ESpellCastProblem AcidBreathDamageMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//just in case
	if(!obj->alive())
		return ESpellCastProblem::WRONG_SPELL_TARGET;

	//there should be no immunities by design
	//but make it a bit configurable
	//ignore all immunities, except specific absolute immunity
	{
		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << owner->id.toEnum() << "addInfo_1";
		if(obj->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, owner->id.toEnum(), 1), cachingStr.str()))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	return ESpellCastProblem::OK;
}

///DeathStareMechanics
void DeathStareMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//calculating dmg to display
	si32 damageToDisplay = parameters.effectPower;

	if(!ctx.attackedCres.empty())
		vstd::amin(damageToDisplay, (*ctx.attackedCres.begin())->count); //stack is already reduced after attack

	ctx.setDamageToDisplay(damageToDisplay);

	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = owner->id;
		bsa.damageAmount = parameters.effectPower * (attackedCre)->valOfBonuses(Bonus::STACK_HEALTH);//todo: move here all DeathStare calculation
		bsa.stackAttacked = (attackedCre)->ID;
		bsa.attackerID = -1;
		(attackedCre)->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);
	}
}

///DispellHelpfulMechanics
void DispellHelpfulMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);

	doDispell(battle, packet, Selector::positiveSpellEffects);
}

ESpellCastProblem::ESpellCastProblem DispellHelpfulMechanics::isImmuneByStack(const ISpellCaster * caster,  const CStack * obj) const
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
