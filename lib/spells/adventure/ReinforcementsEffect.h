/*
 * ReinforcementsEffect.h, part of VCMI engine
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

class CGTownInstance;

class DLL_LINKAGE ReinforcementsEffect final : public IAdventureSpellEffect
{
	const CSpell * owner;
	bool allowTownSelection;

public:
	ReinforcementsEffect(const CSpell * s, const JsonNode & config);

private:
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const override;
};

VCMI_LIB_NAMESPACE_END
