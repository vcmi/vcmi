/*
 * GameSettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

enum class EGameSettings
{
	TEXTS_HERO_CLASS,
	TEXTS_ARTIFACT,
	TEXTS_CREATURE,
	TEXTS_FACTION,
	TEXTS_HERO,
	TEXTS_SPELL,
	TEXTS_OBJECT,
	TEXTS_TERRAIN,
	TEXTS_RIVER,
	TEXTS_ROAD,
	TEXTS_MAP_VERSION,

	MODULE_STACK_EXPERIENCE,
	MODULE_STACK_ARTIFACT,
	MODULE_COMMANDERS,

	BOOL_WINNING_HERO_WITH_NO_TROOPS_RETREATS,
	BOOL_BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE,
	BOOL_NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS,
	BOOL_DWELLINGS_ACCUMULATE_CREATURES,
	BOOL_ALL_CREATURES_GET_DOUBLE_MONTHS,
	BOOL_NEGATIVE_LUCK,

	INT_CREEP_SIZE,
	INT_WEEKLY_GROWTH,
	INT_NEUTRAL_STACK_EXP,
	INT_MAX_BUILDING_PER_TURN,
	INT_MAX_HEROES_AVAILABLE_PER_PLAYER,
	INT_MAX_HEROES_ON_MAP_PER_PLAYER,
	DOUBLE_ATTACK_POINT_DMG_MULTIPLIER,
	DOUBLE_ATTACK_POINTS_DMG_MULTIPLIER_CAP,
	DOUBLE_DEFENSE_POINT_DMG_MULTIPLIER,
	DOUBLE_DEFENSE_POINTS_DMG_MULTIPLIER_CAP,
	VECTOR_HERO_STARTING_ARMY_STACKS_COUNT_CHANCES,
	VECTOR_DEFAULT_BUILDING_SET_DWELLING_CHANCES,

	BONUSES_LIST_GLOBAL,
	BONUSES_LIST_HERO,

	OPTIONS_COUNT
};

class DLL_LINKAGE IGameSettings
{
public:
	virtual const JsonNode & getValue(EGameSettings option) const = 0;

	bool getBoolean(EGameSettings option) const;
	int64_t getInteger(EGameSettings option) const;
	double getDouble(EGameSettings option) const;
};

class DLL_LINKAGE GameSettings final : public IGameSettings
{
	std::vector<JsonNode> gameSettings;

public:
	GameSettings();

	void load(const JsonNode & input);

	const JsonNode & getValue(EGameSettings option) const override;

	template<typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & gameSettings;
	}
};

VCMI_LIB_NAMESPACE_END
