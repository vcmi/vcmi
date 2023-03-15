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

std::vector<int> IGameSettings::getVector(EGameSettings option) const
{
	return getValue(option).convertTo<std::vector<int>>();
}

GameSettings::GameSettings()
	: gameSettings(static_cast<size_t>(EGameSettings::OPTIONS_COUNT))
{
}

void GameSettings::load(const JsonNode & input)
{
	struct SettingOption
	{
		EGameSettings setting;
		std::string group;
		std::string key;
	};

	static const std::vector<SettingOption> optionPath = {
		{EGameSettings::BONUSES_GLOBAL,                         "bonuses",   "global"                     },
		{EGameSettings::BONUSES_PER_HERO,                       "bonuses",   "perHero"                    },
		{EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR,      "combat",    "attackPointDamageFactor"    },
		{EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP,  "combat",    "attackPointDamageFactorCap" },
		{EGameSettings::COMBAT_BAD_LUCK_DICE,                   "combat",    "badLuckDice"                },
		{EGameSettings::COMBAT_BAD_MORALE_DICE,                 "combat",    "badMoraleDice"              },
		{EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR,     "combat",    "defensePointDamageFactor"   },
		{EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP, "combat",    "defensePointDamageFactorCap"},
		{EGameSettings::COMBAT_GOOD_LUCK_DICE,                  "combat",    "goodLuckDice"               },
		{EGameSettings::COMBAT_GOOD_MORALE_DICE,                "combat",    "goodMoraleDice"             },
		{EGameSettings::CREATURES_ALLOW_ALL_FOR_DOUBLE_MONTH,   "creatures", "allowAllForDoubleMonth"     },
		{EGameSettings::CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS,   "creatures", "allowRandomSpecialWeeks"    },
		{EGameSettings::CREATURES_DAILY_STACK_EXPERIENCE,       "creatures", "dailyStackExperience"       },
		{EGameSettings::CREATURES_WEEKLY_GROWTH_CAP,            "creatures", "weeklyGrowthCap"            },
		{EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT,        "creatures", "weeklyGrowthPercent"        },
		{EGameSettings::DWELLINGS_ACCUMULATE_WHEN_NEUTRAL,      "dwellings", "accumulateWhenNeutral"      },
		{EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED,        "dwellings", "accumulateWhenOwned"        },
		{EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP,           "heroes",    "perPlayerOnMapCap"          },
		{EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP,            "heroes",    "perPlayerTotalCap"          },
		{EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS,   "heroes",    "retreatOnWinWithoutTroops"  },
		{EGameSettings::HEROES_STARTING_STACKS_CHANCES,         "heroes",    "startingStackChances"       },
		{EGameSettings::MARKETS_BLACK_MARKET_RESTOCK_PERIOD,    "markets",   "blackMarketRestockPeriod"   },
		{EGameSettings::MODULE_COMMANDERS,                      "modules",   "commanders"                 },
		{EGameSettings::MODULE_STACK_ARTIFACT,                  "modules",   "stackArtifact"              },
		{EGameSettings::MODULE_STACK_EXPERIENCE,                "modules",   "stackExperience"            },
		{EGameSettings::TEXTS_ARTIFACT,                         "textData",  "artifact"                   },
		{EGameSettings::TEXTS_CREATURE,                         "textData",  "creature"                   },
		{EGameSettings::TEXTS_FACTION,                          "textData",  "faction"                    },
		{EGameSettings::TEXTS_HERO,                             "textData",  "hero"                       },
		{EGameSettings::TEXTS_HERO_CLASS,                       "textData",  "heroClass"                  },
		{EGameSettings::TEXTS_MAP_VERSION,                      "textData",  "mapVersion"                 },
		{EGameSettings::TEXTS_OBJECT,                           "textData",  "object"                     },
		{EGameSettings::TEXTS_RIVER,                            "textData",  "river"                      },
		{EGameSettings::TEXTS_ROAD,                             "textData",  "road"                       },
		{EGameSettings::TEXTS_SPELL,                            "textData",  "spell"                      },
		{EGameSettings::TEXTS_TERRAIN,                          "textData",  "terrain"                    },
		{EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP,           "towns",     "buildingsPerTurnCap"        },
		{EGameSettings::TOWNS_STARTING_DWELLING_CHANCES,        "towns",     "startingDwellingChances"    },
	};

	for(const auto & option : optionPath)
	{
		const JsonNode & optionValue = input[option.group][option.key];

		if(!optionValue.isNull())
			gameSettings[static_cast<size_t>(option.setting)] = optionValue;
	}
}

const JsonNode & GameSettings::getValue(EGameSettings option) const
{
	assert(option < EGameSettings::OPTIONS_COUNT);
	auto index = static_cast<size_t>(option);

	assert(!gameSettings[index].isNull());
	return gameSettings[index];
}

VCMI_LIB_NAMESPACE_END
