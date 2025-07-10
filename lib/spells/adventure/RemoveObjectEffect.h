/*
 * RemoveObjectEffect.h, part of VCMI engine
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

class RemoveObjectEffect final : public AdventureSpellRangedEffect
{
	const CSpell * owner;
	std::vector<MapObjectID> removedObjects;
	MetaString failMessage;
	ImagePath cursorPath;

public:
	RemoveObjectEffect(const CSpell * s, const JsonNode & config);

	bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const override;
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

VCMI_LIB_NAMESPACE_END
