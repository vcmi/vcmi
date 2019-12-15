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

class DLL_EXPORT IPathfindingManager
{
public:
	virtual ~IPathfindingManager() = default;
	virtual void init(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;

	virtual void updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain = false) = 0;
	virtual std::vector<AIPath> getPathsToTile(const HeroPtr & hero, const int3 & tile) const = 0;
	virtual std::vector<AIPath> getPathsToTile(const int3 & tile) const = 0;
};

class DLL_EXPORT PathfindingManager : public IPathfindingManager
{
	friend class AIhelper;

private:
	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;
	std::unique_ptr<AIPathfinder> pathfinder;

public:
	PathfindingManager() = default;
	PathfindingManager(CPlayerSpecificInfoCallback * CB, VCAI * AI = nullptr); //for tests only

	std::vector<AIPath> getPathsToTile(const HeroPtr & hero, const int3 & tile) const override;
	std::vector<AIPath> getPathsToTile(const int3 & tile) const override;
	void updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain = false) override;

	STRONG_INLINE
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const
	{
		return pathfinder->isTileAccessible(hero, tile);
	}

private:
	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;

	Goals::TGoalVec findPaths(
		crint3 dest,
		bool allowGatherArmy,
		HeroPtr hero,
		const std::function<Goals::TSubgoal(int3)> goalFactory) const;

	Goals::TSubgoal clearWayTo(HeroPtr hero, int3 firstTileToGet) const;
};
