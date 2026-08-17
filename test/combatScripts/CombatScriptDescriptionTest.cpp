/*
 * CombatScriptDescriptionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"

namespace test
{

/// Every scripted ability shares the COMBAT_EVENT_TRIGGER bonus type, so its description has to
/// come from the script rather than from the bonus type.
class CombatScriptDescriptionTest : public ::testing::Test
{
public:
	static std::shared_ptr<Bonus> triggerBonus(const std::string & script, int value, const JsonNode & eventParameters = JsonNode())
	{
		auto scriptID = ScriptID(*LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "script", script, false));

		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_EVENT_TRIGGER, BonusSource::CREATURE_ABILITY, value, BonusSourceID(), BonusSubtypeID(scriptID));
		bonus->parameters = std::make_shared<BonusParameters>(eventParameters);
		return bonus;
	}
};

/// The magnitude of a scripted ability lives in the bonus value, like every other bonus.
TEST_F(CombatScriptDescriptionTest, ValueIsSubstituted)
{
	auto description = LIBRARY->bth->bonusToString(triggerBonus("lifeDrain", 100));

	EXPECT_NE(description.find("100%"), std::string::npos) << description;
	EXPECT_EQ(description.find("${"), std::string::npos) << description;
}

/// Everything that is not a magnitude stays a named script parameter.
TEST_F(CombatScriptDescriptionTest, ParametersAreSubstitutedByName)
{
	JsonNode parameters;
	parameters["creature"].String() = "core:woodElf";

	auto description = LIBRARY->bth->bonusToString(triggerBonus("summonGuardians", 50, parameters));

	EXPECT_NE(description.find("50%"), std::string::npos) << description;
	EXPECT_EQ(description.find("${"), std::string::npos) << description;
	EXPECT_EQ(description.find("core:woodElf"), std::string::npos) << description;
}

/// A parameter is resolved as whatever its schema declares it to be, rather than against every
/// service in turn - a json key can name a creature and a spell at once, and only the script knows
/// which of the two it meant.
TEST_F(CombatScriptDescriptionTest, ParameterIsResolvedAsItsDeclaredType)
{
	const auto * creature = LIBRARY->creatures()->getByName("core:woodElf");
	ASSERT_NE(creature, nullptr);

	// the spell parameter of this script is handed a creature, which no spell is named after
	JsonNode parameters;
	parameters["spell"].String() = "core:woodElf";

	auto description = LIBRARY->bth->bonusToString(triggerBonus("enchanted", 0, parameters));

	EXPECT_EQ(description.find(creature->getNamePluralTranslated()), std::string::npos) << description;
}

/// Regression guard: the shared bonus type has no bonuses.json entry, so it is hidden by default
/// and scripted abilities used to render as nothing at all.
TEST_F(CombatScriptDescriptionTest, DescriptionIsNotEmpty)
{
	auto description = LIBRARY->bth->bonusToString(triggerBonus("soulSteal", 1));

	EXPECT_FALSE(description.empty());
	EXPECT_EQ(description.find("core.script."), std::string::npos) << description;
}

}
