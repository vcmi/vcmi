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

#include "ISpellMechanics.h"
#include "CDefaultSpellMechanics.h"

class DLL_LINKAGE AcidBreathDamageMechanics : public DefaultSpellMechanics
{
public:
	AcidBreathDamageMechanics(CSpell * s): DefaultSpellMechanics(s){};

	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;

protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DeathStareMechanics : public DefaultSpellMechanics
{
public:
	DeathStareMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DispellHelpfulMechanics : public DefaultSpellMechanics
{
public:
	DispellHelpfulMechanics(CSpell * s): DefaultSpellMechanics(s){};

	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override final;

	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
private:
	 static bool positiveSpellEffects(const Bonus * b);
};
