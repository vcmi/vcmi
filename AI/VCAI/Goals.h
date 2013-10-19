#pragma once

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CObjectHandler.h"
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

namespace Goals
{
	struct CGoal;
	typedef CGoal TSubgoal;

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

#define SETTER(type, field) CGoal &set ## field(const type &rhs) { field = rhs; return *this; }
#if 0
	#define SETTER
#endif // _DEBUG

enum {LOW_PR = -1};

struct CGoal
{
	EGoals goalType;
	bool isElementar; SETTER(bool, isElementar)
	bool isAbstract; SETTER(bool, isAbstract) //allows to remember abstract goals
	int priority; SETTER(bool, priority)
	std::string name() const;

	virtual TSubgoal whatToDoToAchieve();

	CGoal(EGoals goal = INVALID) : goalType(goal)
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

	bool invalid() const;

	static TSubgoal goVisitOrLookFor(const CGObjectInstance *obj); //if obj is nullptr, then we'll explore
	static TSubgoal lookForArtSmart(int aid); //checks non-standard ways of obtaining art (merchants, quests, etc.)
	static TSubgoal tryRecruitHero();

	int value; SETTER(int, value)
	int resID; SETTER(int, resID)
	int objid; SETTER(int, objid)
	int aid; SETTER(int, aid)
	int3 tile; SETTER(int3, tile)
	HeroPtr hero; SETTER(HeroPtr, hero)
	const CGTownInstance *town; SETTER(CGTownInstance *, town)
	int bid; SETTER(int, bid)

	bool operator== (CGoal &g)
	{
		switch (goalType)
		{
			case EGoals::GET_OBJ:
				return objid == g.objid;
		}
		return false;
	}


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & goalType & isElementar & isAbstract & priority;
		h & value & resID & objid & aid & tile & hero & town & bid;
	}
};

class Invalid : public CGoal
{
	public:
	Invalid() : CGoal (Goals::INVALID){};
	TSubgoal whatToDoToAchieve() override;
};
class Win : public CGoal
{
	public:
	Win() : CGoal (Goals::WIN){};
	TSubgoal whatToDoToAchieve() override;
};
class NotLose : public CGoal
{
	public:
	NotLose() : CGoal (Goals::DO_NOT_LOSE){};
	TSubgoal whatToDoToAchieve() override;
};
class Conquer : public CGoal
{
	public:
	Conquer() : CGoal (Goals::CONQUER){};
	TSubgoal whatToDoToAchieve() override;
};
class Build : public CGoal
{
	public:
	Build() : CGoal (Goals::BUILD){};
	TSubgoal whatToDoToAchieve() override;
};
class Explore : public CGoal
{
	public:
	Explore() : CGoal (Goals::EXPLORE){};
	TSubgoal whatToDoToAchieve() override;
};
class GatherArmy : public CGoal
{
	public:
	GatherArmy() : CGoal (Goals::GATHER_ARMY){};
	TSubgoal whatToDoToAchieve() override;
};
class BoostHero : public CGoal
{
	public:
	BoostHero() : CGoal (Goals::INVALID){}; //TODO
	TSubgoal whatToDoToAchieve() override;
};
class RecruitHero : public CGoal
{
	public:
	RecruitHero() : CGoal (Goals::RECRUIT_HERO){};
	TSubgoal whatToDoToAchieve() override;
};
class BuildThis : public CGoal
{
	public:
	BuildThis() : CGoal (Goals::BUILD_STRUCTURE){};
	TSubgoal whatToDoToAchieve() override;
};
class CollectRes : public CGoal
{
	public:
	CollectRes() : CGoal (Goals::COLLECT_RES){};
	TSubgoal whatToDoToAchieve() override;
};
class GatherTroops : public CGoal
{
	public:
	GatherTroops() : CGoal (Goals::GATHER_TROOPS){};
	TSubgoal whatToDoToAchieve() override;
};
class GetObj : public CGoal
{
	private:
		GetObj() {}; // empty constructor not allowed
	public:
		GetObj(const int Objid) : CGoal(Goals::GET_OBJ) {setobjid(Objid);};
	TSubgoal whatToDoToAchieve() override;
};
class FindObj : public CGoal
{
	private:
		FindObj() {}; // empty constructor not allowed
	public:
	FindObj(int ID) : CGoal(Goals::FIND_OBJ) {setobjid(ID);};
	FindObj(int ID, int subID) : CGoal(Goals::FIND_OBJ) {setobjid(ID).setresID(subID);};
	TSubgoal whatToDoToAchieve() override;
};
class VisitHero : public CGoal
{
	public:
	VisitHero() : CGoal (Goals::VISIT_HERO){};
	TSubgoal whatToDoToAchieve() override;
};
class GetArtOfType : public CGoal
{
	public:
	GetArtOfType() : CGoal (Goals::GET_ART_TYPE){};
	TSubgoal whatToDoToAchieve() override;
};
class VisitTile : public CGoal
	//tile, in conjunction with hero elementar; assumes tile is reachable
{
	private:
		VisitTile() {}; // empty constructor not allowed
	public:
		VisitTile(int3 Tile) : CGoal (Goals::VISIT_TILE) {settile(Tile);};
	TSubgoal whatToDoToAchieve() override;
}; 
class ClearWayTo : public CGoal
{
	public:
	ClearWayTo(int3 Tile) : CGoal (Goals::CLEAR_WAY_TO) {settile(Tile);};
	TSubgoal whatToDoToAchieve() override;
};
class DigAtTile : public CGoal
	//elementar with hero on tile
{
	public:
	DigAtTile() : CGoal (Goals::DIG_AT_TILE){};
	TSubgoal whatToDoToAchieve() override;
};

class CIssueCommand : CGoal
{
	std::function<bool()> command;

	public:
	CIssueCommand(std::function<bool()> _command): CGoal(ISSUE_COMMAND), command(_command) {}
	TSubgoal whatToDoToAchieve() override;
};

}