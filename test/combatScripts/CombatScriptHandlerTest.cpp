/*
 * CombatScriptHandlerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"

namespace test
{

/// Covers the registry chain on content loaded by the test preset: the combatScripts content
/// type is known to the mod system, its identifier resolves, and the Lua factory produced a
/// usable handler for it.
TEST(CombatScriptHandlerTest, FixtureScriptIsLoaded)
{
	auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "testSpikes", false);

	ASSERT_TRUE(index.has_value());
	EXPECT_NE(nullptr, LIBRARY->combatScripts()->get(CombatScriptID(*index)));
}

/// Bonuses referring to a script from a mod that is no longer present must not crash the battle.
TEST(CombatScriptHandlerTest, UnresolvedScriptIsNull)
{
	EXPECT_EQ(nullptr, LIBRARY->combatScripts()->get(CombatScriptID::NONE));
}

}
