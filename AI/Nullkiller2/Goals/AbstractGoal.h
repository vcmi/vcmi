/*
* AbstractGoal.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../../../lib/GameLibrary.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"

namespace NK2AI
{

struct HeroPtr;
class AIGateway;
class FuzzyHelper;
class Nullkiller;

namespace Goals
{
	class AbstractGoal;
	class ITask;
	class RecruitHero;
	class BuildThis;
	class DigAtTile;
	class CollectRes;
	class BuyArmy;
	class BuildBoat;
	class Invalid;
	class Trade;
	class AdventureSpellCast;

	enum EGoals
	{
		INVALID = -1,
		WIN, CONQUER,
		BUILD,
		EXPLORE, GATHER_ARMY,
		BOOST_HERO,
		RECRUIT_HERO,
		RECRUIT_HERO_BEHAVIOR,
		BUILD_STRUCTURE, //if hero set, then in visited town
		COLLECT_RES,
		GATHER_TROOPS, // val of creatures with objid

		CAPTURE_OBJECTS,

		GET_ART_TYPE,

		DEFENCE,
		STARTUP,
		DIG_AT_TILE,//elementar with hero on tile
		BUY_ARMY, //at specific town
		TRADE, //val resID at object objid
		BUILD_BOAT,
		COMPLETE_QUEST,
		ADVENTURE_SPELL_CAST,
		EXECUTE_HERO_CHAIN,
		EXCHANGE_SWAP_TOWN_HEROES,
		DISMISS_HERO,
		COMPOSITION,
		CLUSTER_BEHAVIOR,
		UNLOCK_CLUSTER,
		HERO_EXCHANGE,
		ARMY_UPGRADE,
		DEFEND_TOWN,
		CAPTURE_OBJECT,
		SAVE_RESOURCES,
		STAY_AT_TOWN_BEHAVIOR,
		STAY_AT_TOWN,
		EXPLORATION_BEHAVIOR,
		EXPLORATION_POINT,
		EXPLORE_NEIGHBOUR_TILE
	};

	class DLL_EXPORT TSubgoal : public std::shared_ptr<AbstractGoal>
	{
	public:
		bool operator==(const TSubgoal & rhs) const;
		bool operator<(const TSubgoal & rhs) const;
	};

	using TTask = std::shared_ptr<ITask>;
	using TTaskVec = std::vector<TTask>;
	using TGoalVec = std::vector<TSubgoal>;

	//method chaining + clone pattern
#define SETTER(type, field) AbstractGoal & set ## field(const type &rhs) {field = rhs; return *this;};

	enum { LOW_PR = -1 };

	DLL_EXPORT TSubgoal sptr(const AbstractGoal & tmp);
	DLL_EXPORT TTask taskptr(const AbstractGoal & tmp);
	
	class DLL_EXPORT AbstractGoal
	{
	public:
		bool isAbstract; SETTER(bool, isAbstract)
		int value; SETTER(int, value)
		ui64 goldCost; SETTER(ui64, goldCost)
		TResources buildingCost; SETTER(TResources, buildingCost)
		int resID; SETTER(int, resID)
		int objid; SETTER(int, objid)
		int aid; SETTER(int, aid)
		int3 tile; SETTER(int3, tile)
		const CGHeroInstance * hero; SETTER(CGHeroInstance *, hero)
		const CGTownInstance *town; SETTER(CGTownInstance *, town)
		int bid; SETTER(int, bid)
		EGoals goalType;

		explicit AbstractGoal(EGoals goal = EGoals::INVALID): goalType(goal)
		{
			// isAbstract = false;
			isAbstract = true;
			value = 0;
			aid = -1;
			resID = -1;
			objid = -1;
			tile = int3(-1, -1, -1);
			town = nullptr;
			hero = nullptr;
			bid = -1;
			goldCost = 0;
		}

		virtual ~AbstractGoal() = default;
		//FIXME: abstract goal should be abstract, but serializer fails to instantiate subgoals in such case
		virtual AbstractGoal * clone() const
		{
			return const_cast<AbstractGoal *>(this);
			// auto * copy = new AbstractGoal(*this);
			// return copy;
		}

		virtual TGoalVec decompose(const Nullkiller * aiNk) const
		{
			return TGoalVec();
		}

		virtual std::string toString() const;

		// virtual bool invalid() const;
		virtual bool invalid() const
		{
			return goalType == EGoals::INVALID;
		}
		
		// virtual bool operator==(const AbstractGoal & g) const;
		virtual bool operator==(const AbstractGoal & g) const { return false; }

		virtual bool isElementar() const { return false; }

		virtual bool hasHash() const { return false; }

		virtual uint64_t getHash() const { return 0; }

		virtual ITask * asTask()
		{
			throw std::runtime_error("Abstract goal is not a task");
		}
		
		bool operator!=(const AbstractGoal & g) const
		{
			return !(*this == g);
		}
	};

	class DLL_EXPORT ITask
	{
	public:
		float priority;

		ITask() : priority(0) {}

		///Visitor pattern
		//TODO: make accept work for std::shared_ptr... somehow
		virtual void accept(AIGateway * aiGw) = 0; //unhandled goal will report standard error
		virtual std::string toString() const = 0;
		virtual const CGHeroInstance * getHero() const = 0;
		virtual ~ITask() = default;
		virtual int getHeroExchangeCount() const = 0;
		virtual bool isObjectAffected(ObjectInstanceID h) const = 0;
		virtual std::vector<ObjectInstanceID> getAffectedObjects() const = 0;
	};
}

class cannotFulfillGoalException : public std::exception
{
	std::string msg;

public:
	explicit cannotFulfillGoalException(const std::string  & message)
		: msg(message)
	{
	}

	const char * what() const noexcept override
	{
		return msg.c_str();
	}
};

class goalFulfilledException : public std::exception
{
	std::string msg;

public:
	Goals::TSubgoal goal;

	explicit goalFulfilledException(Goals::TSubgoal Goal)
		: goal(Goal)
	{
		msg = goal->toString();
	}

	const char * what() const noexcept override
	{
		return msg.c_str();
	}
};

}
