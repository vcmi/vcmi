/*
 * CDefaultSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"

struct StacksInjured;

struct SpellCastContext
{
	SpellCastContext(std::vector<const CStack *> & attackedCres, BattleSpellCast & sc, StacksInjured & si):
		attackedCres(attackedCres), sc(sc), si(si)
	{
	};
	std::vector<const CStack *> & attackedCres;
	BattleSpellCast & sc;
	StacksInjured & si;
};

class DLL_LINKAGE DefaultSpellMechanics : public ISpellMechanics
{
public:
	DefaultSpellMechanics(CSpell * s): ISpellMechanics(s){};

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr) const override;
	std::set<const CStack *> getAffectedStacks(const CBattleInfoCallback * cb, SpellTargetingContext & ctx) const override;

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const override;
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;

	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;

	virtual void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override;

	void battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const override final;

	void battleLogSingleTarget(std::vector<std::string> & logLines, const BattleSpellCast * packet,
		const std::string & casterName, const CStack * attackedStack, bool & displayDamage) const override;

	bool requiresCreatureTarget() const	override;
protected:
	virtual void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const;

	void doDispell(BattleInfo * battle, const BattleSpellCast * packet, const CSelector & selector) const;
private:
	void castMagicMirror(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const;
	void handleResistance(const SpellCastEnvironment * env, std::vector<const CStack*> & attackedCres, BattleSpellCast & sc) const;
	void prepareBattleCast(const BattleSpellCastParameters & parameters, BattleSpellCast & sc) const;
};
