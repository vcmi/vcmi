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

class DLL_LINKAGE SummonBoatEffect final : public IAdventureSpellEffect
{
	const CSpell * owner;
	BoatId createdBoat = BoatId::NONE;
	bool useExistingBoat;

public:
	SummonBoatEffect(const CSpell * s, const JsonNode & config);

	bool canCreateNewBoat() const;
	int getSuccessChance(const spells::Caster * caster) const;

private:
	bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const final;
};

VCMI_LIB_NAMESPACE_END
