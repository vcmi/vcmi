#include "StdInc.h"
#include "CObjectConstructor.h"

/*
 * CObjectConstructor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CObjectWithRewardConstructor::CObjectWithRewardConstructor()
{
}

void CObjectWithRewardConstructor::init(const JsonNode & config)
{
	int id = config["id"].Float();
	std::string name = config["name"].String();
	for (auto & entry : config["types"].Struct()) // for each object type
	{
		JsonNode typeConf = entry.second;

		int subID = typeConf["id"].Float();

		objectInfos[id][subID].info.init(typeConf["properties"]);
		for (auto entry : typeConf["templates"].Struct())
		{
			ObjectTemplate tmpl;
			tmpl.id = Obj(id);
			tmpl.subid = subID;
			tmpl.readJson(entry.second);
			objectInfos[id][subID].templates.push_back(tmpl);
		}
	}
}

std::vector<ObjectTemplate> CObjectWithRewardConstructor::getTemplates(si32 type, si32 subType) const
{
	assert(handlesID(type, subtype));
	return objectInfos.at(type).at(subType).templates;
}

CGObjectInstance * CObjectWithRewardConstructor::create(ObjectTemplate tmpl) const
{
	assert(handlesID(tmpl));
	auto ret = new CObjectWithReward();
	ret->appearance = tmpl;
	return ret;
}

bool CObjectWithRewardConstructor::handlesID(si32 id, si32 subID) const
{
	return objectInfos.count(id) && objectInfos.at(id).count(subID);
}

bool CObjectWithRewardConstructor::handlesID(ObjectTemplate tmpl) const
{
	return handlesID(tmpl.id, tmpl.subid);
}

void CObjectWithRewardConstructor::configureObject(CGObjectInstance * object) const
{
	assert(handlesID(object->appearance));
	objectInfos.at(object->ID).at(object->subID).info.configureObject(dynamic_cast<CObjectWithReward*>(object));
}

const IObjectInfo * CObjectWithRewardConstructor::getObjectInfo(ObjectTemplate tmpl) const
{
	assert(handlesID(tmpl));
	return &objectInfos.at(tmpl.id).at(tmpl.subid).info;
}
