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
#include "../../lib/combatScripts/CombatEventPayload.h"
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

/// An unset script identifier is a legal value that every accessor has to tolerate.
TEST(CombatScriptHandlerTest, UnsetScriptIsNull)
{
	EXPECT_EQ(nullptr, LIBRARY->combatScripts()->get(CombatScriptID::NONE));
	EXPECT_EQ("", LIBRARY->combatScripts()->getDescriptionTextID(CombatScriptID::NONE));
	EXPECT_EQ(0, LIBRARY->combatScripts()->getPriority(CombatScriptID::NONE));
}

/// Scripts sharing an event run in priority order, and death stare has to reach the victim before
/// transmutation replaces it with a different creature.
TEST(CombatScriptHandlerTest, PriorityOrdersScriptsOnTheSameEvent)
{
	const auto priorityOf = [](const std::string & name)
	{
		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", name, false);
		EXPECT_TRUE(index.has_value()) << name;
		return index.has_value() ? LIBRARY->combatScripts()->getPriority(CombatScriptID(*index)) : 0;
	};

	EXPECT_LT(priorityOf("deathStare"), priorityOf("transmutation"));
	EXPECT_LT(priorityOf("transmutation"), priorityOf("destruction"));

	// a script that does not care keeps the default, which is what most of them do
	EXPECT_EQ(0, priorityOf("lifeDrain"));
}

}
