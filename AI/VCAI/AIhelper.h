/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

/*
	!!!	Note: Include THIS file at the end of include list to avoid "undefined base class" error
*/

#include "ResourceManager.h"
#include "BuildingManager.h"

class ResourceManager;
class BuildingManager;


//indirection interface for various modules
class DLL_EXPORT AIhelper : public IResourceManager, public IBuildingManager
{
	friend class VCAI;
	friend struct SetGlobalState; //mess?

	std::shared_ptr<ResourceManager> resourceManager;
	std::shared_ptr<BuildingManager> buildingManager;
	//TODO: vector<IAbstractManager>
public:
	AIhelper();
	~AIhelper();

	//from ResourceManager
	bool canAfford(const TResources & cost) const;
	TResources reservedResources() const override;
	TResources freeResources() const override;
	TResource freeGold() const override;
	TResources allResources() const override;
	TResource allGold() const override;

	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal) override;
	Goals::TSubgoal whatToDo() const override; //peek highest-priority goal
	bool containsObjective(Goals::TSubgoal goal) const;
	bool hasTasksLeft() const override;

private:
	bool notifyGoalCompleted(Goals::TSubgoal goal);

	void setCB(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;

	//from BuildingManager
public:
	bool getBuildingOptions(const CGTownInstance * t) override;
	boost::optional<PotentialBuilding> immediateBuilding() const override;
	boost::optional<PotentialBuilding> expensiveBuilding() const override;
	boost::optional<BuildingID> canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays = 7) const override;
};

