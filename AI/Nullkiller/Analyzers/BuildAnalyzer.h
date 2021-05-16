/*
* BuildAnalyzer.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../VCAI.h"
#include "../../../lib/ResourceSet.h"


class DLL_EXPORT BuildingInfo
{
public:
	BuildingID id;
	TResources buildCost;
	TResources buildCostWithPrerequisits;
	int creatureGrows;
	uint8_t creatureLevel;
	TResources creatureCost;
	CreatureID creatureID;
	CreatureID baseCreatureID;
	TResources dailyIncome;
	uint8_t prerequisitesCount;
	std::string name;
	bool exists = false;
	bool canBuild = false;
	bool notEnoughRes = false;

	BuildingInfo();

	BuildingInfo(const CBuilding* building, const CCreature* creature, CreatureID baseCreatureID);

	std::string toString() const;
};

class DLL_EXPORT TownDevelopmentInfo
{
public:
	const CGTownInstance* town;
	std::vector<BuildingInfo> toBuild;
	std::vector<BuildingInfo> existingDwellings;
	TResources townDevelopmentCost;
	TResources requiredResources;
	TResources armyCost;
	int armyScore;
	int economicsScore;
	HeroRole townRole;
	bool hasSomethingToBuild;

	TownDevelopmentInfo(const CGTownInstance* town)
		:town(town), armyScore(0), economicsScore(0), toBuild(), townDevelopmentCost(), requiredResources(), townRole(HeroRole::SCOUT), hasSomethingToBuild(false)
	{
	}

	TownDevelopmentInfo() : TownDevelopmentInfo(nullptr) {}

	void addBuildingToBuild(const BuildingInfo & building);
	void addExistingDwelling(const BuildingInfo & existingDwelling);
};

class DLL_EXPORT BuildAnalyzer
{
private:
	TResources requiredResources;
	TResources totalDevelopmentCost;
	std::vector<TownDevelopmentInfo> developmentInfos;
	TResources armyCost;
	TResources dailyIncome;
	float goldPreasure;

public:
	void update();

	TResources getResourcesRequiredNow() const;
	TResources getTotalResourcesRequired() const;
	const std::vector<TownDevelopmentInfo> & getDevelopmentInfo() const { return developmentInfos; }
	TResources getDailyIncome() const { return dailyIncome; }
	float getGoldPreasure() const { return goldPreasure; }
	bool hasAnyBuilding(int32_t alignment, BuildingID bid) const;

private:
	BuildingInfo getBuildingOrPrerequisite(
		const CGTownInstance* town,
		BuildingID toBuild,
		bool excludeDwellingDependencies = true) const;


	void updateTownDwellings(TownDevelopmentInfo & developmentInfo);
	void updateOtherBuildings(TownDevelopmentInfo & developmentInfo);
	void updateDailyIncome();
	void reset();
};
