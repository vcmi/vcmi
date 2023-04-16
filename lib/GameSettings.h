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
	BONUSES_GLOBAL,
	BONUSES_PER_HERO,
	COMBAT_ATTACK_POINT_DAMAGE_FACTOR,
	COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP,
	COMBAT_BAD_LUCK_DICE,
	COMBAT_BAD_MORALE_DICE,
	COMBAT_DEFENSE_POINT_DAMAGE_FACTOR,
	COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP,
	COMBAT_GOOD_LUCK_DICE,
	COMBAT_GOOD_MORALE_DICE,
	CREATURES_ALLOW_ALL_FOR_DOUBLE_MONTH,
	CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS,
	CREATURES_DAILY_STACK_EXPERIENCE,
	CREATURES_WEEKLY_GROWTH_CAP,
	CREATURES_WEEKLY_GROWTH_PERCENT,
	DWELLINGS_ACCUMULATE_WHEN_NEUTRAL,
	DWELLINGS_ACCUMULATE_WHEN_OWNED,
	HEROES_PER_PLAYER_ON_MAP_CAP,
	HEROES_PER_PLAYER_TOTAL_CAP,
	HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS,
	HEROES_STARTING_STACKS_CHANCES,
	HEROES_BACKPACK_CAP,
	MARKETS_BLACK_MARKET_RESTOCK_PERIOD,
	MODULE_COMMANDERS,
	MODULE_STACK_ARTIFACT,
	MODULE_STACK_EXPERIENCE,
	TEXTS_ARTIFACT,
	TEXTS_CREATURE,
	TEXTS_FACTION,
	TEXTS_HERO,
	TEXTS_HERO_CLASS,
	TEXTS_MAP_VERSION,
	TEXTS_OBJECT,
	TEXTS_RIVER,
	TEXTS_ROAD,
	TEXTS_SPELL,
	TEXTS_TERRAIN,
	TOWNS_BUILDINGS_PER_TURN_CAP,
	TOWNS_STARTING_DWELLING_CHANCES,
	COMBAT_ONE_HEX_TRIGGERS_OBSTACLES,

	OPTIONS_COUNT
};

class DLL_LINKAGE IGameSettings
{
public:
	virtual const JsonNode & getValue(EGameSettings option) const = 0;

	bool getBoolean(EGameSettings option) const;
	int64_t getInteger(EGameSettings option) const;
	double getDouble(EGameSettings option) const;
	std::vector<int> getVector(EGameSettings option) const;
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
