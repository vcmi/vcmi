/*
 * JsonBonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonNode.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class ILimiter;
class CSelector;
class CAddInfo;

namespace JsonUtils
{
	std::shared_ptr<Bonus> parseBonus(const JsonVector & ability_vec);
	std::shared_ptr<Bonus> parseBonus(const JsonNode & ability);
	std::shared_ptr<Bonus> parseBuildingBonus(const JsonNode & ability, const FactionID & faction, const BuildingID & building, const std::string & description);
	bool parseBonus(const JsonNode & ability, Bonus * placement);
	std::shared_ptr<ILimiter> parseLimiter(const JsonNode & limiter);
	CSelector parseSelector(const JsonNode &ability);
	void resolveAddInfo(CAddInfo & var, const JsonNode & node);
}

VCMI_LIB_NAMESPACE_END
