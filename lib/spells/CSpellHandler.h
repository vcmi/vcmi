/*
 * CSpellHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CSpell.h"
#include "../IHandlerBase.h"
#include <vcmi/spells/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;
class IAdventureSpellMechanics;
class CBattleInfoCallback;
class AdventureSpellCastParameters;
class SpellCastEnvironment;
class JsonSerializeFormat;

class DLL_LINKAGE CSpellHandler: public CHandlerBase<SpellID, spells::Spell, CSpell, spells::Service>
{
	std::vector<int> spellRangeInHexes(std::string rng) const;

public:
	///IHandler base
	std::vector<JsonNode> loadLegacyData() override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	/**
	 * Gets a list of default allowed spells. OH3 spells are all allowed by default.
	 *
	 */
	std::set<SpellID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CSpell> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
