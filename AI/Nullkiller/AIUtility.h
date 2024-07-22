/*
 * AIUtility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

 // Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

/*********************** TBB.h ********************/

#include "tbb/blocked_range.h"
#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/concurrent_unordered_set.h"
#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_invoke.h"

/*********************** TBB.h ********************/

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../CCallback.h"

#include <chrono>


using dwellingContent = std::pair<ui32, std::vector<CreatureID>>;

namespace NKAI
{
struct creInfo;
class AIGateway;
class Nullkiller;

const int GOLD_MINE_PRODUCTION = 1000;
const int WOOD_ORE_MINE_PRODUCTION = 2;
const int RESOURCE_MINE_PRODUCTION = 1;
const int ACTUAL_RESOURCE_COUNT = 7;
const int ALLOWED_ROAMING_HEROES = 8;

//implementation-dependent
extern const float SAFE_ATTACK_CONSTANT;
extern const int GOLD_RESERVE;

extern thread_local CCallback * cb;

enum HeroRole
{
	SCOUT = 0,

	MAIN = 1
};

//provisional class for AI to store a reference to an owned hero object
//checks if it's valid on access, should be used in place of const CGHeroInstance*

struct DLL_EXPORT HeroPtr
{
	const CGHeroInstance * h;
	ObjectInstanceID hid;

public:
	std::string name() const;

	HeroPtr();
	HeroPtr(const CGHeroInstance * H);
	~HeroPtr();

	operator bool() const
	{
		return validAndSet();
	}

	bool operator<(const HeroPtr & rhs) const;
	const CGHeroInstance * operator->() const;
	const CGHeroInstance * operator*() const; //not that consistent with -> but all interfaces use CGHeroInstance*, so it's convenient
	bool operator==(const HeroPtr & rhs) const;
	bool operator!=(const HeroPtr & rhs) const
	{
		return !(*this == rhs);
	}

	const CGHeroInstance * get(bool doWeExpectNull = false) const;
	const CGHeroInstance * get(const CPlayerSpecificInfoCallback * cb, bool doWeExpectNull = false) const;
	bool validAndSet() const;
};

enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};

// AI lives in a dangerous world. CGObjectInstances under pointer may got deleted/hidden.
// This class stores object id, so we can detect when we lose access to the underlying object.
struct ObjectIdRef
{
	ObjectInstanceID id;

	const CGObjectInstance * operator->() const;
	operator const CGObjectInstance *() const;
	operator bool() const;

	ObjectIdRef(ObjectInstanceID _id);
	ObjectIdRef(const CGObjectInstance * obj);

	bool operator<(const ObjectIdRef & rhs) const;
};

template<Obj::Type id>
bool objWithID(const CGObjectInstance * obj)
{
	return obj->ID == id;
}

struct creInfo
{
	int count;
	CreatureID creID;
	int level;
};
creInfo infoFromDC(const dwellingContent & dc);

template<class Func>
void foreach_tile_pos(const Func & foo)
{
	// some micro-optimizations since this function gets called a LOT
	// callback pointer is thread-specific and slow to retrieve -> read map size only once
	int3 mapSize = cb->getMapSize();
	for(int z = 0; z < mapSize.z; z++)
	{
		for(int x = 0; x < mapSize.x; x++)
		{
			for(int y = 0; y < mapSize.y; y++)
			{
				foo(int3(x, y, z));
			}
		}
	}
}

template<class Func, class TCallback>
void foreach_tile_pos(TCallback * cbp, const Func & foo) // avoid costly retrieval of thread-specific pointer
{
	int3 mapSize = cbp->getMapSize();
	for(int z = 0; z < mapSize.z; z++)
	{
		for(int x = 0; x < mapSize.x; x++)
		{
			for(int y = 0; y < mapSize.y; y++)
			{
				foo(cbp, int3(x, y, z));
			}
		}
	}
}

template<class Func>
void foreach_neighbour(const int3 & pos, const Func & foo)
{
	CCallback * cbp = cb; // avoid costly retrieval of thread-specific pointer
	for(const int3 & dir : int3::getDirs())
	{
		const int3 n = pos + dir;
		if(cbp->isInTheMap(n))
			foo(pos + dir);
	}
}

template<class Func>
void foreach_neighbour(CCallback * cbp, const int3 & pos, const Func & foo) // avoid costly retrieval of thread-specific pointer
{
	for(const int3 & dir : int3::getDirs())
	{
		const int3 n = pos + dir;
		if(cbp->isInTheMap(n))
			foo(cbp, pos + dir);
	}
}

bool canBeEmbarkmentPoint(const TerrainTile * t, bool fromWater);
bool isObjectPassable(const Nullkiller * ai, const CGObjectInstance * obj);
bool isObjectPassable(const CGObjectInstance * obj, PlayerColor playerColor, PlayerRelations objectRelations);
bool isBlockVisitObj(const int3 & pos);

bool isWeeklyRevisitable(const Nullkiller * ai, const CGObjectInstance * obj);

bool isObjectRemovable(const CGObjectInstance * obj); //FIXME FIXME: move logic to object property!
bool isSafeToVisit(const CGHeroInstance * h, uint64_t dangerStrength);
bool isSafeToVisit(const CGHeroInstance * h, const CCreatureSet *, uint64_t dangerStrength);

bool compareHeroStrength(const CGHeroInstance * h1, const CGHeroInstance * h2);
bool compareArmyStrength(const CArmedInstance * a1, const CArmedInstance * a2);
bool compareArtifacts(const CArtifactInstance * a1, const CArtifactInstance * a2);
bool townHasFreeTavern(const CGTownInstance * town);

uint64_t getHeroArmyStrengthWithCommander(const CGHeroInstance * hero, const CCreatureSet * heroArmy);

uint64_t timeElapsed(std::chrono::time_point<std::chrono::high_resolution_clock> start);

// todo: move to obj manager
bool shouldVisit(const Nullkiller * ai, const CGHeroInstance * h, const CGObjectInstance * obj);
int getDuplicatingSlots(const CArmedInstance * army);

template <class T>
class SharedPool
{
public:
	struct External_Deleter
	{
		explicit External_Deleter(std::weak_ptr<SharedPool<T>* > pool)
			: pool(pool)
		{
		}

		void operator()(T * ptr)
		{
			std::unique_ptr<T> uptr(ptr);

			if(auto pool_ptr = pool.lock())
			{
				(*pool_ptr.get())->add(std::move(uptr));
			}
		}

	private:
		std::weak_ptr<SharedPool<T>* > pool;
	};

public:
	using ptr_type = std::unique_ptr<T, External_Deleter>;

	SharedPool(std::function<std::unique_ptr<T>()> elementFactory):
		elementFactory(elementFactory), pool(), instance_tracker(new SharedPool<T> *(this))
	{}

	void add(std::unique_ptr<T> t)
	{
		boost::lock_guard<boost::mutex> lock(sync);
		pool.push_back(std::move(t));
	}

	ptr_type acquire()
	{
		boost::lock_guard<boost::mutex> lock(sync);
		bool poolIsEmpty = pool.empty();
		T * element = poolIsEmpty
			? elementFactory().release()
			: pool.back().release();

		ptr_type tmp(
			element,
			External_Deleter(std::weak_ptr<SharedPool<T>*>(instance_tracker)));

		if(!poolIsEmpty) pool.pop_back();

		return tmp;
	}

	bool empty() const
	{
		return pool.empty();
	}

	size_t size() const
	{
		return pool.size();
	}

private:
	std::vector<std::unique_ptr<T>> pool;
	std::function<std::unique_ptr<T>()> elementFactory;
	std::shared_ptr<SharedPool<T> *> instance_tracker;
	boost::mutex sync;
};

}
