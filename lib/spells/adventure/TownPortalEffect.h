/*
 * TownPortalEffect.h, part of VCMI engine
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

class DLL_LINKAGE TownPortalEffect final : public IAdventureSpellEffect
{
	const CSpell * owner;
	int movementPointsRequired;
	int movementPointsTaken;
	bool allowTownSelection;
	bool skipOccupiedTowns;

public:
	TownPortalEffect(const CSpell * s, const JsonNode & config);

	int getMovementPointsRequired() const { return movementPointsRequired; }
	bool townSelectionAllowed() const { return allowTownSelection; }

private:
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const override;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	const CGTownInstance * findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & pool) const;
	std::vector<const CGTownInstance *> getPossibleTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

VCMI_LIB_NAMESPACE_END
