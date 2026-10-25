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
class BonusParameters;
class IIdentifierResolver;
class LibraryLoadContext;
namespace JsonUtils
{
	std::shared_ptr<Bonus> parseBonus(const JsonVector & ability_vec, LibraryLoadContext & context);
	/// Parses bonus outside of library loading, for example from map or game settings
	std::shared_ptr<Bonus> parseBonus(const JsonNode & ability, IIdentifierResolver & resolver);
	/// Parses bonus of library content. Description of the bonus, if any, is registered under descriptionID
	std::shared_ptr<Bonus> parseBonus(const JsonNode & ability, LibraryLoadContext & context, const TextIdentifier & descriptionID = "");
	bool parseBonus(const JsonNode & ability, Bonus * placement, LibraryLoadContext & context, const TextIdentifier & descriptionID = "");
	std::shared_ptr<const ILimiter> parseLimiter(const JsonNode & limiter, IIdentifierResolver & resolver);
}
