/*
 * ViewWorldMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AdventureSpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

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
