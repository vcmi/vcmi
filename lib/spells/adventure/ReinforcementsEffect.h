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

#include "TownRelatedAdventureSpellEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;

class DLL_LINKAGE ReinforcementsEffect final : public spells::adventure::TownRelatedAdventureSpellEffect
{
	std::string casterInTownTextID;
	std::string selectTownTitleTextID;
	std::string selectTownDescriptionTextID;
	std::string garrisonTitleTextID;

public:
	ReinforcementsEffect(const CSpell * s, const JsonNode & config);

private:
	void configureDialogTitleAndDescription(MetaString & title, MetaString & description) const override;
	ESpellCastResult beginCastExtraChecks(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & towns) const override;
	ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override;
};

VCMI_LIB_NAMESPACE_END
