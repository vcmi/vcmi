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
#include "json/JsonUtils.h"

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

int IGameSettings::getVectorValue(EGameSettings option, size_t index) const
{
	return getValue(option)[index].Integer();
}

GameSettings::GameSettings() = default;
GameSettings::~GameSettings() = default;

const std::vector<GameSettings::SettingOption> GameSettings::settingProperties = {
		{EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION,                    "banks",     "showGuardsComposition"                },
		{EGameSettings::BONUSES_GLOBAL,                                   "bonuses",   "global"                               },
		{EGameSettings::BONUSES_PER_HERO,                                 "bonuses",   "perHero"                              },
		{EGameSettings::CAMPAIGN_UNLOCK_ALL,                              "campaign",  "unlockAll"                            },
		{EGameSettings::COMBAT_ABILITY_BIAS,                              "combat",    "abilityBias"                          },
		{EGameSettings::COMBAT_AREA_SHOT_CAN_TARGET_EMPTY_HEX,            "combat",    "areaShotCanTargetEmptyHex"            },
		{EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FORMULA,               "combat",    "attackPointDamageFormula"             },
		{EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FORMULA,              "combat",    "defensePointDamageFormula"            },
		{EGameSettings::COMBAT_GOOD_MORALE_CHANCE,                        "combat",    "goodMoraleChance"                     },
		{EGameSettings::COMBAT_BAD_MORALE_CHANCE,                         "combat",    "badMoraleChance"                      },
		{EGameSettings::COMBAT_MORALE_DICE_SIZE,                          "combat",    "moraleDiceSize"                       },
		{EGameSettings::COMBAT_MORALE_BIAS,                               "combat",    "moraleBias"                           },
		{EGameSettings::COMBAT_GOOD_LUCK_CHANCE,                          "combat",    "goodLuckChance"                       },
		{EGameSettings::COMBAT_BAD_LUCK_CHANCE,                           "combat",    "badLuckChance"                        },
		{EGameSettings::COMBAT_LUCK_DICE_SIZE,                            "combat",    "luckDiceSize"                         },
		{EGameSettings::COMBAT_LUCK_BIAS,                                 "combat",    "luckBias"                             },
		{EGameSettings::COMBAT_LAYOUTS,                                   "combat",    "layouts"                              },
		{EGameSettings::COMBAT_ONE_HEX_TRIGGERS_OBSTACLES,                "combat",    "oneHexTriggersObstacles"              },
		{EGameSettings::CREATURES_ALLOW_ALL_FOR_DOUBLE_MONTH,             "creatures", "allowAllForDoubleMonth"               },
		{EGameSettings::CREATURES_ALLOW_JOINING_FOR_FREE,                 "creatures", "allowJoiningForFree"                  },
		{EGameSettings::CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS,             "creatures", "allowRandomSpecialWeeks"              },
		{EGameSettings::CREATURES_DAILY_STACK_EXPERIENCE,                 "creatures", "dailyStackExperience"                 },
		{EGameSettings::CREATURES_JOINING_PERCENTAGE,                     "creatures", "joiningPercentage"                    },
		{EGameSettings::CREATURES_WEEKLY_GROWTH_CAP,                      "creatures", "weeklyGrowthCap"                      },
		{EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT,                  "creatures", "weeklyGrowthPercent"                  },
		{EGameSettings::DWELLINGS_ACCUMULATE_WHEN_NEUTRAL,                "dwellings", "accumulateWhenNeutral"                },
		{EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED,                  "dwellings", "accumulateWhenOwned"                  },
		{EGameSettings::DWELLINGS_MERGE_ON_RECRUIT,                       "dwellings", "mergeOnRecruit"                       },
		{EGameSettings::HEROES_BACKPACK_CAP,                              "heroes",    "backpackSize"                         },
		{EGameSettings::HEROES_BASE_SCOUNTING_RANGE,                      "heroes",    "baseScoutingRange"                    },
		{EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS,                    "heroes",    "minimalPrimarySkills"                 },
		{EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP,                     "heroes",    "perPlayerOnMapCap"                    },
		{EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP,                      "heroes",    "perPlayerTotalCap"                    },
		{EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS,             "heroes",    "retreatOnWinWithoutTroops"            },
		{EGameSettings::HEROES_STARTING_STACKS_CHANCES,                   "heroes",    "startingStackChances"                 },
		{EGameSettings::HEROES_TAVERN_INVITE,                             "heroes",    "tavernInvite"                         },
		{EGameSettings::HEROES_MOVEMENT_COST_BASE,                        "heroes",    "movementCostBase"                     },
		{EGameSettings::HEROES_MOVEMENT_POINTS_LAND,                      "heroes",    "movementPointsLand"                   },
		{EGameSettings::HEROES_MOVEMENT_POINTS_SEA,                       "heroes",    "movementPointsSea"                    },
		{EGameSettings::HEROES_SKILL_PER_HERO,                            "heroes",    "skillPerHero"                         },
		{EGameSettings::HEROES_SPECIALTY_CREATURE_GROWTH,                 "heroes",    "specialtyCreatureGrowth"              },
		{EGameSettings::HEROES_SPECIALTY_SECONDARY_SKILL_GROWTH,          "heroes",    "specialtySecondarySkillGrowth"        },
		{EGameSettings::LEVEL_UP_TOTAL_SKILLS_AMOUNT,                     "heroes",    "levelupTotalSkillsAmount"             },
		{EGameSettings::LEVEL_UP_UPGRADED_SKILLS_AMOUNT,                  "heroes",    "levelupUpgradedSkillsAmount"          },
		{EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE,                     "mapFormat", "armageddonsBlade"                     },
		{EGameSettings::MAP_FORMAT_CHRONICLES,                            "mapFormat", "chronicles"                           },
		{EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS,                     "mapFormat", "hornOfTheAbyss"                       },
		{EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS,                   "mapFormat", "inTheWakeOfGods"                      },
		{EGameSettings::MAP_FORMAT_JSON_VCMI,                             "mapFormat", "jsonVCMI"                             },
		{EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA,                "mapFormat", "restorationOfErathia"                 },
		{EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH,                       "mapFormat", "shadowOfDeath"                        },
		{EGameSettings::MAP_OBJECTS_H3_BUG_QUEST_TAKES_ENTIRE_ARMY,       "mapObjects","h3BugQuestTakesEntireArmy"            },
		{EGameSettings::MARKETS_BLACK_MARKET_RESTOCK_PERIOD,              "markets",   "blackMarketRestockPeriod"             },
		{EGameSettings::MARKETS_UNIVERSITY_GOLD_COST,                     "markets",   "universityGoldCost"                   },
		{EGameSettings::MODULE_COMMANDERS,                                "modules",   "commanders"                           },
		{EGameSettings::MODULE_STACK_ARTIFACT,                            "modules",   "stackArtifact"                        },
		{EGameSettings::MODULE_STACK_EXPERIENCE,                          "modules",   "stackExperience"                      },
		{EGameSettings::PATHFINDER_IGNORE_GUARDS,                         "pathfinder", "ignoreGuards"                        },
		{EGameSettings::PATHFINDER_ORIGINAL_FLY_RULES,                    "pathfinder", "originalFlyRules"                    },
		{EGameSettings::PATHFINDER_USE_BOAT,                              "pathfinder", "useBoat"                             },
		{EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_RANDOM,           "pathfinder", "useMonolithOneWayRandom"             },
		{EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_UNIQUE,           "pathfinder", "useMonolithOneWayUnique"             },
		{EGameSettings::PATHFINDER_USE_MONOLITH_TWO_WAY,                  "pathfinder", "useMonolithTwoWay"                   },
		{EGameSettings::PATHFINDER_USE_WHIRLPOOL,                         "pathfinder", "useWhirlpool"                        },
		{EGameSettings::RESOURCES_WEEKLY_BONUSES_AI,                      "resources", "weeklyBonusesAI"                      },
		{EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS,            "spells",    "dimensionDoorTriggersGuards"          },
		{EGameSettings::SPELLS_TOMES_GRANT_BANNED_SPELLS,                 "spells",    "tomesGrantBannedSpells"               },
		{EGameSettings::TEXTS_ARTIFACT,                                   "textData",  "artifact"                             },
		{EGameSettings::TEXTS_CREATURE,                                   "textData",  "creature"                             },
		{EGameSettings::TEXTS_FACTION,                                    "textData",  "faction"                              },
		{EGameSettings::TEXTS_HERO,                                       "textData",  "hero"                                 },
		{EGameSettings::TEXTS_HERO_CLASS,                                 "textData",  "heroClass"                            },
		{EGameSettings::TEXTS_OBJECT,                                     "textData",  "object"                               },
		{EGameSettings::TEXTS_RIVER,                                      "textData",  "river"                                },
		{EGameSettings::TEXTS_ROAD,                                       "textData",  "road"                                 },
		{EGameSettings::TEXTS_SPELL,                                      "textData",  "spell"                                },
		{EGameSettings::TEXTS_TERRAIN,                                    "textData",  "terrain"                              },
		{EGameSettings::TOWNS_BASE_SCOUNTING_RANGE,                       "towns",     "baseScoutingRange"                    },
		{EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP,                     "towns",     "buildingsPerTurnCap"                  },
		{EGameSettings::TOWNS_STARTING_DWELLING_CHANCES,                  "towns",     "startingDwellingChances"              },
		{EGameSettings::TOWNS_SPELL_RESEARCH,                             "towns",     "spellResearch"                        },
		{EGameSettings::TOWNS_SPELL_RESEARCH_COST,                        "towns",     "spellResearchCost"                    },
		{EGameSettings::TOWNS_SPELL_RESEARCH_PER_DAY,                     "towns",     "spellResearchPerDay"                  },
		{EGameSettings::TOWNS_SPELL_RESEARCH_COST_EXPONENT_PER_RESEARCH,  "towns",     "spellResearchCostExponentPerResearch" },
		{EGameSettings::INTERFACE_PLAYER_COLORED_BACKGROUND,              "interface", "playerColoredBackground"              },
	};

void GameSettings::loadBase(const JsonNode & input)
{
	JsonUtils::validate(input, "vcmi:gameSettings", input.getModScope());

	for(const auto & option : settingProperties)
	{
		const JsonNode & optionValue = input[option.group][option.key];
		size_t index = static_cast<size_t>(option.setting);

		if(optionValue.isNull())
			continue;

		JsonUtils::mergeCopy(baseSettings[index], optionValue);
	}
	actualSettings = baseSettings;
}

void GameSettings::loadOverrides(const JsonNode & input)
{
	for(const auto & option : settingProperties)
	{
		const JsonNode & optionValue = input[option.group][option.key];
		if (!optionValue.isNull())
			addOverride(option.setting, optionValue);
	}
}

void GameSettings::addOverride(EGameSettings option, const JsonNode & input)
{
	size_t index = static_cast<size_t>(option);

	overridenSettings[index] = input;
	JsonNode newValue = baseSettings[index];
	JsonUtils::mergeCopy(newValue, input);
	actualSettings[index] = newValue;
}

const JsonNode & GameSettings::getValue(EGameSettings option) const
{
	auto index = static_cast<size_t>(option);

	assert(!actualSettings.at(index).isNull());
	return actualSettings.at(index);
}

JsonNode GameSettings::getFullConfig() const
{
	JsonNode result;
	for(const auto & option : settingProperties)
		result[option.group][option.key] = getValue(option.setting);

	return result;
}

JsonNode GameSettings::getAllOverrides() const
{
	JsonNode result;
	for(const auto & option : settingProperties)
	{
		const JsonNode & value = overridenSettings[static_cast<int32_t>(option.setting)];
		if (!value.isNull())
			result[option.group][option.key] = value;
	}

	return result;
}

VCMI_LIB_NAMESPACE_END
