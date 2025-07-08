/*
 * DimensionDoorMechanics.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_END
