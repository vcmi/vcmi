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

#include "ISpellMechanics.h"

enum class ESpellCastResult
{
	OK,
	CANCEL,//cast failed but it is not an error
	ERROR//internal error occurred
};

class DLL_LINKAGE AdventureSpellMechanics: public IAdventureSpellMechanics
{
public:
	AdventureSpellMechanics(CSpell * s): IAdventureSpellMechanics(s){};

	bool adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override final;
protected:
	///actual adventure cast implementation
	virtual ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const;
};

class DLL_LINKAGE SummonBoatMechanics : public AdventureSpellMechanics
{
public:
	SummonBoatMechanics(CSpell * s): AdventureSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE ScuttleBoatMechanics : public AdventureSpellMechanics
{
public:
	ScuttleBoatMechanics(CSpell * s): AdventureSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE DimensionDoorMechanics : public AdventureSpellMechanics
{
public:
	DimensionDoorMechanics(CSpell * s): AdventureSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE TownPortalMechanics : public AdventureSpellMechanics
{
public:
	TownPortalMechanics(CSpell * s): AdventureSpellMechanics(s){};
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE ViewMechanics : public AdventureSpellMechanics
{
public:
	ViewMechanics(CSpell * s): AdventureSpellMechanics(s){};
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


