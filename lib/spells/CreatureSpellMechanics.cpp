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
void AcidBreathDamageMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//calculating dmg to display
	ctx.sc.dmgToDisplay = parameters.usedSpellPower;

	for(auto & attackedCre : ctx.attackedCres) //no immunities
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = owner->id;
		bsa.damageAmount = parameters.usedSpellPower; //damage times the number of attackers
		bsa.stackAttacked = (attackedCre)->ID;
		bsa.attackerID = -1;
		(attackedCre)->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);
	}
}

///DeathStareMechanics
void DeathStareMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//calculating dmg to display
	ctx.sc.dmgToDisplay = parameters.usedSpellPower;
	if(!ctx.attackedCres.empty())
		vstd::amin(ctx.sc.dmgToDisplay, (*ctx.attackedCres.begin())->count); //stack is already reduced after attack

	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = owner->id;
		bsa.damageAmount = parameters.usedSpellPower * (attackedCre)->valOfBonuses(Bonus::STACK_HEALTH);
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

ESpellCastProblem::ESpellCastProblem DispellHelpfulMechanics::isImmuneByStack(const CGHeroInstance * caster,  const CStack * obj) const
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
