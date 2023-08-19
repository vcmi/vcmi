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

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../CCallback.h"

class VCAI;
class CCallback;
struct creInfo;

using crint3 = const int3 &;
using crstring = const std::string &;
using dwellingContent = std::pair<ui32, std::vector<CreatureID>>;

const int GOLD_MINE_PRODUCTION = 1000, WOOD_ORE_MINE_PRODUCTION = 2, RESOURCE_MINE_PRODUCTION = 1;
const int ACTUAL_RESOURCE_COUNT = 7;
const int ALLOWED_ROAMING_HEROES = 8;

//implementation-dependent
extern const double SAFE_ATTACK_CONSTANT;
extern const int GOLD_RESERVE;

extern thread_local CCallback * cb;
extern thread_local VCAI * ai;

//provisional class for AI to store a reference to an owned hero object
//checks if it's valid on access, should be used in place of const CGHeroInstance*

struct DLL_EXPORT HeroPtr
{
	const CGHeroInstance * h;
	ObjectInstanceID hid;

public:
	std::string name;


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
	bool validAndSet() const;


	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & this->h;
		h & hid;
		h & name;
	}
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


	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
	}
};

struct TimeCheck
{
	CStopWatch time;
	std::string txt;
	TimeCheck(crstring TXT)
		: txt(TXT)
	{
	}

	~TimeCheck()
	{
		logAi->trace("Time of %s was %d ms.", txt, time.getDiff());
	}
};

//TODO: replace with vstd::
struct AtScopeExit
{
	std::function<void()> foo;
	AtScopeExit(const std::function<void()> & FOO)
		: foo(FOO)
	{}
	~AtScopeExit()
	{
		foo();
	}
};


class ObjsVector : public std::vector<ObjectIdRef>
{
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
	const Creature * cre;
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

template<class Func>
void foreach_tile_pos(CCallback * cbp, const Func & foo) // avoid costly retrieval of thread-specific pointer
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
bool isBlockedBorderGate(int3 tileToHit);
bool isBlockVisitObj(const int3 & pos);

bool isWeeklyRevisitable(const CGObjectInstance * obj);
bool shouldVisit(HeroPtr h, const CGObjectInstance * obj);

bool isObjectRemovable(const CGObjectInstance * obj); //FIXME FIXME: move logic to object property!
bool isSafeToVisit(HeroPtr h, uint64_t dangerStrength);
bool isSafeToVisit(HeroPtr h, crint3 tile);

bool compareHeroStrength(HeroPtr h1, HeroPtr h2);
bool compareArmyStrength(const CArmedInstance * a1, const CArmedInstance * a2);
bool compareArtifacts(const CArtifactInstance * a1, const CArtifactInstance * a2);

class CDistanceSorter
{
	const CGHeroInstance * hero;

public:
	CDistanceSorter(const CGHeroInstance * hero)
		: hero(hero)
	{
	}
	bool operator()(const CGObjectInstance * lhs, const CGObjectInstance * rhs) const;
};
