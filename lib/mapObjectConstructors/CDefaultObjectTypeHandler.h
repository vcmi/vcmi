/*
* CDefaultObjectTypeHandler.h, part of VCMI engine
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

/// Class that is used for objects that do not have dedicated handler
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
protected:
	ObjectType * createTyped(std::shared_ptr<const ObjectTemplate> tmpl /* = nullptr */) const
	{
		auto obj = new ObjectType();
		preInitObject(obj);

		//Set custom template or leave null
		if (tmpl)
		{
			obj->appearance = tmpl;
		}

		return obj;
	}
public:
	CDefaultObjectTypeHandler() {}

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override
	{
		return createTyped(tmpl);
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override
	{
	}

	virtual std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override
	{
		return nullptr;
	}
};

VCMI_LIB_NAMESPACE_END
