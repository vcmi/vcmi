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
#include "../texts/TextIdentifier.h"

struct Bonus;
class ILimiter;
class CSelector;
class BonusParameters;
class IIdentifierResolver;
namespace JsonUtils
{
	std::shared_ptr<Bonus> parseBonus(const JsonVector & ability_vec, IIdentifierResolver & resolver);
	std::shared_ptr<Bonus> parseBonus(const JsonNode & ability, IIdentifierResolver & resolver, const TextIdentifier & descriptionID = "");
	bool parseBonus(const JsonNode & ability, Bonus * placement, IIdentifierResolver & resolver, const TextIdentifier & descriptionID = "");
	std::shared_ptr<const ILimiter> parseLimiter(const JsonNode & limiter, IIdentifierResolver & resolver);
	CSelector parseSelector(const JsonNode & ability, IIdentifierResolver & resolver);
}
