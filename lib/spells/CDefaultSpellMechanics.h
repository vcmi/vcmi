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

class DLL_LINKAGE SpellCastContext
{
public:
	const DefaultSpellMechanics * mechanics;
	const SpellCastEnvironment * env;
	std::vector<const CStack *> attackedCres;//must be vector, as in Chain Lightning order matters
	BattleSpellCast sc;//todo: make private
	StacksInjured si;
	const BattleSpellCastParameters & parameters;

	SpellCastContext(const DefaultSpellMechanics * mechanics_, const SpellCastEnvironment * env_, const BattleSpellCastParameters & parameters_);
	virtual ~SpellCastContext();

	void addDamageToDisplay(const si32 value);
	void setDamageToDisplay(const si32 value);

	void beforeCast();
	void afterCast();
private:
	const CGHeroInstance * otherHero;
	int spellCost;
	si32 damageToDisplay;

	void prepareBattleLog();
};

///all combat spells
class DLL_LINKAGE DefaultSpellMechanics : public ISpellMechanics
{
public:
	DefaultSpellMechanics(CSpell * s): ISpellMechanics(s){};

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr) const override;
	std::vector<const CStack *> getAffectedStacks(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override final;

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;

	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;

	virtual void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override;

	void battleCast(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters) const override final;

	void battleLog(std::vector<MetaString> & logLines, const BattleSpellCastParameters & parameters,
		const std::vector<const CStack *> & attacked, const si32 damageToDisplay, bool & displayDamage) const;

	void battleLogDefault(std::vector<MetaString> & logLines, const BattleSpellCastParameters & parameters,
		const std::vector<const CStack *> & attacked) const;

	bool requiresCreatureTarget() const	override;

protected:
	virtual void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const;

	virtual std::vector<const CStack *> calculateAffectedStacks(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const;

protected:
	void doDispell(BattleInfo * battle, const BattleSpellCast * packet, const CSelector & selector) const;
	bool canDispell(const IBonusBearer * obj, const CSelector & selector, const std::string &cachingStr = "") const;
private:
	void cast(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, std::vector <const CStack*> & reflected) const;

	void handleImmunities(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx, std::vector<const CStack *> & stacks) const;
	void handleMagicMirror(const SpellCastEnvironment * env, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const;
	void handleResistance(const SpellCastEnvironment * env, SpellCastContext & ctx) const;

	static bool dispellSelector(const Bonus * bonus);

	friend class SpellCastContext;
};

///not affecting creatures directly
class DLL_LINKAGE SpecialSpellMechanics : public DefaultSpellMechanics
{
public:
	SpecialSpellMechanics(CSpell * s): DefaultSpellMechanics(s){};

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
protected:
	std::vector<const CStack *> calculateAffectedStacks(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
};
