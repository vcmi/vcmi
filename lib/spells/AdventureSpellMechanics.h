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

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;

enum class ESpellCastResult
{
	OK, // cast successful
	CANCEL, // cast failed but it is not an error, no mana has been spent
	PENDING,
	ERROR// error occurred, for example invalid request from player
};

class AdventureSpellMechanics : public IAdventureSpellMechanics
{
public:
	AdventureSpellMechanics(const CSpell * s);

	bool canBeCast(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;
	bool canBeCastAt(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;

	bool adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override final;
protected:
	///actual adventure cast implementation
	virtual ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const;
	virtual bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const;

	void performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

class SummonBoatMechanics final : public AdventureSpellMechanics
{
public:
	SummonBoatMechanics(const CSpell * s);
protected:
	bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const override;

	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class ScuttleBoatMechanics final : public AdventureSpellMechanics
{
public:
	ScuttleBoatMechanics(const CSpell * s);
protected:
	bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const override;

	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class DimensionDoorMechanics final : public AdventureSpellMechanics
{
public:
	DimensionDoorMechanics(const CSpell * s);
protected:
	bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const override;
	bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const override;

	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

class TownPortalMechanics final : public AdventureSpellMechanics
{
public:
	TownPortalMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
private:
	const CGTownInstance * findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance*> & pool) const;
	int32_t movementCost(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	std::vector <const CGTownInstance*> getPossibleTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

class ViewMechanics : public AdventureSpellMechanics
{
public:
	ViewMechanics(const CSpell * s);
protected:
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	virtual bool filterObject(const CGObjectInstance * obj, const int32_t spellLevel) const = 0;
	virtual bool showTerrain(const int32_t spellLevel) const = 0;
};

class ViewAirMechanics final : public ViewMechanics
{
public:
	ViewAirMechanics(const CSpell * s);
protected:
	bool filterObject(const CGObjectInstance * obj, const int32_t spellLevel) const override;
	bool showTerrain(const int32_t spellLevel) const override;
};

class ViewEarthMechanics final : public ViewMechanics
{
public:
	ViewEarthMechanics(const CSpell * s);
protected:
	bool filterObject(const CGObjectInstance * obj, const int32_t spellLevel) const override;
	bool showTerrain(const int32_t spellLevel) const override;
};



VCMI_LIB_NAMESPACE_END
