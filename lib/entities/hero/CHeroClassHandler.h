/*
 * CHeroClassHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/HeroClassService.h>

#include "CHeroClass.h" // convenience include - users of handler generally also use its entity

#include "../../IHandlerBase.h"
#include "../../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CHeroClassHandler : public CHandlerBase<HeroClassID, HeroClass, CHeroClass, HeroClassService>
{
	void fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill pSkill) const;

public:
	std::vector<JsonNode> loadLegacyData() override;

	void afterLoadFinalization() override;

	~CHeroClassHandler();

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CHeroClass> loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
