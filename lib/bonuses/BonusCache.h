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

VCMI_LIB_NAMESPACE_BEGIN

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
		std::atomic<int32_t> version = 0;
		std::atomic<int32_t> value = 0;

		BonusCacheEntry() = default;
		BonusCacheEntry(const BonusCacheEntry & other)
			: version(other.version.load())
			, value(other.value.load())
		{
		}
		BonusCacheEntry & operator =(const BonusCacheEntry & other)
		{
			version = other.version.load();
			value = other.value.load();
			return *this;
		}
	};

	int getBonusValueImpl(BonusCacheEntry & currentValue, const CSelector & selector, BonusCacheMode) const;
};

/// Cache that tracks a single query to bonus system
class BonusValueCache : public BonusCacheBase
{
	CSelector selector;
	mutable BonusCacheEntry value;
public:
	BonusValueCache(const IBonusBearer * target, const CSelector & selector);
	int getValue() const;
	bool hasBonus() const;
};

/// Cache that can track a list of queries to bonus system
template<size_t SIZE>
class BonusValuesArrayCache : public BonusCacheBase
{
public:
	using SelectorsArray = std::array<const CSelector, SIZE>;

	BonusValuesArrayCache(const IBonusBearer * target, const SelectorsArray * selectors)
		: BonusCacheBase(target)
		, selectors(selectors)
	{}

	int getBonusValue(int index) const
	{
		return getBonusValueImpl(cache[index], (*selectors)[index], BonusCacheMode::VALUE);
	}

	int hasBonus(int index) const
	{
		return getBonusValueImpl(cache[index], (*selectors)[index], BonusCacheMode::PRESENCE);
	}

private:
	using CacheArray = std::array<BonusCacheEntry, SIZE>;

	const SelectorsArray * selectors;
	mutable CacheArray cache;
};

class UnitBonusValuesProxy
{
public:
	enum ECacheKeys : int8_t
	{
		TOTAL_ATTACKS_MELEE,
		TOTAL_ATTACKS_RANGED,

		MIN_DAMAGE_MELEE,
		MIN_DAMAGE_RANGED,
		MAX_DAMAGE_MELEE,
		MAX_DAMAGE_RANGED,

		ATTACK_MELEE,
		ATTACK_RANGED,

		DEFENCE_MELEE,
		DEFENCE_RANGED,

		IN_FRENZY,
		HYPNOTIZED,
		FORGETFULL,
		HAS_FREE_SHOOTING,
		STACK_HEALTH,
		INVINCIBLE,

		CLONE_MARKER,

		TOTAL_KEYS,
	};
	static constexpr size_t KEYS_COUNT = static_cast<size_t>(ECacheKeys::TOTAL_KEYS);

	using SelectorsArray = BonusValuesArrayCache<KEYS_COUNT>::SelectorsArray;

	UnitBonusValuesProxy(const IBonusBearer * Target):
		cache(Target, generateSelectors())
	{}

	int getBonusValue(ECacheKeys which) const
	{
		auto index = static_cast<size_t>(which);
		return cache.getBonusValue(index);
	}

	int hasBonus(ECacheKeys which) const
	{
		auto index = static_cast<size_t>(which);
		return cache.hasBonus(index);
	}

private:
	const SelectorsArray * generateSelectors();

	BonusValuesArrayCache<KEYS_COUNT> cache;
};

/// Cache that tracks values of primary skill values in bonus system
class PrimarySkillsCache
{
	const IBonusBearer * target;
	mutable std::atomic<int32_t> version = 0;
	mutable std::array<std::atomic<int32_t>, 4> skills;

	void update() const;
public:
	PrimarySkillsCache(const IBonusBearer * target);

	const std::array<std::atomic<int32_t>, 4> & getSkills() const;
	const std::atomic<int32_t> & getSkill(PrimarySkill id) const
	{
		return getSkills()[id.getNum()];
	}
};

/// Cache that tracks values of spell school mastery in bonus system
class MagicSchoolMasteryCache
{
	const IBonusBearer * target;
	mutable std::atomic<int32_t> version = 0;
	mutable std::vector<std::atomic<int32_t>> schools;

	void update() const;
public:
	MagicSchoolMasteryCache(const IBonusBearer * target);

	int32_t getMastery(const SpellSchool & school) const;
};

/// Cache that tracks values for different values of bonus duration
class BonusCachePerTurn : public BonusCacheBase
{
	static constexpr int cachedTurns = 8;

	const CSelector selector;
	mutable TConstBonusListPtr bonusList;
	mutable std::mutex bonusListMutex;
	mutable std::atomic<int32_t> bonusListVersion = 0;
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

VCMI_LIB_NAMESPACE_END
