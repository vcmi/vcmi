/*
* BuildingManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AIUtility.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CBuildingHandler.h"
#include "VCAI.h"

struct DLL_EXPORT PotentialBuilding
{
	BuildingID bid;
	TResources price;
	//days to build?
};

class DLL_EXPORT IBuildingManager //: public: IAbstractManager
{ //info about town development
public:
	virtual ~IBuildingManager() = default;
	virtual void init(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;

	virtual bool getBuildingOptions(const CGTownInstance * t) = 0;
	virtual std::optional<PotentialBuilding> immediateBuilding() const = 0;
	virtual std::optional<PotentialBuilding> expensiveBuilding() const = 0;
	virtual std::optional<BuildingID> canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays) const = 0;
};

class DLL_EXPORT BuildingManager : public IBuildingManager
{
	friend class VCAI;
	friend class AIhelper;
	friend struct SetGlobalState;

	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;

public:

	//try build anything in given town, and execute resulting Goal if any
	bool getBuildingOptions(const CGTownInstance * t) override;
	BuildingID getMaxPossibleGoldBuilding(const CGTownInstance * t);
	std::optional<PotentialBuilding> immediateBuilding() const override;
	std::optional<PotentialBuilding> expensiveBuilding() const override;
	std::optional<BuildingID> canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays = 7) const override;

protected:

	bool tryBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays = 7);
	//try build first unbuilt structure
	bool tryBuildThisStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays = 7);
	//try build ANY unbuilt structure
	
	bool tryBuildNextStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays = 7);

private:
	//TODO: remember current town?
	std::vector<PotentialBuilding> immediateBuildings; //what we can build right now in current town
	std::vector<PotentialBuilding> expensiveBuildings; //what we coudl build but can't afford

	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
};
