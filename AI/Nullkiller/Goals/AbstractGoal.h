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

#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/CBuildingHandler.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CTownHandler.h"
#include "../AIUtility.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

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
		COMPOSITION
	};

	class DLL_EXPORT TSubgoal : public std::shared_ptr<AbstractGoal>
	{
	public:
		bool operator==(const TSubgoal & rhs) const;
		bool operator<(const TSubgoal & rhs) const;
		//TODO: serialize?
	};

	typedef std::shared_ptr<ITask> TTask;
	typedef std::vector<TTask> TTaskVec;
	typedef std::vector<TSubgoal> TGoalVec;

	//method chaining + clone pattern
#define VSETTER(type, field) virtual AbstractGoal & set ## field(const type &rhs) {field = rhs; return *this;};
#define OSETTER(type, field) CGoal<T> & set ## field(const type &rhs) override { field = rhs; return *this; };

#if 0
#define SETTER
#endif // _DEBUG

	enum { LOW_PR = -1 };

	DLL_EXPORT TSubgoal sptr(const AbstractGoal & tmp);
	DLL_EXPORT TTask taskptr(const AbstractGoal & tmp);
	
	class DLL_EXPORT AbstractGoal
	{
	public:
		bool isAbstract; VSETTER(bool, isAbstract)
		int value; VSETTER(int, value)
		float strategicalValue; VSETTER(float, strategicalValue)
		ui64 goldCost; VSETTER(ui64, goldCost)
		int resID; VSETTER(int, resID)
		int objid; VSETTER(int, objid)
		int aid; VSETTER(int, aid)
		int3 tile; VSETTER(int3, tile)
		HeroPtr hero; VSETTER(HeroPtr, hero)
		const CGTownInstance *town; VSETTER(CGTownInstance *, town)
		int bid; VSETTER(int, bid)
		TSubgoal parent; VSETTER(TSubgoal, parent)
		//EvaluationContext evaluationContext; VSETTER(EvaluationContext, evaluationContext)

		AbstractGoal(EGoals goal = EGoals::INVALID)
			: goalType(goal), hero()
		{
			isAbstract = false;
			value = 0;
			aid = -1;
			resID = -1;
			objid = -1;
			tile = int3(-1, -1, -1);
			town = nullptr;
			bid = -1;
			strategicalValue = 0;
			goldCost = 0;
		}
		virtual ~AbstractGoal() {}
		//FIXME: abstract goal should be abstract, but serializer fails to instantiate subgoals in such case
		virtual AbstractGoal * clone() const
		{
			return const_cast<AbstractGoal *>(this);
		}

		virtual TGoalVec decompose() const
		{
			return TGoalVec();
		}

		EGoals goalType;

		virtual std::string toString() const;

		bool invalid() const;
		
		virtual bool operator==(const AbstractGoal & g) const;

		virtual bool isElementar() const { return false; }
		
		bool operator!=(const AbstractGoal & g) const
		{
			return !(*this == g);
		}

		template<typename Handler> void serialize(Handler & h, const int version)
		{
			float priority;
			bool isElementar;

			h & goalType;
			h & isElementar;
			h & isAbstract;
			h & priority;
			h & value;
			h & resID;
			h & objid;
			h & aid;
			h & tile;
			h & hero;
			h & town;
			h & bid;
		}
	};

	class DLL_EXPORT ITask
	{
	public:
		float priority;

		ITask() : priority(0) {}

		///Visitor pattern
		//TODO: make accept work for std::shared_ptr... somehow
		virtual void accept(VCAI * ai) = 0; //unhandled goal will report standard error
		virtual std::string toString() const = 0;
		virtual ~ITask() {}
	};
}
