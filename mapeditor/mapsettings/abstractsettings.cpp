/*
 * abstractsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "abstractsettings.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CGCreature.h"

//parses date for lose condition (1m 1w 1d)
int expiredDate(const QString & date)
{
	int result = 0;
	for(auto component : date.split(" "))
	{
		int days = component.left(component.lastIndexOf('d')).toInt();
		int weeks = component.left(component.lastIndexOf('w')).toInt();
		int months = component.left(component.lastIndexOf('m')).toInt();
		result += days > 0 ? days - 1 : 0;
		result += (weeks > 0 ? weeks - 1 : 0) * 7;
		result += (months > 0 ? months - 1 : 0) * 28;
	}
	return result;
}

QString expiredDate(int date)
{
	QString result;
	int m = date / 28;
	int w = (date % 28) / 7;
	int d = date % 7;
	if(m)
		result += QString::number(m) + "m";
	if(w)
	{
		if(!result.isEmpty())
			result += " ";
		result += QString::number(w) + "w";
	}
	if(d)
	{
		if(!result.isEmpty())
			result += " ";
		result += QString::number(d) + "d";
	}
	return result;
}

int3 posFromJson(const JsonNode & json)
{
	return int3(json.Vector()[0].Integer(), json.Vector()[1].Integer(), json.Vector()[2].Integer());
}

std::vector<JsonNode> linearJsonArray(const JsonNode & json)
{
	std::vector<JsonNode> result;
	if(json.getType() == JsonNode::JsonType::DATA_STRUCT)
		result.push_back(json);
	if(json.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto & node : json.Vector())
		{
			auto subvector = linearJsonArray(node);
			result.insert(result.end(), subvector.begin(), subvector.end());
		}
	}
	return result;
}

AbstractSettings::AbstractSettings(QWidget *parent)
	: QWidget{parent}
{

}

std::string AbstractSettings::getTownName(const CMap & map, int objectIdx)
{
	std::string name;
	if(auto town = dynamic_cast<const CGTownInstance*>(map.objects[objectIdx].get()))
	{
		auto * ctown = town->town;
		if(!ctown)
			ctown = VLC->townh->randomTown;

		name = ctown->faction ? town->getObjectName() : town->getNameTranslated() + ", (random)";
	}
	return name;
}

std::string AbstractSettings::getHeroName(const CMap & map, int objectIdx)
{
	std::string name;
	if(auto hero = dynamic_cast<const CGHeroInstance*>(map.objects[objectIdx].get()))
	{
		name = hero->getNameTranslated();
	}
	return name;
}

std::string AbstractSettings::getMonsterName(const CMap & map, int objectIdx)
{
	std::string name;
	[[maybe_unused]] auto monster = dynamic_cast<const CGCreature*>(map.objects[objectIdx].get());
	if(monster)
	{
		//TODO: get proper name
		//name = hero->name;
	}
	return name;
}

JsonNode AbstractSettings::conditionToJson(const EventCondition & event)
{
	JsonNode result;
	result["condition"].Integer() = event.condition;
	result["value"].Integer() = event.value;
	result["objectType"].Integer() = event.objectType;
	result["objectSubytype"].Integer() = event.objectSubtype;
	result["objectInstanceName"].String() = event.objectInstanceName;
	result["metaType"].Integer() = (ui8)event.metaType;
	{
		auto & position = result["position"].Vector();
		position.resize(3);
		position[0].Float() = event.position.x;
		position[1].Float() = event.position.y;
		position[2].Float() = event.position.z;
	}
	return result;
};
