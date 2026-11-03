/*
 * ResourceSet.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "GameConstants.h"

using TResource = int32_t;
using TResourceCap = int64_t; //to avoid overflow when adding integers. Signed values are easier to control.

class JsonNode;
class JsonSerializeFormat;
class IIdentifierResolver;

/// Amounts of resources, sorted by resource. Resources that are not stored in the set have amount of 0
class ResourceSet
{
private:
	boost::container::flat_map<GameResID, TResource> container;
public:
	DLL_LINKAGE void resolveFromJson(const JsonNode & node, IIdentifierResolver & resolver);

#define scalarOperator(OPSIGN)									\
	ResourceSet& operator OPSIGN ## =(const TResource &rhs) \
	{														\
		for(auto & entry : container)						\
			entry.second OPSIGN ## = rhs;						\
															\
		return *this;											\
	}

#define vectorOperator(OPSIGN)										\
	ResourceSet& operator OPSIGN ## =(const ResourceSet &rhs)	\
	{															\
		for(const auto & entry : rhs.container)							\
			container[entry.first] OPSIGN ## = entry.second;						\
																\
		return *this;												\
	}

#define twoOperands(OPSIGN, RHS_TYPE) \
	friend ResourceSet operator OPSIGN(ResourceSet lhs, const RHS_TYPE &rhs) \
	{ \
		lhs OPSIGN ## = rhs; \
		return lhs; \
	}

	scalarOperator(*)
	scalarOperator(/)
	vectorOperator(+)
	vectorOperator(-)
	twoOperands(*, TResource)
	twoOperands(/, TResource)
	twoOperands(+, ResourceSet)
	twoOperands(-, ResourceSet)

#undef scalarOperator
#undef vectorOperator
#undef twoOperands

	TResource & operator[](GameResID index)
	{
		return container[index];
	}

	TResource operator[](GameResID index) const
	{
		auto it = container.find(index);
		return it == container.end() ? 0 : it->second;
	}

	/// returns true if amount of every resource is zero
	bool empty() const
	{
		return !nonZero();
	}

	/// iterates over stored resources, which may include resources with amount of 0
	auto begin() const
	{
		return container.cbegin();
	}

	auto end() const
	{
		return container.cend();
	}

	/// iterates over resources with amount other than 0
	auto nonZeroEntries() const &
	{
		return container | std::views::filter([](const auto & entry){ return entry.second != 0; });
	}

	void nonZeroEntries() const && = delete;

	/// returns how many times price can be paid from this set of funds; INT_MAX if price has no positive amounts
	int affordableCount(const ResourceSet & price) const
	{
		int ret = INT_MAX;
		for(const auto & [resource, amount] : price.container)
			if(amount > 0)
				vstd::amin(ret, std::max(0, (*this)[resource] / amount));

		return ret;
	}

	/// returns how many periods of income are needed to accumulate positive amounts of this set; INT_MAX if some resource has no positive income
	int turnsToAccumulate(const ResourceSet & income) const
	{
		int ret = 0;
		for(const auto & [resource, amount] : container)
		{
			if(amount <= 0)
				continue;

			if(income[resource] <= 0)
				return INT_MAX;

			ret = std::max(ret, vstd::divideAndCeil(amount, income[resource]));
		}
		return ret;
	}

	ResourceSet operator-() const
	{
		ResourceSet ret;
		for(const auto & [resource, amount] : container)
			ret[resource] = -amount;
		return ret;
	}

	ResourceSet multipliedBy(double factor) const
	{
		ResourceSet ret;
		for(const auto & [resource, amount] : container)
			ret[resource] = static_cast<int>(std::round(amount * factor));
		return ret;
	}

	bool operator==(const ResourceSet &rhs) const
	{
		for(const auto & [resource, amount] : container)
			if(amount != rhs[resource])
				return false;

		for(const auto & [resource, amount] : rhs.container)
			if(amount != (*this)[resource])
				return false;

		return true;
	}

	template <typename Handler> void serialize(Handler &h)
	{
		if(h.hasFeature(Handler::Version::SPARSE_RESOURCE_SET))
		{
			h & container;
		}
		else
		{
			std::vector<TResource> amounts;
			h & amounts;
			container.clear();
			for(size_t i = 0; i < amounts.size(); ++i)
				if(amounts[i] != 0)
					container[GameResID(i)] = amounts[i];
		}
	}

	DLL_LINKAGE void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);

	DLL_LINKAGE void amin(const TResourceCap &val); //performs vstd::amin on each element, val must not be negative
	DLL_LINKAGE void removeNegative(); //values below 0 are set to 0 - upgrade cost can't be negative, for example
	DLL_LINKAGE void applyHandicap(int percentage);
	DLL_LINKAGE bool nonZero() const; //returns true if at least one value is non-zero;
	DLL_LINKAGE bool canAfford(const ResourceSet &price) const;
	DLL_LINKAGE TResourceCap marketValue() const;

	DLL_LINKAGE std::string toString() const;
};

using TResources = ResourceSet;
