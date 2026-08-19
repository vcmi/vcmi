/*
 * BonusFilter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BonusFilter.h"

#include "../GameLibrary.h"
#include "../CBonusTypeHandler.h"

std::pair<CSelector, std::string> BonusFilter::compile() const
{
	if(subtype && !type)
		throw std::runtime_error("Bonus filter names a subtype but no type! What a subtype is depends on the type of the bonus.");

	CSelector selector = Selector::all;
	// mirrors the keys the engine builds for its own queries, so that a script asking what the
	// engine already asked is answered from the same cached list
	std::string cachingString;

	if(type)
	{
		const auto decoded = static_cast<BonusType>(BonusTypeID::decode(*type));

		if(subtype)
		{
			const auto decodedSubtype = decodeBonusSubtype(decoded, *subtype);

			selector = Selector::typeSubtype(decoded, decodedSubtype);
			cachingString = "type_" + std::to_string(static_cast<int>(decoded)) + "_" + std::to_string(decodedSubtype.getNum());
		}
		else
		{
			selector = Selector::type()(decoded);
			cachingString = "type_" + std::to_string(static_cast<int>(decoded));
		}
	}

	if(sourceType)
	{
		selector = selector.And(Selector::sourceTypeSel(*sourceType));
		cachingString += "_source_" + std::to_string(static_cast<int>(*sourceType));
	}

	return {selector, cachingString};
}
