/*
* HillFortInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "AObjectTypeHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class HillFortInstanceConstructor final : public AObjectTypeHandler
{
	JsonNode parameters;

protected:
	void initTypeData(const JsonNode & config) override;
	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<AObjectTypeHandler&>(*this);
		h & parameters;
	}
};

VCMI_LIB_NAMESPACE_END
