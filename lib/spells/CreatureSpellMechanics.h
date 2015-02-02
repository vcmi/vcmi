/*
 * CreatureSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
 #pragma once
 
 #include "CDefaultSpellMechanics.h"

class AcidBreathDamageMechanics: public DefaultSpellMechanics
{
public:
	AcidBreathDamageMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};

class DeathStareMechanics: public DefaultSpellMechanics
{
public:
	DeathStareMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};

class DispellHelpfulMechanics: public DefaultSpellMechanics
{
public:
	DispellHelpfulMechanics(CSpell * s): DefaultSpellMechanics(s){};
	
	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override;
	
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;	
};
