/*
 * TownPortalMechanics.h, part of VCMI engine
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

class CGTownInstance;

class TownPortalMechanics final : public AdventureSpellMechanics
{
public:
	TownPortalMechanics(const CSpell * s);

protected:
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;

private:
	const CGTownInstance * findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & pool) const;
	int32_t movementCost(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	std::vector<const CGTownInstance *> getPossibleTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

VCMI_LIB_NAMESPACE_END
