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

#include "../mapObjects/CObjectClassesHandler.h"
#include "../rewardable/Info.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CRewardableConstructor : public AObjectTypeHandler
{
	Rewardable::Info objectInfo;

	void initTypeData(const JsonNode & config) override;
	
	bool blockVisit = false;

public:
	bool hasNameTextID() const override;

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;

	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		AObjectTypeHandler::serialize(h, version);

		if (version >= 816)
			h & objectInfo;
	}
};

VCMI_LIB_NAMESPACE_END
