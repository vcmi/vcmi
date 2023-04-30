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
class CCallback;

namespace Goals
{
	class AbstractGoal;
	class Explore;
	class RecruitHero;
	class VisitTile;
	class VisitObj;
	class VisitHero;
	class BuildThis;
	class DigAtTile;
	class CollectRes;
	class Build;
	class BuyArmy;
	class BuildBoat;
	class GatherArmy;
	class ClearWayTo;
	class Invalid;
	class Trade;
	class CompleteQuest;
	class AdventureSpellCast;

	enum EGoals
	{
		INVALID = -1,
		WIN, CONQUER, BUILD, //build needs to get a real reasoning
		EXPLORE, GATHER_ARMY,
		BOOST_HERO,
		RECRUIT_HERO,
		BUILD_STRUCTURE, //if hero set, then in visited town
		COLLECT_RES,
		GATHER_TROOPS, // val of creatures with objid

		VISIT_OBJ, //visit or defeat or collect the object
		FIND_OBJ, //find and visit any obj with objid + resid //TODO: consider universal subid for various types (aid, bid)
		VISIT_HERO, //heroes can move around - set goal abstract and track hero every turn

		GET_ART_TYPE,

		VISIT_TILE, //tile, in conjunction with hero elementar; assumes tile is reachable
		CLEAR_WAY_TO,
		DIG_AT_TILE,//elementar with hero on tile
		BUY_ARMY, //at specific town
		TRADE, //val resID at object objid
		BUILD_BOAT,
		COMPLETE_QUEST,
		ADVENTURE_SPELL_CAST
	};

	class DLL_EXPORT TSubgoal : public std::shared_ptr<AbstractGoal>
	{
	public:
		bool operator==(const TSubgoal & rhs) const;
		bool operator<(const TSubgoal & rhs) const;
		//TODO: serialize?
	};

	using TGoalVec = std::vector<TSubgoal>;

	//method chaining + clone pattern
#define VSETTER(type, field) virtual AbstractGoal & set ## field(const type &rhs) {field = rhs; return *this;};
#define OSETTER(type, field) CGoal<T> & set ## field(const type &rhs) override { field = rhs; return *this; };

#if 0
#define SETTER
#endif // _DEBUG

	enum { LOW_PR = -1 };

	DLL_EXPORT TSubgoal sptr(const AbstractGoal & tmp);

	struct DLL_EXPORT EvaluationContext
	{
		float movementCost;
		int manaCost;
		uint64_t danger;

		EvaluationContext()
			: movementCost(0.0),
			manaCost(0),
			danger(0)
		{
		}
	};

	class DLL_EXPORT AbstractGoal
	{
	public:
		bool isElementar; VSETTER(bool, isElementar)
			bool isAbstract; VSETTER(bool, isAbstract)
			float priority; VSETTER(float, priority)
			int value; VSETTER(int, value)
			int resID; VSETTER(int, resID)
			int objid; VSETTER(int, objid)
			int aid; VSETTER(int, aid)
			int3 tile; VSETTER(int3, tile)
			HeroPtr hero; VSETTER(HeroPtr, hero)
			const CGTownInstance *town; VSETTER(CGTownInstance *, town)
			int bid; VSETTER(int, bid)
			TSubgoal parent; VSETTER(TSubgoal, parent)
			EvaluationContext evaluationContext; VSETTER(EvaluationContext, evaluationContext)

		AbstractGoal(EGoals goal = EGoals::INVALID): goalType(goal)
		{
			priority = 0;
			isElementar = false;
			isAbstract = false;
			value = 0;
			aid = -1;
			resID = -1;
			objid = -1;
			tile = int3(-1, -1, -1);
			town = nullptr;
			bid = -1;
		}
		virtual ~AbstractGoal() {}
		//FIXME: abstract goal should be abstract, but serializer fails to instantiate subgoals in such case
		virtual AbstractGoal * clone() const
		{
			return const_cast<AbstractGoal *>(this);
		}
		virtual TGoalVec getAllPossibleSubgoals()
		{
			return TGoalVec();
		}
		virtual TSubgoal whatToDoToAchieve()
		{
			return sptr(AbstractGoal());
		}

		EGoals goalType;

		virtual std::string name() const;
		virtual std::string completeMessage() const
		{
			return "This goal is unspecified!";
		}

		bool invalid() const;

		///Visitor pattern
		//TODO: make accept work for std::shared_ptr... somehow
		virtual void accept(VCAI * ai); //unhandled goal will report standard error
		virtual float accept(FuzzyHelper * f);

		virtual bool operator==(const AbstractGoal & g) const;
		bool operator<(AbstractGoal & g); //final
		virtual bool fulfillsMe(Goals::TSubgoal goal) //TODO: multimethod instead of type check
		{
			return false; //use this method to check if goal is fulfilled by another (not equal) goal, operator == is handled spearately
		}

		bool operator!=(const AbstractGoal & g) const
		{
			return !(*this == g);
		}

		template<typename Handler> void serialize(Handler & h, const int version)
		{
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
}
