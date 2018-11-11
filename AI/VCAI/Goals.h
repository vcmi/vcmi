/*
 * Goals.h, part of VCMI engine
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
#include "AIUtility.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
class AbstractGoal;
class VisitTile;

class DLL_EXPORT TSubgoal : public std::shared_ptr<Goals::AbstractGoal>
{
	public:
		bool operator==(const TSubgoal & rhs) const;
		bool operator<(const TSubgoal & rhs) const;
	//TODO: serialize?
};

typedef std::vector<TSubgoal> TGoalVec;

enum EGoals
{
	INVALID = -1,
	WIN, DO_NOT_LOSE, CONQUER, BUILD, //build needs to get a real reasoning
	EXPLORE, GATHER_ARMY,
	BOOST_HERO,
	RECRUIT_HERO,
	BUILD_STRUCTURE, //if hero set, then in visited town
	COLLECT_RES,
	GATHER_TROOPS, // val of creatures with objid

	OBJECT_GOALS_BEGIN,
	VISIT_OBJ, //visit or defeat or collect the object
	FIND_OBJ, //find and visit any obj with objid + resid //TODO: consider universal subid for various types (aid, bid)
	VISIT_HERO, //heroes can move around - set goal abstract and track hero every turn

	GET_ART_TYPE,

	//BUILD_STRUCTURE,
	ISSUE_COMMAND,

	VISIT_TILE, //tile, in conjunction with hero elementar; assumes tile is reachable
	CLEAR_WAY_TO,
	DIG_AT_TILE,//elementar with hero on tile
	BUY_ARMY, //at specific town
	TRADE, //val resID at object objid
	BUILD_BOAT
};

	//method chaining + clone pattern
#define VSETTER(type, field) virtual AbstractGoal & set ## field(const type &rhs) {field = rhs; return *this;};
#define OSETTER(type, field) CGoal<T> & set ## field(const type &rhs) override { field = rhs; return *this; };

#if 0
	#define SETTER
#endif // _DEBUG

enum {LOW_PR = -1};

DLL_EXPORT TSubgoal sptr(const AbstractGoal & tmp);

struct DLL_EXPORT EvaluationContext
{
	uint64_t movementCost;
	int manaCost;
	uint64_t danger;

	EvaluationContext()
		:movementCost(0), danger(0), manaCost(0)
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

	AbstractGoal(EGoals goal = INVALID)
		: goalType (goal), evaluationContext()
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
	virtual ~AbstractGoal(){}
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

	static TSubgoal goVisitOrLookFor(const CGObjectInstance * obj); //if obj is nullptr, then we'll explore
	static TSubgoal lookForArtSmart(int aid); //checks non-standard ways of obtaining art (merchants, quests, etc.)

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

template<typename T> class DLL_EXPORT CGoal : public AbstractGoal
{
public:
	CGoal<T>(EGoals goal = INVALID) : AbstractGoal(goal)
	{
		priority = 0;
		isElementar = false;
		isAbstract = false;
		value = 0;
		aid = -1;
		objid = -1;
		resID = -1;
		tile = int3(-1, -1, -1);
		town = nullptr;
	}

	OSETTER(bool, isElementar)
	OSETTER(bool, isAbstract)
	OSETTER(float, priority)
	OSETTER(int, value)
	OSETTER(int, resID)
	OSETTER(int, objid)
	OSETTER(int, aid)
	OSETTER(int3, tile)
	OSETTER(HeroPtr, hero)
	OSETTER(CGTownInstance *, town)
	OSETTER(int, bid)

	void accept(VCAI * ai) override;
	float accept(FuzzyHelper * f) override;

	CGoal<T> * clone() const override
	{
		return new T(static_cast<T const &>(*this)); //casting enforces template instantiation
	}
	TSubgoal iAmElementar()
	{
		setisElementar(true); //FIXME: it's not const-correct, maybe we shoudl only set returned clone?
		TSubgoal ptr;
		ptr.reset(clone());
		return ptr;
	}
	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<AbstractGoal &>(*this);
		//h & goalType & isElementar & isAbstract & priority;
		//h & value & resID & objid & aid & tile & hero & town & bid;
	}

	virtual bool operator==(const AbstractGoal & g) const override
	{
		if(goalType != g.goalType)
			return false;

		return (*this) == (dynamic_cast<const T &>(g));
	}

	virtual bool operator==(const T & other) const = 0;
};

class DLL_EXPORT Invalid : public CGoal<Invalid>
{
public:
	Invalid()
		: CGoal(Goals::INVALID)
	{
		priority = -1e10;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;

	virtual bool operator==(const Invalid & other) const override
	{
		return true;
	}
};

class DLL_EXPORT Win : public CGoal<Win>
{
public:
	Win()
		: CGoal(Goals::WIN)
	{
		priority = 100;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;

	virtual bool operator==(const Win & other) const override
	{
		return true;
	}
};

class DLL_EXPORT NotLose : public CGoal<NotLose>
{
public:
	NotLose()
		: CGoal(Goals::DO_NOT_LOSE)
	{
		priority = 100;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	//TSubgoal whatToDoToAchieve() override;

	virtual bool operator==(const NotLose & other) const override
	{
		return true;
	}
};

class DLL_EXPORT Conquer : public CGoal<Conquer>
{
public:
	Conquer()
		: CGoal(Goals::CONQUER)
	{
		priority = 10;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	virtual bool operator==(const Conquer & other) const override;
};

class DLL_EXPORT Build : public CGoal<Build>
{
public:
	Build()
		: CGoal(Goals::BUILD)
	{
		priority = 1;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;

	virtual bool operator==(const Build & other) const override
	{
		return true;
	}
};

class DLL_EXPORT BuildBoat : public CGoal<BuildBoat>
{
private:
	const IShipyard * shipyard;

public:
	BuildBoat(const IShipyard * shipyard)
		: CGoal(Goals::BUILD_BOAT), shipyard(shipyard)
	{
		priority = 0;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	void accept(VCAI * ai) override;
	std::string name() const override;
	std::string completeMessage() const override;
	virtual bool operator==(const BuildBoat & other) const override;
};

class DLL_EXPORT Explore : public CGoal<Explore>
{
public:
	Explore()
		: CGoal(Goals::EXPLORE)
	{
		priority = 1;
	}
	Explore(HeroPtr h)
		: CGoal(Goals::EXPLORE)
	{
		hero = h;
		priority = 1;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
	bool fulfillsMe(TSubgoal goal) override;
	virtual bool operator==(const Explore & other) const override;
};

class DLL_EXPORT GatherArmy : public CGoal<GatherArmy>
{
public:
	GatherArmy()
		: CGoal(Goals::GATHER_ARMY)
	{
	}
	GatherArmy(int val)
		: CGoal(Goals::GATHER_ARMY)
	{
		value = val;
		priority = 2.5;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
	virtual bool operator==(const GatherArmy & other) const override;
};

class DLL_EXPORT BuyArmy : public CGoal<BuyArmy>
{
private:
	BuyArmy()
		: CGoal(Goals::BUY_ARMY)
	{}
public:
	BuyArmy(const CGTownInstance * Town, int val)
		: CGoal(Goals::BUY_ARMY)
	{
		town = Town; //where to buy this army
		value = val; //expressed in AI unit strength
		priority = 3;//TODO: evaluate?
	}
	bool fulfillsMe(TSubgoal goal) override;

	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
	virtual bool operator==(const BuyArmy & other) const override;
};

class DLL_EXPORT BoostHero : public CGoal<BoostHero>
{
public:
	BoostHero()
		: CGoal(Goals::INVALID)
	{
		priority = -1e10; //TODO
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	virtual bool operator==(const BoostHero & other) const override;
	//TSubgoal whatToDoToAchieve() override {return sptr(Invalid());};
};

class DLL_EXPORT RecruitHero : public CGoal<RecruitHero>
{
public:
	RecruitHero()
		: CGoal(Goals::RECRUIT_HERO)
	{
		priority = 1;
	}

	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}

	TSubgoal whatToDoToAchieve() override;

	virtual bool operator==(const RecruitHero & other) const override
	{
		return true;
	}
};

class DLL_EXPORT BuildThis : public CGoal<BuildThis>
{
public:
	BuildThis() //should be private, but unit test uses it
		: CGoal(Goals::BUILD_STRUCTURE)
	{}
	BuildThis(BuildingID Bid, const CGTownInstance * tid)
		: CGoal(Goals::BUILD_STRUCTURE)
	{
		bid = Bid;
		town = tid;
		priority = 1;
	}
	BuildThis(BuildingID Bid)
		: CGoal(Goals::BUILD_STRUCTURE)
	{
		bid = Bid;
		priority = 1;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	//bool fulfillsMe(TSubgoal goal) override;
	virtual bool operator==(const BuildThis & other) const override;
};

class DLL_EXPORT CollectRes : public CGoal<CollectRes>
{
public:
	CollectRes()
		: CGoal(Goals::COLLECT_RES)
	{
	}
	CollectRes(int rid, int val)
		: CGoal(Goals::COLLECT_RES)
	{
		resID = rid;
		value = val;
		priority = 2;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	TSubgoal whatToDoToTrade();
	bool fulfillsMe(TSubgoal goal) override; //TODO: Trade
	virtual bool operator==(const CollectRes & other) const override;
};

class DLL_EXPORT Trade : public CGoal<Trade>
{
public:
	Trade()
		: CGoal(Goals::TRADE)
	{
	}
	Trade(int rid, int val, int Objid)
		: CGoal(Goals::TRADE)
	{
		resID = rid;
		value = val;
		objid = Objid;
		priority = 3; //trading is instant, but picking resources is free
	}
	TSubgoal whatToDoToAchieve() override;
	virtual bool operator==(const Trade & other) const override;
};

class DLL_EXPORT GatherTroops : public CGoal<GatherTroops>
{
public:
	GatherTroops()
		: CGoal(Goals::GATHER_TROOPS)
	{
		priority = 2;
	}
	GatherTroops(int type, int val)
		: CGoal(Goals::GATHER_TROOPS)
	{
		objid = type;
		value = val;
		priority = 2;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;
	virtual bool operator==(const GatherTroops & other) const override;

private:
	int getCreaturesCount(const CArmedInstance * army);
};

class DLL_EXPORT VisitObj : public CGoal<VisitObj> //this goal was previously known as GetObj
{
public:
	VisitObj() = delete; // empty constructor not allowed
	VisitObj(int Objid);

	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;
	std::string completeMessage() const override;
	virtual bool operator==(const VisitObj & other) const override;
};

class DLL_EXPORT FindObj : public CGoal<FindObj>
{
public:
	FindObj() {} // empty constructor not allowed

	FindObj(int ID)
		: CGoal(Goals::FIND_OBJ)
	{
		objid = ID;
		resID = -1; //subid unspecified
		priority = 1;
	}
	FindObj(int ID, int subID)
		: CGoal(Goals::FIND_OBJ)
	{
		objid = ID;
		resID = subID;
		priority = 1;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;
	virtual bool operator==(const FindObj & other) const override;
};

class DLL_EXPORT VisitHero : public CGoal<VisitHero>
{
public:
	VisitHero()
		: CGoal(Goals::VISIT_HERO)
	{
	}
	VisitHero(int hid)
		: CGoal(Goals::VISIT_HERO)
	{
		objid = hid;
		priority = 4;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;
	std::string completeMessage() const override;
	virtual bool operator==(const VisitHero & other) const override;
};

class DLL_EXPORT GetArtOfType : public CGoal<GetArtOfType>
{
public:
	GetArtOfType()
		: CGoal(Goals::GET_ART_TYPE)
	{
	}
	GetArtOfType(int type)
		: CGoal(Goals::GET_ART_TYPE)
	{
		aid = type;
		priority = 2;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	virtual bool operator==(const GetArtOfType & other) const override;
};

class DLL_EXPORT VisitTile : public CGoal<VisitTile>
	//tile, in conjunction with hero elementar; assumes tile is reachable
{
public:
	VisitTile() {} // empty constructor not allowed

	VisitTile(int3 Tile)
		: CGoal(Goals::VISIT_TILE)
	{
		tile = Tile;
		priority = 5;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
	virtual bool operator==(const VisitTile & other) const override;
};

class DLL_EXPORT ClearWayTo : public CGoal<ClearWayTo>
{
public:
	ClearWayTo()
		: CGoal(Goals::CLEAR_WAY_TO)
	{
	}
	ClearWayTo(int3 Tile)
		: CGoal(Goals::CLEAR_WAY_TO)
	{
		tile = Tile;
		priority = 5;
	}
	ClearWayTo(int3 Tile, HeroPtr h)
		: CGoal(Goals::CLEAR_WAY_TO)
	{
		tile = Tile;
		hero = h;
		priority = 5;
	}
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool fulfillsMe(TSubgoal goal) override;
	virtual bool operator==(const ClearWayTo & other) const override;
};

class DLL_EXPORT DigAtTile : public CGoal<DigAtTile>
	//elementar with hero on tile
{
public:
	DigAtTile()
		: CGoal(Goals::DIG_AT_TILE)
	{
	}
	DigAtTile(int3 Tile)
		: CGoal(Goals::DIG_AT_TILE)
	{
		tile = Tile;
		priority = 20;
	}
	TGoalVec getAllPossibleSubgoals() override
	{
		return TGoalVec();
	}
	TSubgoal whatToDoToAchieve() override;
	virtual bool operator==(const DigAtTile & other) const override;
};

}
