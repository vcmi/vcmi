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

VCMI_LIB_NAMESPACE_BEGIN

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
		R.template cfunction<&getBonuses>("getBonuses",
			{{"predicate", "fun(b: Bonus): boolean", "Selector — called for each bonus on the bearer; bonus is kept when it returns true."}},
			{"BonusList", "Bonuses for which the predicate returned true."},
			"Returns all bonuses affecting the bearer for which the predicate returns true.");
	}

private:
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

VCMI_LIB_NAMESPACE_END
