/*
 * BonusCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BonusSelector.h"

enum class BonusCacheMode : int8_t
{
	VALUE, // total value of bonus will be cached
	PRESENCE, // presence of bonus will be cached
};

/// Internal base class with no own cache
class BonusCacheBase
{
protected:
	const IBonusBearer * target;

	explicit BonusCacheBase(const IBonusBearer * target):
		target(target)
	{}

	struct BonusCacheEntry
	{
		std::atomic<int64_t> version = 0;
		std::atomic<int64_t> value = 0;
	};

	int getBonusValueImpl(BonusCacheEntry & currentValue, const CSelector & selector, BonusCacheMode) const;
};

/// Cache that tracks a single query to bonus system
class BonusValueCache : public BonusCacheBase
{
	CSelector selector;
	mutable BonusCacheEntry value;
public:
	BonusValueCache(const IBonusBearer * target, const CSelector selector);
	int getValue() const;
};

/// Cache that can track a list of queries to bonus system
template<typename EnumType, size_t SIZE>
class BonusValuesArrayCache : public BonusCacheBase
{
public:
	using SelectorsArray = std::array<const CSelector, SIZE>;

	BonusValuesArrayCache(const IBonusBearer * target, const SelectorsArray * selectors)
		: BonusCacheBase(target)
		, selectors(selectors)
	{}

	int getBonusValue(EnumType which) const
	{
		auto index = static_cast<size_t>(which);
		return getBonusValueImpl(cache[index], (*selectors)[index], BonusCacheMode::VALUE);
	}

	int hasBonus(EnumType which) const
	{
		auto index = static_cast<size_t>(which);
		return getBonusValueImpl(cache[index], (*selectors)[index], BonusCacheMode::PRESENCE);
	}

private:
	using CacheArray = std::array<BonusCacheEntry, SIZE>;

	const SelectorsArray * selectors;
	mutable CacheArray cache;
};

/// Cache that tracks values of primary skill values in bonus system
class PrimarySkillsCache
{
	const IBonusBearer * target;
	mutable std::atomic<int64_t> version = 0;
	mutable std::array<std::atomic<int32_t>, 4> skills;

	void update() const;
public:
	PrimarySkillsCache(const IBonusBearer * target);

	const std::array<std::atomic<int32_t>, 4> & getSkills() const;
};

/// Cache that tracks values for different values of bonus duration
class BonusCachePerTurn : public BonusCacheBase
{
	static constexpr int cachedTurns = 8;

	const CSelector selector;
	mutable TConstBonusListPtr bonusList;
	mutable std::mutex bonusListMutex;
	mutable std::atomic<int64_t> bonusListVersion = 0;
	mutable std::array<BonusCacheEntry, cachedTurns> cache;
	const BonusCacheMode mode;

	int getValueUncached(int turns) const;
public:
	BonusCachePerTurn(const IBonusBearer * target, const CSelector & selector, BonusCacheMode mode)
		: BonusCacheBase(target)
		, selector(selector)
		, mode(mode)
	{}

	int getValue(int turns) const;
};
