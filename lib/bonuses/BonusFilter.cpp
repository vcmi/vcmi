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

std::pair<CSelector, std::string> BonusFilter::compile() const
{
	if(subtype && !type)
		throw std::runtime_error("Bonus filter names a subtype but no type! What a subtype is depends on the type of the bonus.");

	CSelector selector = Selector::all;
	// mirrors the keys the engine builds for its own queries, so that a script asking what the
	// engine already asked is answered from the same cached list
	std::string cachingString;

	auto appendKey = [&cachingString](const std::string & part)
	{
		if(!cachingString.empty())
			cachingString += '_';

		cachingString += part;
	};

	if(type)
	{
		const auto decoded = static_cast<BonusType>(BonusTypeID::decode(*type));

		if(subtype)
		{
			const auto decodedSubtype = decodeBonusSubtype(decoded, *subtype);

			selector = Selector::typeSubtype(decoded, decodedSubtype);
			appendKey("type_" + std::to_string(static_cast<int>(decoded)) + "_" + std::to_string(decodedSubtype.getNum()));
		}
		else
		{
			selector = Selector::type()(decoded);
			appendKey("type_" + std::to_string(static_cast<int>(decoded)));
		}
	}

	if(sourceType)
	{
		selector = selector.And(Selector::sourceTypeSel(*sourceType));
		appendKey("source_" + std::to_string(static_cast<int>(*sourceType)));
	}

	if(shooting)
	{
		const auto limited = *shooting ? BonusLimitEffect::ONLY_DISTANCE_FIGHT : BonusLimitEffect::ONLY_MELEE_FIGHT;

		selector = selector.And(Selector::effectRange()(BonusLimitEffect::NO_LIMIT).Or(Selector::effectRange()(limited)));
		appendKey(*shooting ? "ranged" : "melee");
	}

	// an empty key is not cached at all, and a filter that names nothing is asked often enough -
	// once per unit per attack, by the damage calculator - to be worth a key of its own
	if(cachingString.empty())
		cachingString = "all";

	return {selector, cachingString};
}
