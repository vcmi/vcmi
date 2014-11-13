/*
 * SpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#pragma once

#include "CSpellHandler.h"

class DLL_LINKAGE ISpellMechanics
{
public:
	
	struct SpellTargetingContext
	{
		CBattleInfoCallback * cb;
		
		CSpell::TargetInfo ti;
		
		int schoolLvl;
	};
	
public:
	ISpellMechanics(CSpell * s);
	virtual ~ISpellMechanics(){};	
	
	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const = 0;
	virtual std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const = 0;
	
	virtual ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const = 0;
	
	
    /** \brief 
     *
     * \param 
     * \return true if no error
     *
     */                           
	virtual bool adventureCast(SpellCastContext & context) const = 0; 
	virtual bool battleCast(SpellCastContext & context) const = 0; 	
	
protected:
	CSpell * owner;	
};

class DefaultSpellMechanics: public ISpellMechanics
{
public:
	DefaultSpellMechanics(CSpell * s): ISpellMechanics(s){};
	
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const override;
	std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const override;
	
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;
	
	bool adventureCast(SpellCastContext & context) const override; 
	bool battleCast(SpellCastContext & context) const override; 
};



class WallMechanics: public DefaultSpellMechanics
{
public:
	WallMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr) const override;	
	
};


class ChainLightningMechanics: public DefaultSpellMechanics
{
public:
	ChainLightningMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	
};

class CloneMechanics: public DefaultSpellMechanics
{
public:
	CloneMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;
};

class DispellHelpfulMechanics: public DefaultSpellMechanics
{
public:
	DispellHelpfulMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;	
};

class HypnotizeMechanics: public DefaultSpellMechanics
{
public:
	HypnotizeMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;	
}; 

///all rising spells
class RisingSpellMechanics: public DefaultSpellMechanics
{
public:
	RisingSpellMechanics(CSpell * s): DefaultSpellMechanics(s){};		
	
};

///all rising spells but SACRIFICE
class SpecialRisingSpellMechanics: public RisingSpellMechanics
{
public:
	SpecialRisingSpellMechanics(CSpell * s): RisingSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const override;						
};

class SacrificeMechanics: public RisingSpellMechanics
{
public:
	SacrificeMechanics(CSpell * s): RisingSpellMechanics(s){};		
};
