/*
 * ObjectInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../mapObjects/ObjectTemplate.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CompoundMapObjectID;
class CGObjectInstance;

struct DLL_LINKAGE ObjectInfo
{
	ObjectInfo();
	ObjectInfo(const ObjectInfo & other);
	ObjectInfo & operator=(const ObjectInfo & other);

	std::vector<std::shared_ptr<const ObjectTemplate>> templates;
	ui32 value = 0;
	ui16 probability = 0;
	ui32 maxPerZone = 1;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;
	std::function<void(CGObjectInstance *)> destroyObject;
	
	void setAllTemplates(MapObjectID type, MapObjectSubID subtype);
	void setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrain);

	//bool matchesId(const CompoundMapObjectID & id) const;
};

VCMI_LIB_NAMESPACE_END