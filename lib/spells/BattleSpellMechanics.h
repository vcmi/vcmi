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

class CObstacleInstance;
class SpellCreatedObstacle;

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
protected:
	std::vector<const CStack *> calculateAffectedStacks(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
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
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;

	EHealLevel getHealLevel(int effectLevel) const override final;
private:
    static bool dispellSelector(const Bonus * b);
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

class DLL_LINKAGE EarthquakeMechanics : public SpecialSpellMechanics
{
public:
	EarthquakeMechanics(CSpell * s): SpecialSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE HypnotizeMechanics : public DefaultSpellMechanics
{
public:
	HypnotizeMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
};

class DLL_LINKAGE ObstacleMechanics : public SpecialSpellMechanics
{
public:
	ObstacleMechanics(CSpell * s): SpecialSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
protected:
	static bool isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	void placeObstacle(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, const BattleHex & pos) const;
	virtual void setupObstacle(SpellCreatedObstacle * obstacle) const = 0;
};

class PatchObstacleMechanics : public ObstacleMechanics
{
public:
	PatchObstacleMechanics(CSpell * s): ObstacleMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE LandMineMechanics : public PatchObstacleMechanics
{
public:
	LandMineMechanics(CSpell * s): PatchObstacleMechanics(s){};
	bool requiresCreatureTarget() const	override;
protected:
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE QuicksandMechanics : public PatchObstacleMechanics
{
public:
	QuicksandMechanics(CSpell * s): PatchObstacleMechanics(s){};
	bool requiresCreatureTarget() const	override;
protected:
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE WallMechanics : public ObstacleMechanics
{
public:
	WallMechanics(CSpell * s): ObstacleMechanics(s){};
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const override;
};

class DLL_LINKAGE FireWallMechanics : public WallMechanics
{
public:
	FireWallMechanics(CSpell * s): WallMechanics(s){};
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE ForceFieldMechanics : public WallMechanics
{
public:
	ForceFieldMechanics(CSpell * s): WallMechanics(s){};
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE RemoveObstacleMechanics : public SpecialSpellMechanics
{
public:
	RemoveObstacleMechanics(CSpell * s): SpecialSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
private:
    bool canRemove(const CObstacleInstance * obstacle, const int spellLevel) const;
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

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
	int calculateHealedHP(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};

///ANIMATE_DEAD and RESURRECTION
class DLL_LINKAGE SpecialRisingSpellMechanics : public RisingSpellMechanics
{
public:
	SpecialRisingSpellMechanics(CSpell * s): RisingSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const override;
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const override;
};

class DLL_LINKAGE SummonMechanics : public SpecialSpellMechanics
{
public:
	SummonMechanics(CSpell * s, CreatureID cre): SpecialSpellMechanics(s), creatureToSummon(cre){};

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
private:
	CreatureID creatureToSummon;
};

class DLL_LINKAGE TeleportMechanics: public DefaultSpellMechanics
{
public:
	TeleportMechanics(CSpell * s): DefaultSpellMechanics(s){};

	ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;
};
