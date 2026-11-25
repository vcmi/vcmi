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

void ObjectInfo::setAllTemplates(const CObjectClassesHandler & objects, MapObjectID type, MapObjectSubID subtype)
{
	auto & templHandler = objects.getHandlerFor(type, subtype);
	templates = templHandler.getTemplates();
}

void ObjectInfo::setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrainType)
{
	auto & templHandler = LIBRARY->objtypeh->getHandlerFor(type, subtype);
	templates = templHandler.getTemplates(terrainType);
}

CompoundMapObjectID ObjectInfo::getCompoundID() const
{
	return CompoundMapObjectID(primaryID, secondaryID);
}

bool ObjectInfo::isStandalone() const
{
	return LIBRARY->objtypeh->getHandlerFor(getCompoundID()).getRMGInfo().standalone;
}
