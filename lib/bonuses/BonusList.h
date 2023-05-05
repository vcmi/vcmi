/*
 * BonusList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"
#include "BonusSelector.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE BonusList
{
public:
	using TInternalContainer = std::vector<std::shared_ptr<Bonus>>;

private:
	TInternalContainer bonuses;
	bool belongsToTree;
	void changed() const;

public:
	using const_reference = TInternalContainer::const_reference;
	using value_type = TInternalContainer::value_type;

	using const_iterator = TInternalContainer::const_iterator;
	using iterator = TInternalContainer::iterator;

	BonusList(bool BelongsToTree = false);
	BonusList(const BonusList &bonusList);
	BonusList(BonusList && other) noexcept;
	BonusList& operator=(const BonusList &bonusList);

	// wrapper functions of the STL vector container
	TInternalContainer::size_type size() const { return bonuses.size(); }
	void push_back(const std::shared_ptr<Bonus> & x);
	TInternalContainer::iterator erase (const int position);
	void clear();
	bool empty() const { return bonuses.empty(); }
	void resize(TInternalContainer::size_type sz, const std::shared_ptr<Bonus> & c = nullptr);
	void reserve(TInternalContainer::size_type sz);
	TInternalContainer::size_type capacity() const { return bonuses.capacity(); }
	STRONG_INLINE std::shared_ptr<Bonus> &operator[] (TInternalContainer::size_type n) { return bonuses[n]; }
	STRONG_INLINE const std::shared_ptr<Bonus> &operator[] (TInternalContainer::size_type n) const { return bonuses[n]; }
	std::shared_ptr<Bonus> &back() { return bonuses.back(); }
	std::shared_ptr<Bonus> &front() { return bonuses.front(); }
	const std::shared_ptr<Bonus> &back() const { return bonuses.back(); }
	const std::shared_ptr<Bonus> &front() const { return bonuses.front(); }

	// There should be no non-const access to provide solid,robust bonus caching
	TInternalContainer::const_iterator begin() const { return bonuses.begin(); }
	TInternalContainer::const_iterator end() const { return bonuses.end(); }
	TInternalContainer::size_type operator-=(const std::shared_ptr<Bonus> & i);

	// BonusList functions
	void stackBonuses();
	int totalValue() const;
	void getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit = nullptr) const;
	void getAllBonuses(BonusList &out) const;

	//special find functions
	std::shared_ptr<Bonus> getFirst(const CSelector &select);
	std::shared_ptr<const Bonus> getFirst(const CSelector &select) const;
	int valOfBonuses(const CSelector &select) const;

	// conversion / output
	JsonNode toJsonNode() const;

	// remove_if implementation for STL vector types
	template <class Predicate>
	void remove_if(Predicate pred)
	{
		BonusList newList;
		for(const auto & b : bonuses)
		{
			if (!pred(b.get()))
				newList.push_back(b);
		}
		bonuses.clear();
		bonuses.resize(newList.size());
		std::copy(newList.begin(), newList.end(), bonuses.begin());
	}

	template <class InputIterator>
	void insert(const int position, InputIterator first, InputIterator last);
	void insert(TInternalContainer::iterator position, TInternalContainer::size_type n, const std::shared_ptr<Bonus> & x);

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & static_cast<TInternalContainer&>(bonuses);
	}

	// C++ for range support
	auto begin () -> decltype (bonuses.begin())
	{
		return bonuses.begin();
	}

	auto end () -> decltype (bonuses.end())
	{
		return bonuses.end();
	}
};

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList);


VCMI_LIB_NAMESPACE_END