/*
 * MapDifficulty.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../constants/StringConstants.h"

enum class EMapDifficulty : uint8_t
{
	EASY = 0,
	NORMAL = 1,
	HARD = 2,
	EXPERT = 3,
	IMPOSSIBLE = 4,
	COUNT = 5
};

class MapDifficultySet
{
	static constexpr uint8_t allDifficultiesMask = (1 << static_cast<uint8_t>(EMapDifficulty::COUNT)) - 1;

	uint8_t mask = allDifficultiesMask;
public:
	MapDifficultySet() = default;
	explicit MapDifficultySet(uint8_t mask)
		:mask(mask)
	{}

	bool contains(const	EMapDifficulty & difficulty) const
	{
		return (1 << static_cast<uint8_t>(difficulty)) & mask;
	}

	/// true in the default state, when every difficulty is allowed (i.e. no restriction)
	bool allowsAll() const
	{
		return mask == allDifficultiesMask;
	}

	bool operator==(const MapDifficultySet & other) const
	{
		return mask == other.mask;
	}

	/// Chess names of the contained difficulties; empty when every difficulty is allowed.
	std::vector<std::string> toNames() const
	{
		std::vector<std::string> names;
		if(!allowsAll())
			for(size_t i = 0; i < std::size(GameConstants::DIFFICULTY_NAMES); ++i)
				if(contains(static_cast<EMapDifficulty>(i)))
					names.push_back(GameConstants::DIFFICULTY_NAMES[i]);
		return names;
	}

	/// Inverse of toNames(); an empty list means "no restriction" (default mask).
	static MapDifficultySet fromNames(const std::vector<std::string> & names)
	{
		uint8_t mask = 0;
		for(const auto & name : names)
			for(size_t i = 0; i < std::size(GameConstants::DIFFICULTY_NAMES); ++i)
				if(GameConstants::DIFFICULTY_NAMES[i] == name)
					mask |= (1u << i);
		return names.empty() ? MapDifficultySet() : MapDifficultySet(mask);
	}

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & mask;
	}
};
