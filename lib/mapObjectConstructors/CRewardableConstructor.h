/*
 * CRewardableConstructor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "AObjectTypeHandler.h"
#include "../rewardable/Info.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;

class DLL_LINKAGE CRewardableConstructor : public AObjectTypeHandler
{
	Rewardable::Info objectInfo;

	void assignBonuses(std::vector<Bonus> & bonuses, MapObjectID objectID) const;
	void initTypeData(const JsonNode & config) override;
	
	bool blockVisit = false;

public:
	bool hasNameTextID() const override;

	std::shared_ptr<CGObjectInstance> create(IGameCallback * cb, std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;

	void configureObject(CGObjectInstance * object, vstd::RNG & rng) const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;

	Rewardable::Configuration generateConfiguration(IGameCallback * cb, vstd::RNG & rand, MapObjectID objectID, const std::map<std::string, JsonNode> & presetVariables) const;
};

VCMI_LIB_NAMESPACE_END
