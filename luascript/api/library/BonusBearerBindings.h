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
#include "../../../lib/bonuses/BonusList.h"
#include "../../../lib/bonuses/BonusSelector.h"
#include "../../../lib/bonuses/IBonusBearer.h"

namespace scripting::api
{

/// Shared bindings for proxies whose underlying C++ type derives from IBonusBearer.
/// Exposes a predicate-filtered enumeration of bonuses on the bearer.
template<class Leaf>
class BonusBearerBindings
{
public:
	static void registerMethods(MethodRegistrar & R)
	{
		R.template function<&getBonusesOfType>("getBonusesOfType",
			{{"type", "Bonus type to collect, by its json key - \"SLAYER\", \"JOUSTING\", ..."}},
			{"Bonuses of that type affecting the bearer."},
			"Returns the bonuses of one type affecting the bearer. Prefer this over `getBonuses` "
			"whenever the type is known: the engine both caches this query and answers it without "
			"handing every unrelated bonus to the script.");
		R.template cfunction<&getBonuses>("getBonuses",
			{{"predicate", "fun(b: Bonus): boolean", "Selector — called for each bonus on the bearer; bonus is kept when it returns true."}},
			{"BonusList", "Bonuses for which the predicate returned true."},
			"Returns all bonuses affecting the bearer for which the predicate returns true.");
	}

private:
	/// Resolving a bonus type by name goes through the identifier storage, which sweeps every mod
	/// scope for candidates - affordable while content loads, not once per query of a battle. The
	/// answer never changes, so each thread keeps the ones it has asked for.
	static BonusType decodeType(const std::string & type)
	{
		thread_local std::unordered_map<std::string, BonusType> known;

		auto entry = known.find(type);

		if(entry != known.end())
			return entry->second;

		auto decoded = static_cast<BonusType>(BonusTypeID::decode(type));
		known.emplace(type, decoded);

		return decoded;
	}

	static BonusList getBonusesOfType(const Leaf & bearer, const std::string & type)
	{
		return *bearer.getBonusesOfType(decodeType(type));
	}

	static int getBonuses(lua_State * L)
	{
		LuaStack S(L);
		const Leaf * bearer = nullptr;
		S.get(1, bearer);

		if(!bearer || !lua_isfunction(L, 2))
		{
			S.clear();
			return 0;
		}

		auto allBonuses = bearer->getAllBonuses(Selector::all);

		BonusList result;
		for(const auto & bonus : *allBonuses)
		{
			lua_pushvalue(L, 2);
			S.push(*bonus);
			lua_call(L, 1, 1);
			const bool keep = lua_toboolean(L, -1);
			lua_pop(L, 1);
			if(keep)
				result.push_back(bonus);
		}

		S.clear();
		S.push(result);
		return 1;
	}
};

}
