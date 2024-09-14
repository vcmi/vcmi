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

#include "../VCMI_Lib.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

ObjectInfo::ObjectInfo():
	destroyObject([](CGObjectInstance * obj){}),
	maxPerZone(std::numeric_limits<ui32>::max())
{
}

ObjectInfo::ObjectInfo(const ObjectInfo & other)
{
	templates = other.templates;
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
	value = other.value;
	probability = other.probability;
	maxPerZone = other.maxPerZone;
	generateObject = other.generateObject;
	destroyObject = other.destroyObject;
	return *this;
}

void ObjectInfo::setAllTemplates(MapObjectID type, MapObjectSubID subtype)
{
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	templates = templHandler->getTemplates();
}

void ObjectInfo::setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrainType)
{
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	templates = templHandler->getTemplates(terrainType);
}

VCMI_LIB_NAMESPACE_END