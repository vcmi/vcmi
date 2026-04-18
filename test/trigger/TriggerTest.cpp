/*
 * TriggerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../../lib/modding/ModScope.h"
#include "../../lib/json/JsonBonus.h"
#include "lib/mapObjects/CQuest.h"

#include <bonuses/Trigger.h>

class TriggerTest : public ::testing::Test
{
public:
	Trigger triggerFromJson(std::string triggerJson)
	{
		char triggerChar[triggerJson.size() + 1];
		std::strcpy(triggerChar, triggerJson.c_str());
		JsonNode triggerNode(triggerChar, sizeof(triggerChar), "test trigger");
		triggerNode.setModScope(ModScope::scopeGame());
		return JsonUtils::parseTrigger(triggerNode);
	}
};

TEST_F(TriggerTest, ReturnsFalseIfEmptyTriggerSequence)
{
	Trigger trigger = triggerFromJson(R"(
	{
		"eventSequence" : {{}}
	}
	)");

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
}

TEST_F(TriggerTest, ReturnsTrueOnceIfOncePerBattle)
{
	Trigger trigger = triggerFromJson(R"(
	{
		"eventSequence" : ["AFTER_MOVE","BEFORE_ATTACK"],
		"oncePerBattle" : true,
		"continuous" : false
	}
	)");

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::UNIT_SPELLCAST), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), true);

	EXPECT_EQ(trigger.triggered(CombatEventType::WAIT), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), false);
}

TEST_F(TriggerTest, ReturnsTrueMultipleTimesIfNotOncePerBattle)
{
	Trigger trigger = triggerFromJson(R"(
	{
		"eventSequence" : ["AFTER_MOVE","BEFORE_ATTACK"],
		"oncePerBattle" : false,
		"continuous" : false
	}
	)");

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::UNIT_SPELLCAST), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), true);

	EXPECT_EQ(trigger.triggered(CombatEventType::WAIT), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), true);
}

TEST_F(TriggerTest, ReturnsTrueOnlyForContinuousTriggersIfContinuous)
{
	Trigger trigger = triggerFromJson(R"(
	{
		"eventSequence" : ["AFTER_MOVE","BEFORE_ATTACK"],
		"continuous" : true
	}
	)");

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::UNIT_SPELLCAST), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), false);

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), true);
}

TEST_F(TriggerTest, ReturnsTrueForBothAlternativeTriggers)
{
	Trigger trigger = triggerFromJson(R"(
	{
		"eventSequence" : ["AFTER_MOVE","BEFORE_ATTACK|BEFORE_MOVE"],
		"continuous" : true
	}
	)");

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::DEFEND), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::UNIT_SPELLCAST), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), false);

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_ATTACK), true);

	EXPECT_EQ(trigger.triggered(CombatEventType::AFTER_MOVE), false);
	EXPECT_EQ(trigger.triggered(CombatEventType::BEFORE_MOVE), true);
}
