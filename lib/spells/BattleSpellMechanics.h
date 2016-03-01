/*
 * BattleSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CDefaultSpellMechanics.h"

class DLL_LINKAGE HealingSpellMechanics : public DefaultSpellMechanics
{
public:
	enum class EHealLevel
	{
		HEAL,
		RESURRECT,
		TRUE_RESURRECT
	};

	HealingSpellMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
	virtual int calculateHealedHP(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const;
	virtual EHealLevel getHealLevel(int effectLevel) const = 0;
};

class DLL_LINKAGE AntimagicMechanics : public DefaultSpellMechanics
{
public:
	AntimagicMechanics(CSpell * s): DefaultSpellMechanics(s){};

	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override final;
};

class DLL_LINKAGE ChainLightningMechanics : public DefaultSpellMechanics
{
public:
	ChainLightningMechanics(CSpell * s): DefaultSpellMechanics(s){};
	std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const override;
};

class DLL_LINKAGE CloneMechanics : public DefaultSpellMechanics
{
public:
	CloneMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE CureMechanics : public HealingSpellMechanics
{
public:
	CureMechanics(CSpell * s): HealingSpellMechanics(s){};

	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override final;

	EHealLevel getHealLevel(int effectLevel) const override final;
};

class DLL_LINKAGE DispellMechanics : public DefaultSpellMechanics
{
public:
	DispellMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;

	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override final;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE EarthquakeMechanics : public DefaultSpellMechanics
{
public:
	EarthquakeMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE HypnotizeMechanics : public DefaultSpellMechanics
{
public:
	HypnotizeMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
};

class DLL_LINKAGE ObstacleMechanics : public DefaultSpellMechanics
{
public:
	ObstacleMechanics(CSpell * s): DefaultSpellMechanics(s){};

protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE WallMechanics : public ObstacleMechanics
{
public:
	WallMechanics(CSpell * s): ObstacleMechanics(s){};
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const override;
};

class DLL_LINKAGE RemoveObstacleMechanics : public DefaultSpellMechanics
{
public:
	RemoveObstacleMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const SpellTargetingContext & ctx) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

///all rising spells
class DLL_LINKAGE RisingSpellMechanics : public HealingSpellMechanics
{
public:
	RisingSpellMechanics(CSpell * s): HealingSpellMechanics(s){};
	EHealLevel getHealLevel(int effectLevel) const override;
};

class DLL_LINKAGE SacrificeMechanics : public RisingSpellMechanics
{
public:
	SacrificeMechanics(CSpell * s): RisingSpellMechanics(s){};

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
	int calculateHealedHP(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

///all rising spells but SACRIFICE
class DLL_LINKAGE SpecialRisingSpellMechanics : public RisingSpellMechanics
{
public:
	SpecialRisingSpellMechanics(CSpell * s): RisingSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
};

class DLL_LINKAGE SummonMechanics : public DefaultSpellMechanics
{
public:
	SummonMechanics(CSpell * s, CreatureID cre): DefaultSpellMechanics(s), creatureToSummon(cre){};

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
private:
	CreatureID creatureToSummon;
};

class DLL_LINKAGE TeleportMechanics: public DefaultSpellMechanics
{
public:
	TeleportMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};
