/*
 * GameSettingsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../../lib/GameSettings.h"

namespace test
{
	class GameSettingsTest : public testing::Test
	{
	public:
		GameSettings gameSettings;
		JsonNode settingsData;
		JsonNode overrideSettingsData;

		void SetUp() override
		{
		    settingsData = JsonNode(JsonPath::builtin("config/gameConfig"));
			overrideSettingsData = JsonNode(JsonPath::builtin("test/gameSettingsOverride"));
		}
	};

	TEST_F(GameSettingsTest, LoadsAndOverridesDataCorrectly)
	{
		gameSettings.loadBase(settingsData["settings"]);

		//CREATURES_WEEKLY_GROWTH_PERCENT represents arbitrary config value equal to 10 that is unlikely to change
		int64_t configValueThatEquals10 = gameSettings.getValue(EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT).Integer();

		gameSettings.loadOverrides(overrideSettingsData);
		int64_t configValueThatEquals50 = gameSettings.getValue(EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT).Integer();

		gameSettings.addOverride(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP, JsonNode(9999));
		int64_t configValueThatEquals9999 = gameSettings.getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP);

		EXPECT_EQ(configValueThatEquals10, 10);
		EXPECT_EQ(configValueThatEquals50, 50);
		EXPECT_EQ(configValueThatEquals9999, 9999);
	}
}

