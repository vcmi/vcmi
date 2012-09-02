#pragma once

typedef si32 TResource;
typedef si64 TResourceCap; //to avoid overflow when adding integers. Signed values are easier to control.

class JsonNode;

namespace Res
{
	class ResourceSet;
	bool canAfford(const ResourceSet &res, const ResourceSet &price); //can a be used to pay price b

	enum ERes 
	{
		WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL
	};

	//class to be representing a vector of resource
	class ResourceSet : public std::vector<int>
	{
	public:
		DLL_LINKAGE ResourceSet();
		// read resources set from json. Format example: { "gold": 500, "wood":5 }
		DLL_LINKAGE ResourceSet(const JsonNode & node);


#define scalarOperator(OPSIGN)									\
		DLL_LINKAGE ResourceSet operator OPSIGN(const TResource &rhs) const	\
		{														\
			ResourceSet ret = *this;							\
			for(int i = 0; i < size(); i++)						\
				ret[i] = at(i) OPSIGN rhs;						\
																\
			return ret;											\
		}



#define vectorOperator(OPSIGN)										\
		DLL_LINKAGE ResourceSet operator OPSIGN(const ResourceSet &rhs) const	\
		{															\
			ResourceSet ret = *this;								\
			for(int i = 0; i < size(); i++)							\
				ret[i] = at(i) OPSIGN rhs[i];						\
																	\
			return ret;												\
		}


#define opEqOperator(OPSIGN, RHS_TYPE)							\
		DLL_LINKAGE ResourceSet& operator OPSIGN ## =(const RHS_TYPE &rhs)	\
		{														\
			return *this = *this OPSIGN rhs;					\
		}

		scalarOperator(+)
		scalarOperator(-)
		scalarOperator(*)
		scalarOperator(/)
		opEqOperator(+, TResource)
		opEqOperator(-, TResource)
		opEqOperator(*, TResource)
		vectorOperator(+)
		vectorOperator(-)
		opEqOperator(+, ResourceSet)
		opEqOperator(-, ResourceSet)

#undef scalarOperator
#undef vectorOperator
#undef opEqOperator

		//to be used for calculations of type "how many units of sth can I afford?"
		DLL_LINKAGE int operator/(const ResourceSet &rhs)
		{
			int ret = INT_MAX;
			for(int i = 0; i < size(); i++)
				if(rhs[i])
					vstd::amin(ret, at(i) / rhs[i]);

			return ret;
		}

		DLL_LINKAGE ResourceSet & operator=(const TResource &rhs)
		{
			for(int i = 0; i < size(); i++)
				at(i) = rhs;

			return *this;
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
			h & static_cast<std::vector<int>&>(*this);
		}

		DLL_LINKAGE void amax(const TResourceCap &val); //performs vstd::amax on each element
		DLL_LINKAGE bool nonZero() const; //returns true if at least one value is non-zero;
		DLL_LINKAGE bool canAfford(const ResourceSet &price) const;
		DLL_LINKAGE bool canBeAfforded(const ResourceSet &res) const;

		//special iterator of iterating over non-zero resources in set
		class DLL_LINKAGE nziterator
		{
			struct ResEntry
			{
				TResourceCap resType, resVal;
			} cur;
			const ResourceSet &rs;
			void advance();
		
		public:
			nziterator(const ResourceSet &RS);
			bool valid();
			nziterator operator++();
			nziterator operator++(int);
			const ResEntry& operator*() const;
			const ResEntry* operator->() const;
		
		};
	};
}

typedef Res::ResourceSet TResources;

