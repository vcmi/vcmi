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
	static std::shared_ptr<Bonus> triggerBonus(const std::string & script, const JsonNode & eventParameters)
	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_EVENT_TRIGGER, BonusSource::CREATURE_ABILITY, 0, BonusSourceID());

		BonusParametersCombatScript data;
		data.eventParameters = eventParameters;

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", script, false);
		if(index.has_value())
			data.eventScript = CombatScriptID(*index);

		bonus->parameters = std::make_shared<BonusParameters>(data);
		return bonus;
	}
};

TEST_F(CombatScriptDescriptionTest, ParametersAreSubstitutedByName)
{
	JsonNode parameters;
	parameters["percentage"].Integer() = 100;

	auto description = LIBRARY->bth->bonusToString(triggerBonus("lifeDrain", parameters));

	EXPECT_NE(description.find("100%"), std::string::npos) << description;
	EXPECT_EQ(description.find("${"), std::string::npos) << description;
}

/// Regression guard: the shared bonus type has no bonuses.json entry, so it is hidden by default
/// and scripted abilities used to render as nothing at all.
TEST_F(CombatScriptDescriptionTest, DescriptionIsNotEmpty)
{
	JsonNode parameters;
	parameters["creaturesPerKill"].Integer() = 1;

	auto description = LIBRARY->bth->bonusToString(triggerBonus("soulSteal", parameters));

	EXPECT_FALSE(description.empty());
	EXPECT_EQ(description.find("core.combatScript."), std::string::npos) << description;
}

/// A bonus whose script comes from a mod that is no longer installed must render as nothing.
TEST_F(CombatScriptDescriptionTest, UnresolvedScriptHasNoDescription)
{
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_EVENT_TRIGGER, BonusSource::CREATURE_ABILITY, 0, BonusSourceID());
	bonus->parameters = std::make_shared<BonusParameters>(BonusParametersCombatScript());

	EXPECT_EQ(LIBRARY->bth->bonusToString(bonus), "");
}

}
