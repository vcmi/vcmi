/*
 * AdventureSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
 #pragma once
 
 #include "CDefaultSpellMechanics.h"


//todo: make configurable
class AdventureBonusingMechanics: public DefaultSpellMechanics 
{
public:	
	AdventureBonusingMechanics(CSpell * s, Bonus::BonusType _bonusTypeID): DefaultSpellMechanics(s), bonusTypeID(_bonusTypeID){};	
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
private:
	Bonus::BonusType bonusTypeID;
};

class SummonBoatMechanics: public DefaultSpellMechanics 
{
public:
	SummonBoatMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};

class ScuttleBoatMechanics: public DefaultSpellMechanics 
{
public:
	ScuttleBoatMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};

class DimensionDoorMechanics: public DefaultSpellMechanics 
{
public:	
	DimensionDoorMechanics(CSpell * s): DefaultSpellMechanics(s){};	
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};

class TownPortalMechanics: public DefaultSpellMechanics 
{
public:	
	TownPortalMechanics(CSpell * s): DefaultSpellMechanics(s){};	
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};

class ViewAirMechanics: public DefaultSpellMechanics 
{
public:	
	ViewAirMechanics(CSpell * s): DefaultSpellMechanics(s){};	
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};

class ViewEarthMechanics: public DefaultSpellMechanics 
{
public:	
	ViewEarthMechanics(CSpell * s): DefaultSpellMechanics(s){};	
protected:
	bool applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;	
};


