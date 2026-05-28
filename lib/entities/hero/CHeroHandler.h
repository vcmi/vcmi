/*
 * CHeroHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/HeroTypeService.h>

#include "CHero.h" // convenience include - users of handler generally also use its entity


#include "../../GameConstants.h"
#include "../../IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CHeroHandler : public CHandlerBase<HeroTypeID, HeroType, CHero, HeroTypeService>
{
	/// expPerLEvel[i] is amount of exp needed to reach level i;
	/// consists of 196 values. Any higher levels require experience larger that TExpType can hold
	std::vector<TExpType> expPerLevel;

	struct SpecialtyToGenerate
	{
		HeroTypeID hero;
		SecondarySkill skill;
		int stepSize;
	};

	/// Helper field to generate specialties for heroes after loading is complete
	mutable std::vector<SpecialtyToGenerate> skillSpecialtiesToGenerate;

	/// helpers for loading to avoid huge load functions
	void loadHeroArmy(CHero * hero, const JsonNode & node) const;
	void loadHeroSkills(CHero * hero, const JsonNode & node) const;
	void loadHeroSpecialty(CHero * hero, const JsonNode & node) const;

	void loadExperience();

	std::vector<std::shared_ptr<Bonus>> createCreatureSpecialty(CreatureID cid, int fixedLevel, int growthPerStep) const;
	std::vector<std::shared_ptr<Bonus>> createSecondarySkillSpecialty(SecondarySkill skillID, int growthPerStep) const;

public:
	ui32 level(TExpType experience) const; //calculates level corresponding to given experience amount
	TExpType reqExp(ui32 level) const; //calculates experience required for given level
	ui32 maxSupportedLevel() const;

	std::vector<JsonNode> loadLegacyData() override;

	void beforeValidate(JsonNode & object) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	CHeroHandler();
	~CHeroHandler();

	std::set<HeroTypeID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CHero> loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
