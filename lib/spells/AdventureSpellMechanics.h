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

class ISpellMechanics;
class DefaultSpellMechanics;

class DLL_LINKAGE SummonBoatMechanics : public DefaultSpellMechanics
{
public:
	SummonBoatMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE ScuttleBoatMechanics : public DefaultSpellMechanics
{
public:
	ScuttleBoatMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE DimensionDoorMechanics : public DefaultSpellMechanics
{
public:
	DimensionDoorMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE TownPortalMechanics : public DefaultSpellMechanics
{
public:
	TownPortalMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE ViewMechanics : public DefaultSpellMechanics
{
public:
	ViewMechanics(CSpell * s): DefaultSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
	virtual bool filterObject(const CGObjectInstance * obj, const int spellLevel) const = 0;
};

class DLL_LINKAGE ViewAirMechanics : public ViewMechanics
{
public:
	ViewAirMechanics(CSpell * s): ViewMechanics(s){};
protected:
	bool filterObject(const CGObjectInstance * obj, const int spellLevel) const override;
};

class DLL_LINKAGE ViewEarthMechanics : public ViewMechanics
{
public:
	ViewEarthMechanics(CSpell * s): ViewMechanics(s){};
protected:
	bool filterObject(const CGObjectInstance * obj, const int spellLevel) const override;
};


