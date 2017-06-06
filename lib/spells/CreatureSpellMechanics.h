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
	AcidBreathDamageMechanics(const CSpell * s);
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DeathStareMechanics : public DefaultSpellMechanics
{
public:
	DeathStareMechanics(const CSpell * s);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DispellHelpfulMechanics : public DefaultSpellMechanics
{
public:
	DispellHelpfulMechanics(const CSpell * s);
	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override final;
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
private:
	static bool positiveSpellEffects(const Bonus * b);
};
