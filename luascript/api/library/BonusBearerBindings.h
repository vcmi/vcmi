/*
 * BonusBearerBindings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../MethodRegistrar.h"
#include "../../LuaStack.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/bonuses/BonusFilter.h"
#include "../../../lib/bonuses/BonusList.h"
#include "../../../lib/bonuses/BonusSelector.h"
#include "../../../lib/bonuses/IBonusBearer.h"

namespace scripting::api
{

/// Shared bindings for proxies whose underlying C++ type derives from IBonusBearer.
template<class Leaf>
class BonusBearerBindings
{
public:
	static void registerMethods(MethodRegistrar & R)
	{
		R.template function<&getBonuses>("getBonuses",
			{{"filter", "Which bonuses to collect. An empty filter collects every one of them."}},
			{"Bonuses of the bearer the filter describes."},
            "Returns the bonuses of the bearer that match the filter. "
            "For cases where called want more precise control, please use filter for initial pass and use separate :filter predicate");
		R.template function<&getBonusesValue>("getBonusesValue",
			{{"filter", "Which bonuses to count. An empty filter counts every one of them."}},
			{"Value of the matching bonuses taken together."},
			"Returns what the matching bonuses are worth together. Not a plain sum - percentages, "
			"independent floors and ceilings combine by the rules of the engine. Prefer this over "
            "adding up `getBonuses` where possible.");
	}

private:
	static const std::pair<CSelector, std::string> & compile(const BonusFilter & filter)
	{
        // Compiling a filter resolves identifiers, which is a sweep of every mod scope. The answer
        // never changes, so each thread keeps the ones it has been asked for.
		using Key = std::tuple<std::string, std::string, int>;
		thread_local std::map<Key, std::pair<CSelector, std::string>> known;

		Key key{
			filter.type.value_or(std::string{}),
			filter.subtype.value_or(std::string{}),
			filter.sourceType ? static_cast<int>(*filter.sourceType) : -1
		};

		auto entry = known.find(key);

		if(entry == known.end())
			entry = known.emplace(key, filter.compile()).first;

		return entry->second;
	}

	static BonusList getBonuses(const Leaf & bearer, const BonusFilter & filter)
	{
		const auto & [selector, cachingString] = compile(filter);

		return *bearer.getBonuses(selector, cachingString);
	}

	static int32_t getBonusesValue(const Leaf & bearer, const BonusFilter & filter)
	{
		const auto & [selector, cachingString] = compile(filter);

		return bearer.valOfBonuses(selector, cachingString);
	}
};

}
