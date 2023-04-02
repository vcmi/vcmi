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

namespace Res
{
	class ResourceSet;
	bool canAfford(const ResourceSet &res, const ResourceSet &price); //can a be used to pay price b

	enum ERes
	{
		WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL,

		WOOD_AND_ORE = 127,  // special case for town bonus resource
		INVALID = -1
	};

	//class to be representing a vector of resource
	class ResourceSet
	{
	private:
		std::array<int, GameConstants::RESOURCE_QUANTITY> container;
	public:
		// read resources set from json. Format example: { "gold": 500, "wood":5 }
		DLL_LINKAGE ResourceSet(const JsonNode & node);
		DLL_LINKAGE ResourceSet(TResource wood = 0, TResource mercury = 0, TResource ore = 0, TResource sulfur = 0, TResource crystal = 0,
								TResource gems = 0, TResource gold = 0, TResource mithril = 0);


#define scalarOperator(OPSIGN)									\
		ResourceSet& operator OPSIGN ## =(const TResource &rhs) \
		{														\
			for(auto i = 0; i < container.size(); i++)						\
				container.at(i) OPSIGN ## = rhs;						\
																\
			return *this;											\
		}

#define vectorOperator(OPSIGN)										\
		ResourceSet& operator OPSIGN ## =(const ResourceSet &rhs)	\
		{															\
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
		TResource & operator[](Res::ERes index)
		{
			return operator[](static_cast<size_t>(index));
		}

		const TResource & operator[](Res::ERes index) const 
		{
			return operator[](static_cast<size_t>(index));
		}

		TResource & operator[](size_t index)
		{
			return container[index];
		}

		const TResource & operator[](size_t index) const 
		{
			return container[index];
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

		ResourceSet & operator=(const TResource &rhs)
		{
			for(int i = 0; i < container.size(); i++)
				container.at(i) = rhs;

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

	// WARNING: comparison operators are used for "can afford" relation: a <= b means that foreach i a[i] <= b[i]
	// that doesn't work the other way: a > b doesn't mean that a cannot be afforded with b, it's still b can afford a
// 		bool operator<(const ResourceSet &rhs)
// 		{
// 			for(int i = 0; i < size(); i++)
// 				if(at(i) >= rhs[i])
// 					return false;
//
// 			return true;
// 		}

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & container;
		}

		DLL_LINKAGE void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);

		DLL_LINKAGE void amax(const TResourceCap &val); //performs vstd::amax on each element
		DLL_LINKAGE void amin(const TResourceCap &val); //performs vstd::amin on each element
		DLL_LINKAGE void positive(); //values below 0 are set to 0 - upgrade cost can't be negative, for example
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
				Res::ERes resType;
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
}

using TResources = Res::ResourceSet;


VCMI_LIB_NAMESPACE_END
