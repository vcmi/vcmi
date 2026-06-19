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

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & mask;
	}
};
