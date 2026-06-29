/*
* KeyGuardInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "../mapObjects/Quest.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Builds keymaster-gated quest guards/gates (the H3 border guard / border gate):
/// the object's colour is its subtype, so the limiter simply requires the matching key.
template<class ObjectType>
class KeyGuardInstanceConstructor final : public CDefaultObjectTypeHandler<ObjectType>
{
	void initializeObject(ObjectType * object) const override
	{
		object->addQuest().mission.requiredKeys.push_back(MapObjectSubID(this->getSubIndex()));
	}
};

VCMI_LIB_NAMESPACE_END
