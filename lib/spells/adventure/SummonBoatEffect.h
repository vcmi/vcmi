/*
 * SummonBoatEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AdventureSpellEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

class SummonBoatEffect final : public IAdventureSpellEffect
{
	const CSpell * owner;
	bool useExistingBoat;
	bool createNewBoat;

public:
	SummonBoatEffect(const CSpell * s, const JsonNode & config);

protected:
	bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;

	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const final;
};

VCMI_LIB_NAMESPACE_END
