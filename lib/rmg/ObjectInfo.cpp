/*
 * ObjectInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObjectInfo.h"

#include "../GameLibrary.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

ObjectInfo::ObjectInfo(si32 ID, si32 subID):
	primaryID(ID),
	secondaryID(subID),
	destroyObject([](const CGObjectInstance & obj){}),
	maxPerZone(std::numeric_limits<ui32>::max())
{
}

ObjectInfo::ObjectInfo(CompoundMapObjectID id):
	ObjectInfo(id.primaryID, id.secondaryID)
{
}

ObjectInfo::ObjectInfo(const ObjectInfo & other)
{
	templates = other.templates;
	primaryID = other.primaryID;
	secondaryID = other.secondaryID;
	value = other.value;
	probability = other.probability;
	maxPerZone = other.maxPerZone;
	generateObject = other.generateObject;
	destroyObject = other.destroyObject;
}

ObjectInfo & ObjectInfo::operator=(const ObjectInfo & other)
{
	if (this == &other)
		return *this;

	templates = other.templates;
	primaryID = other.primaryID;
	secondaryID = other.secondaryID;
	value = other.value;
	probability = other.probability;
	maxPerZone = other.maxPerZone;
	generateObject = other.generateObject;
	destroyObject = other.destroyObject;
	return *this;
}

void ObjectInfo::setAllTemplates(MapObjectID type, MapObjectSubID subtype)
{
	auto templHandler = LIBRARY->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	templates = templHandler->getTemplates();
}

void ObjectInfo::setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrainType)
{
	auto templHandler = LIBRARY->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	templates = templHandler->getTemplates(terrainType);
}

CompoundMapObjectID ObjectInfo::getCompoundID() const
{
	return CompoundMapObjectID(primaryID, secondaryID);
}

VCMI_LIB_NAMESPACE_END
