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

/// Class that is used as base for multiple object constructors
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
	void configureObject(CGObjectInstance * object, vstd::RNG & rng) const final
	{
		ObjectType * castedObject = dynamic_cast<ObjectType*>(object);

		if(castedObject == nullptr)
			throw std::runtime_error("Unexpected object type!");

		randomizeObject(castedObject, rng);
	}

	CGObjectInstance * create(IGameCallback * cb, std::shared_ptr<const ObjectTemplate> tmpl) const final
	{
		ObjectType * result = createObject(cb);

		preInitObject(result);

		if(tmpl)
			result->appearance = tmpl;

		initializeObject(result);

		return result;
	}

protected:
	virtual void initializeObject(ObjectType * object) const {}
	virtual void randomizeObject(ObjectType * object, vstd::RNG & rng) const {}
	virtual ObjectType * createObject(IGameCallback * cb) const
	{
		return new ObjectType(cb);
	}
};

VCMI_LIB_NAMESPACE_END
