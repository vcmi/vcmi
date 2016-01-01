#pragma once

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "AIUtility.h"

/*
 * Goals.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	struct AbstractGoal;
	class VisitTile;
	typedef std::shared_ptr<Goals::AbstractGoal> TSubgoal;
	typedef std::vector<TSubgoal> TGoalVec;

	enum EGoals
{
	INVALID = -1,
	WIN, DO_NOT_LOSE, CONQUER, BUILD, //build needs to get a real reasoning
	EXPLORE, GATHER_ARMY, BOOST_HERO,
	RECRUIT_HERO,
	BUILD_STRUCTURE, //if hero set, then in visited town
	COLLECT_RES,
	GATHER_TROOPS, // val of creatures with objid

	OBJECT_GOALS_BEGIN,
	GET_OBJ, //visit or defeat or collect the object
	FIND_OBJ, //find and visit any obj with objid + resid //TODO: consider universal subid for various types (aid, bid)
	VISIT_HERO, //heroes can move around - set goal abstract and track hero every turn

	GET_ART_TYPE,

	//BUILD_STRUCTURE,
	ISSUE_COMMAND,

	VISIT_TILE, //tile, in conjunction with hero elementar; assumes tile is reachable
	CLEAR_WAY_TO,
	DIG_AT_TILE //elementar with hero on tile
};

	//method chaining + clone pattern
#define VSETTER(type, field) virtual AbstractGoal & set ## field(const type &rhs) {field = rhs; return *this;};
#define OSETTER(type, field) CGoal<T> & set ## field(const type &rhs) override { field = rhs; return *this; };

#if 0
	#define SETTER
#endif // _DEBUG

enum {LOW_PR = -1};

TSubgoal sptr(const AbstractGoal & tmp);

class AbstractGoal
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

	AbstractGoal (EGoals goal = INVALID) : goalType (goal)
	{
		priority = 0;
		isElementar = false;
		isAbstract = false;
		value = 0;
		aid = -1;
		resID = -1;
		tile = int3(-1, -1, -1);
		town = nullptr;
	}
	virtual ~AbstractGoal(){};
	//FIXME: abstract goal should be abstract, but serializer fails to instantiate subgoals in such case
	virtual AbstractGoal * clone() const {return const_cast<AbstractGoal*>(this);};
	virtual TGoalVec getAllPossibleSubgoals() {TGoalVec vec; return vec;};
	virtual TSubgoal whatToDoToAchieve() {return sptr(AbstractGoal());};

	EGoals goalType;

	std::string name() const;
	virtual std::string completeMessage() const {return "This goal is unspecified!";};

	bool invalid() const;

	static TSubgoal goVisitOrLookFor(const CGObjectInstance *obj); //if obj is nullptr, then we'll explore
	static TSubgoal lookForArtSmart(int aid); //checks non-standard ways of obtaining art (merchants, quests, etc.)
	static TSubgoal tryRecruitHero();

	///Visitor pattern
	//TODO: make accept work for std::shared_ptr... somehow
	virtual void accept (VCAI * ai); //unhandled goal will report standard error
	virtual float accept (FuzzyHelper * f);

	virtual bool operator== (AbstractGoal &g);
	virtual bool fulfillsMe (Goals::TSubgoal goal) //TODO: multimethod instead of type check
	{
		return false;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & goalType & isElementar & isAbstract & priority;
		h & value & resID & objid & aid & tile & hero & town & bid;
	}
};

template <typename T> class CGoal : public AbstractGoal
{
public:
	CGoal<T> (EGoals goal = INVALID) : AbstractGoal (goal)
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

	void accept (VCAI * ai) override;
	float accept (FuzzyHelper * f) override;

	CGoal<T> * clone() const override
	{
		return new T(static_cast<T const&>(*this)); //casting enforces template instantiation
	}
	TSubgoal iAmElementar()
	{
		setisElementar(true);
		std::shared_ptr<AbstractGoal> ptr;
		ptr.reset(clone());
		return ptr;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<AbstractGoal&> (*this);
		//h & goalType & isElementar & isAbstract & priority;
		//h & value & resID & objid & aid & tile & hero & town & bid;
	}
};

class Invalid : public CGoal<Invalid>
{
	public:
	Invalid() : CGoal (Goals::INVALID) {priority = -1e10;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class Win : public CGoal<Win>
{
	public:
	Win() : CGoal (Goals::WIN) {priority = 100;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class NotLose : public CGoal<NotLose>
{
	public:
	NotLose() : CGoal (Goals::DO_NOT_LOSE) {priority = 100;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	//TSubgoal whatToDoToAchieve() override;
};
class Conquer : public CGoal<Conquer>
{
	public:
	Conquer() : CGoal (Goals::CONQUER) {priority = 10;};
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
};
class Build : public CGoal<Build>
{
	public:
	Build() : CGoal (Goals::BUILD) {priority = 1;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class Explore : public CGoal<Explore>
{
	public:
	Explore() : CGoal (Goals::EXPLORE){priority = 1;};
	Explore(HeroPtr h) : CGoal (Goals::EXPLORE){hero = h; priority = 1;};
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
	bool fulfillsMe (TSubgoal goal) override;
};
class GatherArmy : public CGoal<GatherArmy>
{
public:
	GatherArmy() : CGoal (Goals::GATHER_ARMY){};

	GatherArmy(int val) : CGoal (Goals::GATHER_ARMY){value = val; priority = 2.5;};
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	std::string completeMessage() const override;
};
class BoostHero : public CGoal<BoostHero>
{
	public:
	BoostHero() : CGoal (Goals::INVALID){priority = -1e10;}; //TODO
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	//TSubgoal whatToDoToAchieve() override {return sptr(Invalid());};
};
class RecruitHero : public CGoal<RecruitHero>
{
	public:
	RecruitHero() : CGoal (Goals::RECRUIT_HERO){priority = 1;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class BuildThis : public CGoal<BuildThis>
{
public:
	BuildThis() : CGoal (Goals::BUILD_STRUCTURE){}; //FIXME: should be not allowed (private)

	BuildThis(BuildingID Bid, const CGTownInstance *tid) : CGoal (Goals::BUILD_STRUCTURE) {bid = Bid; town = tid; priority =  5;};
	BuildThis(BuildingID Bid) : CGoal (Goals::BUILD_STRUCTURE) {bid = Bid; priority = 5;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class CollectRes : public CGoal<CollectRes>
{
public:	
	CollectRes() : CGoal (Goals::COLLECT_RES){};

	CollectRes(int rid, int val) : CGoal (Goals::COLLECT_RES) {resID = rid; value = val; priority = 2;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class GatherTroops : public CGoal<GatherTroops>
{
public:
	GatherTroops() : CGoal (Goals::GATHER_TROOPS){priority = 2;};

	GatherTroops(int type, int val) : CGoal (Goals::GATHER_TROOPS){objid = type; value = val; priority = 2;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class GetObj : public CGoal<GetObj>
{
public:
	GetObj() {}; // empty constructor not allowed

	GetObj(int Objid) : CGoal(Goals::GET_OBJ) {objid = Objid; priority = 3;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
	bool operator== (GetObj &g) {return g.objid ==  objid;}
	bool fulfillsMe (TSubgoal goal) override;
	std::string completeMessage() const override;
};
class FindObj : public CGoal<FindObj>
{
public:
	FindObj() {}; // empty constructor not allowed

	FindObj(int ID) : CGoal(Goals::FIND_OBJ) {objid = ID; priority = 1;};
	FindObj(int ID, int subID) : CGoal(Goals::FIND_OBJ) {objid = ID; resID = subID; priority = 1;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class VisitHero : public CGoal<VisitHero>
{
public:
	VisitHero() : CGoal (Goals::VISIT_HERO){};

	VisitHero(int hid) : CGoal (Goals::VISIT_HERO){objid = hid; priority = 4;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
	bool operator== (VisitHero &g) { return g.goalType == goalType && g.objid == objid; }
	bool fulfillsMe (TSubgoal goal) override;
	std::string completeMessage() const override;
};
class GetArtOfType : public CGoal<GetArtOfType>
{
public:
	GetArtOfType() : CGoal (Goals::GET_ART_TYPE){};

	GetArtOfType(int type) : CGoal (Goals::GET_ART_TYPE){aid = type; priority = 2;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
};
class VisitTile : public CGoal<VisitTile>
	//tile, in conjunction with hero elementar; assumes tile is reachable
{
public:
	VisitTile() {}; // empty constructor not allowed

	VisitTile(int3 Tile) : CGoal (Goals::VISIT_TILE) {tile = Tile; priority = 5;};
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool operator== (VisitTile &g) { return g.goalType == goalType && g.tile == tile; }
	std::string completeMessage() const override;
}; 
class ClearWayTo : public CGoal<ClearWayTo>
{
public:
	ClearWayTo() : CGoal (Goals::CLEAR_WAY_TO){};

	ClearWayTo(int3 Tile) : CGoal (Goals::CLEAR_WAY_TO) {tile = Tile; priority = 5;};
	ClearWayTo(int3 Tile, HeroPtr h) : CGoal (Goals::CLEAR_WAY_TO) {tile = Tile; hero = h; priority = 5;};
	TGoalVec getAllPossibleSubgoals() override;
	TSubgoal whatToDoToAchieve() override;
	bool operator== (ClearWayTo &g) { return g.goalType == goalType && g.tile == tile; }
};
class DigAtTile : public CGoal<DigAtTile>
	//elementar with hero on tile
{
public:
	DigAtTile() : CGoal (Goals::DIG_AT_TILE){};

	DigAtTile(int3 Tile) : CGoal (Goals::DIG_AT_TILE) {tile = Tile; priority = 20;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	TSubgoal whatToDoToAchieve() override;
	bool operator== (DigAtTile &g) { return g.goalType == goalType && g.tile == tile; }
};

class CIssueCommand : public CGoal<CIssueCommand>
{
	std::function<bool()> command;

	public:
	CIssueCommand(): CGoal(ISSUE_COMMAND){};

	CIssueCommand(std::function<bool()> _command): CGoal(ISSUE_COMMAND), command(_command) {priority = 1e10;};
	TGoalVec getAllPossibleSubgoals() override {return TGoalVec();};
	//TSubgoal whatToDoToAchieve() override {return sptr(Invalid());};
};

}
