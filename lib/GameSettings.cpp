/*
 * GameSettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameSettings.h"

#include "JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

bool IGameSettings::getBoolean(EGameSettings option) const
{
	return getValue(option).Bool();
}

int64_t IGameSettings::getInteger(EGameSettings option) const
{
	return getValue(option).Integer();
}

double IGameSettings::getDouble(EGameSettings option) const
{
	return getValue(option).Float();
}

GameSettings::GameSettings():
	gameSettings(static_cast<size_t>(EGameSettings::OPTIONS_COUNT))
{
}

void GameSettings::load(const JsonNode & input)
{
	static std::array<std::array<std::string, 2>, 34> optionPath =
	{ {
		{"textData", "heroClass"},
		{"textData", "artifact"},
		{"textData", "creature"},
		{"textData", "faction"},
		{"textData", "hero"},
		{"textData", "spell"},
		{"textData", "object"},
		{"textData", "terrain"},
		{"textData", "river"},
		{"textData", "road"},
		{"textData", "mapVersion"},
		{"modules", "STACK_EXPERIENCE"},
		{"modules", "STACK_ARTIFACTS"},
		{"modules", "COMMANDERS"},
		{"hardcodedFeatures", "DWELLINGS_ACCUMULATE_CREATURES"},
		{"hardcodedFeatures", "NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS"},
		{"hardcodedFeatures", "BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE"},
		{"hardcodedFeatures", "WINNING_HERO_WITH_NO_TROOPS_RETREATS"},
		{"hardcodedFeatures", "ALL_CREATURES_GET_DOUBLE_MONTHS"},
		{"hardcodedFeatures", "NEGATIVE_LUCK"},
		{"hardcodedFeatures", "CREEP_SIZE"},
		{"hardcodedFeatures", "WEEKLY_GROWTH"},
		{"hardcodedFeatures", "NEUTRAL_STACK_EXP"},
		{"hardcodedFeatures", "MAX_BUILDING_PER_TURN"},
		{"hardcodedFeatures", "MAX_HEROES_AVAILABLE_PER_PLAYER"},
		{"hardcodedFeatures", "MAX_HEROES_ON_MAP_PER_PLAYER"},
		{"hardcodedFeatures", "ATTACK_POINT_DMG_MULTIPLIER"},
		{"hardcodedFeatures", "ATTACK_POINTS_DMG_MULTIPLIER_CAP"},
		{"hardcodedFeatures", "DEFENSE_POINT_DMG_MULTIPLIER"},
		{"hardcodedFeatures", "DEFENSE_POINTS_DMG_MULTIPLIER_CAP"},
		{"hardcodedFeatures", "HERO_STARTING_ARMY_STACKS_COUNT_CHANCES"},
		{"hardcodedFeatures", "DEFAULT_BUILDING_SET_DWELLING_CHANCES"},
		{"bonuses", "global"},
		{"bonuses", "heroes"},
	} };
	static_assert(static_cast<size_t>(EGameSettings::OPTIONS_COUNT) == optionPath.size(), "Missing option path");

	for (size_t i = 0; i < optionPath.size(); ++i)
	{
		const std::string & group = optionPath[i][0];
		const std::string & key = optionPath[i][1];

		const JsonNode & optionValue = input[group][key];

		if (!optionValue.isNull())
			gameSettings[i] = optionValue;
	}
}

const JsonNode & GameSettings::getValue(EGameSettings option) const
{
	assert(option < EGameSettings::OPTIONS_COUNT);
	auto index = static_cast<size_t>(option);

	return gameSettings[index];
}


VCMI_LIB_NAMESPACE_END
