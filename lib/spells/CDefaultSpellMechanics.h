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
#include "../NetPacks.h"

class DefaultSpellMechanics;

class SpellCastContext
{
public:
	const DefaultSpellMechanics * mechanics;
	std::vector<const CStack *> attackedCres;//must be vector, as in Chain Lightning order matters
	BattleSpellCast sc;//todo: make private
	StacksInjured si;

	SpellCastContext(const DefaultSpellMechanics * mechanics_, const BattleSpellCastParameters & parameters);

	void addDamageToDisplay(const si32 value);
	void setDamageToDisplay(const si32 value);

	void sendCastPacket(const SpellCastEnvironment * env);
private:
	void prepareBattleCast(const BattleSpellCastParameters & parameters);
};

///all combat spells
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

	friend class SpellCastContext;
};
