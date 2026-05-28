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

#include "TownRelatedAdventureSpellEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;

class DLL_LINKAGE TownPortalEffect final : public spells::adventure::TownRelatedAdventureSpellEffect
{
	int movementPointsRequired;
	int movementPointsTaken;

public:
	TownPortalEffect(const CSpell * s, const JsonNode & config);

	int getMovementPointsRequired() const { return movementPointsRequired; }
	bool townSelectionAllowed() const { return allowTownSelection; }

private:
	bool shouldOfferTownInDialog(const CGTownInstance * town) const override;
	void configureDialogTitleAndDescription(MetaString & title, MetaString & description) const override;
	ESpellCastResult beginCastExtraChecks(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & towns) const override;
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
	void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

VCMI_LIB_NAMESPACE_END
