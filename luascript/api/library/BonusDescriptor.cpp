/*
 * BonusDescriptor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BonusDescriptor.h"

#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/json/JsonBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

JsonNode BonusDescriptor::toJson() const
{
	JsonNode result;
	result["val"] = JsonNode(val);
	result["turns"] = JsonNode(turns);
	if(hidden)
		result["hidden"] = JsonNode(hidden);
	if(!stacking.empty())
		result["stacking"] = JsonNode(stacking);
	if(!description.empty())
		result["description"] = JsonNode(description);
	if(!icon.empty())
		result["icon"] = JsonNode(icon);

	if(!type.isNull())               result["type"] = type;
	if(!subtype.isNull())            result["subtype"] = subtype;
	if(!valueType.isNull())          result["valueType"] = valueType;
	if(!effectRange.isNull())        result["effectRange"] = effectRange;
	if(!duration.isNull())           result["duration"] = duration;
	if(!sourceType.isNull())         result["sourceType"] = sourceType;
	if(!sourceID.isNull())           result["sourceID"] = sourceID;
	if(!targetSourceType.isNull())   result["targetSourceType"] = targetSourceType;
	if(!addInfo.isNull())            result["addInfo"] = addInfo;
	if(!limiters.isNull())           result["limiters"] = limiters;
	if(!propagator.isNull())         result["propagator"] = propagator;
	if(!updater.isNull())            result["updater"] = updater;
	if(!propagationUpdater.isNull()) result["propagationUpdater"] = propagationUpdater;

	return result;
}

Bonus BonusDescriptor::toBonus() const
{
	Bonus b;
	JsonUtils::parseBonus(toJson(), &b);
	return b;
}

}

VCMI_LIB_NAMESPACE_END
