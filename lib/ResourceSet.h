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

VCMI_LIB_NAMESPACE_BEGIN

using TResource = int32_t;
using TResourceCap = int64_t; //to avoid overflow when adding integers. Signed values are easier to control.

class JsonNode;
class JsonSerializeFormat;

class ResourceSet;

//class to be representing a vector of resource
class ResourceSet
{
private:
	std::vector<TResource> container = {};
	DLL_LINKAGE void resizeContainer();
public:
	DLL_LINKAGE ResourceSet();
	DLL_LINKAGE ResourceSet(const ResourceSet& rhs);

	DLL_LINKAGE void resolveFromJson(const JsonNode & node);


#define scalarOperator(OPSIGN)									\
	ResourceSet& operator OPSIGN ## =(const TResource &rhs) \
	{														\
		resizeContainer(); \
		for(auto i = 0; i < container.size(); i++)						\
			container.at(i) OPSIGN ## = rhs;						\
															\
		return *this;											\
	}

#define vectorOperator(OPSIGN)										\
	ResourceSet& operator OPSIGN ## =(const ResourceSet &rhs)	\
	{															\
		resizeContainer(); \
		for(auto i = 0; i < container.size(); i++)							\
			container.at(i) OPSIGN ## = rhs[i];						\
																\
		return *this;												\
	}

#define twoOperands(OPSIGN, RHS_TYPE) \
	friend ResourceSet operator OPSIGN(ResourceSet lhs, const RHS_TYPE &rhs) \
	{ \
		lhs OPSIGN ## = rhs; \
		return lhs; \
	}

	scalarOperator(+)
	scalarOperator(-)
	scalarOperator(*)
	scalarOperator(/)
	vectorOperator(+)
	vectorOperator(-)
	twoOperands(+, TResource)
	twoOperands(-, TResource)
	twoOperands(*, TResource)
	twoOperands(/, TResource)
	twoOperands(+, ResourceSet)
	twoOperands(-, ResourceSet)


#undef scalarOperator
#undef vectorOperator
#undef twoOperands

	using const_reference = decltype(container)::const_reference;
	using value_type = decltype(container)::value_type;
	using const_iterator = decltype(container)::const_iterator;
	using iterator = decltype(container)::iterator;

	// Array-like interface
	TResource & operator[](GameResID index)
	{
		resizeContainer();
		return operator[](index.getNum());
	}

	const TResource & operator[](GameResID index) const 
	{
		if (index.getNum() >= container.size()) {
			static const TResource defaultValue{};
			return defaultValue;
		}
		return operator[](index.getNum());
	}

	TResource & operator[](size_t index)
	{
		resizeContainer();
		return container.at(index);
	}

	const TResource & operator[](size_t index) const 
	{
		if (index >= container.size()) {
			static const TResource defaultValue{};
			return defaultValue;
		}
		return container.at(index);
	}

	bool empty () const
	{
		for(const auto & res : *this)
			if(res)
				return false;

		return true;
	}

	// C++ range-based for support
	auto begin () -> decltype (container.begin())
	{
		return container.begin();
	}

	auto end () -> decltype (container.end())
	{
		return container.end();
	}

	auto begin () const -> decltype (container.cbegin())
	{
		return container.cbegin();
	}

	auto end () const -> decltype (container.cend())
	{
		return container.cend();
	}

	auto size () const -> decltype (container.size())
	{
		return container.size();
	}

	//to be used for calculations of type "how many units of sth can I afford?"
	int operator/(const ResourceSet &rhs)
	{
		int ret = INT_MAX;
		for(int i = 0; i < container.size(); i++)
			if(rhs[i])
				vstd::amin(ret, container.at(i) / rhs[i]);

		return ret;
	}

	//Returns how many items of "this" we can afford with provided funds
	int maxPurchasableCount(const ResourceSet& availableFunds) {
		int ret = 0; // Initialize to 0 because we want the maximum number of accumulations

		for (size_t i = 0; i < container.size(); ++i) {
			if (container.at(i) > 0) { // We only care about fulfilling positive needs
				if (availableFunds[i] == 0) {
					// If income is 0 and we need a positive amount, it's impossible to fulfill
					return INT_MAX;
				}
				else {
					// Calculate the number of times we need to accumulate income to fulfill the need
					int ceiledResult = vstd::divideAndCeil(container.at(i), availableFunds[i]);
					ret = std::max(ret, ceiledResult);
				}
			}
		}
		return ret;
	}

	ResourceSet & operator=(const TResource &rhs)
	{
		for(int & i : container)
			i = rhs;

		return *this;
	}

	ResourceSet& operator=(const ResourceSet& rhs)
	{
		if (this != &rhs)
		{
			container = rhs.container;
		}
		return *this;
	}

	ResourceSet operator-() const
	{
		ResourceSet ret;
		for(int i = 0; i < container.size(); i++)
			ret[i] = -container.at(i);
		return ret;
	}

	bool operator==(const ResourceSet &rhs) const
	{
		return this->container == rhs.container;
	}

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.version >= Handler::Version::CONFIGURABLE_RESOURCES)
			h & container;
		else
		{
			if (h.saving)
			{
				std::array<TResource, 8> tmp = {};
				for (size_t i = 0; i < 7; ++i)
        			tmp[i] = container[i];
				tmp[7] = TResource{};
				h & tmp;
			}
			else
			{
				std::array<TResource, 8> tmp = {};
				h & tmp;
				container = std::vector<TResource>(tmp.begin(), tmp.begin() + 7);
			}
		}
	}

	DLL_LINKAGE void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);

	DLL_LINKAGE void amax(const TResourceCap &val); //performs vstd::amax on each element
	DLL_LINKAGE void amin(const TResourceCap &val); //performs vstd::amin on each element
	DLL_LINKAGE void positive(); //values below 0 are set to 0 - upgrade cost can't be negative, for example
	DLL_LINKAGE void applyHandicap(int percentage);
	DLL_LINKAGE bool nonZero() const; //returns true if at least one value is non-zero;
	DLL_LINKAGE bool canAfford(const ResourceSet &price) const;
	DLL_LINKAGE bool canBeAfforded(const ResourceSet &res) const;
	DLL_LINKAGE TResourceCap marketValue() const;

	DLL_LINKAGE std::string toString() const;

	//special iterator of iterating over non-zero resources in set
	class DLL_LINKAGE nziterator
	{
		struct ResEntry
		{
			GameResID resType;
			TResourceCap resVal;
		} cur;
		const ResourceSet &rs;
		void advance();

	public:
		nziterator(const ResourceSet &RS);
		bool valid() const;
		nziterator operator++();
		nziterator operator++(int);
		const ResEntry& operator*() const;
		const ResEntry* operator->() const;
	};


};

using TResources = ResourceSet;


VCMI_LIB_NAMESPACE_END
