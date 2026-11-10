/*
 * StackExperienceRanksTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/GameSettings.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/army/CStackInstance.h"
#include "../mock/mock_IGameInfoCallback.h"

class StackExperienceSettingsCallback : public IGameInfoCallbackMock
{
public:
	GameSettings settings;

	const IGameSettings & getSettings() const override { return settings; }
};

/// Experience tables have rows for creature levels starting from 1, and the last row also covers all higher levels
TEST(StackExperienceRanksTest, TableRowSelectedByCreatureLevel)
{
	const std::string config = R"({
		"modules" : { "stackExperience" : true },
		"creatures" : {
			"stackExperienceRanks" : [
				[   10,   20,   30,   40,   50,   60,   70,   80,   90,  100 ],
				[ 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000 ]
			],
			"stackExperienceLimit" : [ 200, 20000 ],
			"stackExperiencePerBattle" : [ 50, 10 ],
			"stackExperienceAfterUpgrade" : 50
		}
	})";

	StackExperienceSettingsCallback cb;
	cb.settings.loadBase(JsonNode(config.data(), config.size(), "test"));

	auto rankWithExperience = [&cb](CreatureID creature, TExpType experience)
	{
		CStackInstance stack(&cb, creature, 1);
		stack.giveTotalStackExperience(experience);
		return stack.getExpRank();
	};

	// level 1 uses first row, level 0 uses it as well
	EXPECT_EQ(rankWithExperience(CreatureID::TROGLODYTES, 25), 2);
	EXPECT_EQ(rankWithExperience(CreatureID::CATAPULT, 25), 2);
	// levels 2 and above use last row, including levels beyond the table
	EXPECT_EQ(rankWithExperience(CreatureID::AIR_ELEMENTAL, 25), 0);
	EXPECT_EQ(rankWithExperience(CreatureID::AZURE_DRAGON, 2500), 2);
	EXPECT_EQ(rankWithExperience(CreatureID::AZURE_DRAGON, 10000), 10);

	CStackInstance dragons(&cb, CreatureID::AZURE_DRAGON, 1);
	dragons.giveAverageStackExperience(1000000);
	EXPECT_EQ(dragons.getTotalExperience(), 2000); // 10% of 20000 in a single battle
	EXPECT_EQ(dragons.getExperienceForRank(3), 3000);
	EXPECT_EQ(dragons.getMaxExperiencePerUnit(), 20000);
}
