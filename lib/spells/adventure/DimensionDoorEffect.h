/*
 * DimensionDoorEffect.h, part of VCMI engine
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

class DimensionDoorEffect final : public AdventureSpellRangedEffect
{
	std::string cursor;
	std::string cursorGuarded;
	int movementPointsRequired;
	int movementPointsTaken;
	bool waterLandFailureTakesPoints;
	bool exposeFow;

public:
	DimensionDoorEffect(const CSpell * s, const JsonNode & config);

private:
	bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;
	bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const final;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const final;
	std::string getCursorForTarget(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;
};

VCMI_LIB_NAMESPACE_END
