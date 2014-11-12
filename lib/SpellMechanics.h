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

class DefaultSpellMechanics: public ISpellMechanics
{
public:
	DefaultSpellMechanics(CSpell * s): ISpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;
	
	bool adventureCast(SpellCastContext & context) override; 
	bool battleCast(SpellCastContext & context) override; 
};

class CloneMechnics: public DefaultSpellMechanics
{
public:
	CloneMechnics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;
};

class DispellHelpfulMechanics: public DefaultSpellMechanics
{
public:
	DispellHelpfulMechanics(CSpell * s): DefaultSpellMechanics(s){};
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;	
};

class HypnotizeMechanics: public DefaultSpellMechanics
{
public:
	HypnotizeMechanics(CSpell * s): DefaultSpellMechanics(s){};	
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;	
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
	ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;						
};

class SacrificeMechanics: public RisingSpellMechanics
{
public:
	SacrificeMechanics(CSpell * s): RisingSpellMechanics(s){};		
};
