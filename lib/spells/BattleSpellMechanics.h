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


class ChainLightningMechanics: public DefaultSpellMechanics
{
public:
	ChainLightningMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const override;
};

class CloneMechanics: public DefaultSpellMechanics
{
public:
	CloneMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;	
};

class CureMechanics: public DefaultSpellMechanics
{
public:
	CureMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	
	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override;	
};



class DispellMechanics: public DefaultSpellMechanics
{
public:
	DispellMechanics(CSpell * s): DefaultSpellMechanics(s){};
	
	void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const override;	
};

class HypnotizeMechanics: public DefaultSpellMechanics
{
public:
	HypnotizeMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;	
}; 

class ObstacleMechanics: public DefaultSpellMechanics
{
public:
	ObstacleMechanics(CSpell * s): DefaultSpellMechanics(s){};		

protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;	
};

class WallMechanics: public ObstacleMechanics
{
public:
	WallMechanics(CSpell * s): ObstacleMechanics(s){};	
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const override;		
};

class RemoveObstacleMechanics: public DefaultSpellMechanics
{
public:
	RemoveObstacleMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};

///all rising spells
class RisingSpellMechanics: public DefaultSpellMechanics
{
public:
	RisingSpellMechanics(CSpell * s): DefaultSpellMechanics(s){};		
	
};

class SacrificeMechanics: public RisingSpellMechanics
{
public:
	SacrificeMechanics(CSpell * s): RisingSpellMechanics(s){};	
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};

///all rising spells but SACRIFICE
class SpecialRisingSpellMechanics: public RisingSpellMechanics
{
public:
	SpecialRisingSpellMechanics(CSpell * s): RisingSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;						
};

class SummonMechanics: public DefaultSpellMechanics
{
public:
	SummonMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};

class TeleportMechanics: public DefaultSpellMechanics
{
public:
	TeleportMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const override;		
};
