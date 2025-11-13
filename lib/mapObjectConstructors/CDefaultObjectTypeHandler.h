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
	void configureObject(CGObjectInstance * object, IGameRandomizer & gameRandomizer) const final
	{
		ObjectType * castedObject = dynamic_cast<ObjectType*>(object);

		if(castedObject == nullptr)
			throw std::runtime_error("Unexpected object type!");

		randomizeObject(castedObject, gameRandomizer);
	}

	std::shared_ptr<CGObjectInstance> create(IGameInfoCallback * cb, std::shared_ptr<const ObjectTemplate> tmpl) const final
	{
		assert(cb);

		auto result = createObject(cb);

		preInitObject(result.get());

		if(tmpl)
			result->appearance = tmpl;

		initializeObject(result.get());

		return result;
	}

protected:
	virtual void initializeObject(ObjectType * object) const {}
	virtual void randomizeObject(ObjectType * object, IGameRandomizer & gameRandomizer) const {}
	virtual std::shared_ptr<ObjectType> createObject(IGameInfoCallback * cb) const
	{
		assert(cb);

		return std::make_shared<ObjectType>(cb);
	}
};

VCMI_LIB_NAMESPACE_END
