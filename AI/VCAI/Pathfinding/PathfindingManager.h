/*
* PathfindingManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../VCAI.h"
#include "AINodeStorage.h"

class IPathfindingManager
{
public:
	virtual ~IPathfindingManager() = default;
	virtual void init(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;

	virtual void resetPaths() = 0;
	virtual Goals::TGoalVec howToVisitTile(HeroPtr hero, int3 tile, bool allowGatherArmy = true) = 0;
	virtual Goals::TGoalVec howToVisitObj(HeroPtr hero, ObjectIdRef obj, bool allowGatherArmy = true) = 0;
	virtual Goals::TGoalVec howToVisitTile(int3 tile) = 0;
	virtual Goals::TGoalVec howToVisitObj(ObjectIdRef obj) = 0;
	virtual std::vector<AIPath> getPathsToTile(HeroPtr hero, int3 tile) = 0;
};
	
class PathfindingManager : public IPathfindingManager
{
	friend class AIhelper;

private:
	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;
	std::unique_ptr<AIPathfinder> pathfinder;

public:
	PathfindingManager() = default;
	PathfindingManager(CPlayerSpecificInfoCallback * CB, VCAI * AI = nullptr); //for tests only

	Goals::TGoalVec howToVisitTile(HeroPtr hero, int3 tile, bool allowGatherArmy = true) override;
	Goals::TGoalVec howToVisitObj(HeroPtr hero, ObjectIdRef obj, bool allowGatherArmy = true) override;
	Goals::TGoalVec howToVisitTile(int3 tile) override;
	Goals::TGoalVec howToVisitObj(ObjectIdRef obj) override;
	std::vector<AIPath> getPathsToTile(HeroPtr hero, int3 tile) override;
	void resetPaths() override;

private:
	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;

	Goals::TGoalVec findPath(
		HeroPtr hero, 
		crint3 dest,
		bool allowGatherArmy, 
		const std::function<Goals::TSubgoal(int3)> goalFactory);

	Goals::TSubgoal clearWayTo(HeroPtr hero, int3 firstTileToGet);
};
