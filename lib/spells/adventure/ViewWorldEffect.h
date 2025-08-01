/*
 * ViewWorldEffect.h, part of VCMI engine
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

class ViewWorldEffect final : public IAdventureSpellEffect
{
	std::vector<MapObjectID> filteredObjects;
	bool showTerrain = false;

public:
	ViewWorldEffect(const CSpell * s, const JsonNode & config);

	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

VCMI_LIB_NAMESPACE_END
