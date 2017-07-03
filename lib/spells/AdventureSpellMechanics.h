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

class CGTownInstance;

enum class ESpellCastResult
{
	OK,
	CANCEL,//cast failed but it is not an error
	PENDING,
	ERROR//internal error occurred
};

class DLL_LINKAGE AdventureSpellMechanics : public IAdventureSpellMechanics
{
public:
	AdventureSpellMechanics(const CSpell * s);

	bool adventureCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override final;
protected:
	///actual adventure cast implementation
	virtual ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual ESpellCastResult beginCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	void performCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	void endCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const ESpellCastResult result) const;
};

class DLL_LINKAGE SummonBoatMechanics : public AdventureSpellMechanics
{
public:
	SummonBoatMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE ScuttleBoatMechanics : public AdventureSpellMechanics
{
public:
	ScuttleBoatMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE DimensionDoorMechanics : public AdventureSpellMechanics
{
public:
	DimensionDoorMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class DLL_LINKAGE TownPortalMechanics : public AdventureSpellMechanics
{
public:
	TownPortalMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	ESpellCastResult beginCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
private:
	const CGTownInstance * findNearestTown(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance*> & pool) const;
	int movementCost(const AdventureSpellCastParameters & parameters) const;
	std::vector <const CGTownInstance*> getPossibleTowns(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

class DLL_LINKAGE ViewMechanics : public AdventureSpellMechanics
{
public:
	ViewMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	virtual bool filterObject(const CGObjectInstance * obj, const int spellLevel) const = 0;
};

class DLL_LINKAGE ViewAirMechanics : public ViewMechanics
{
public:
	ViewAirMechanics(const CSpell * s);
protected:
	bool filterObject(const CGObjectInstance * obj, const int spellLevel) const override;
};

class DLL_LINKAGE ViewEarthMechanics : public ViewMechanics
{
public:
	ViewEarthMechanics(const CSpell * s);
protected:
	bool filterObject(const CGObjectInstance * obj, const int spellLevel) const override;
};


