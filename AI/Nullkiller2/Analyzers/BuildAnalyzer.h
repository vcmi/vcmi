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

#include "../AIUtility.h"
#include "../../../lib/ResourceSet.h"

namespace NK2AI
{
class ArmyManager;

class Nullkiller;

class DLL_EXPORT BuildingInfo
{
public:
	BuildingID id;
	TResources buildCost;
	TResources buildCostWithPrerequisites;
	int creatureGrows;
	uint8_t creatureLevel;
	TResources creatureCost;
	CreatureID creatureID;
	CreatureID baseCreatureID;
	TResources dailyIncome;
	uint8_t prerequisitesCount;
	uint64_t armyStrength;
	TResources armyCost;
	std::string name;
	bool exists = false;
	bool canBuild = false;
	bool notEnoughRes = false;

	BuildingInfo();

	BuildingInfo(
		const CBuilding * building,
		const CCreature * creature,
		CreatureID baseCreature,
		const CGTownInstance * town,
		const std::unique_ptr<ArmyManager> & armyManager);

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
	uint64_t armyStrength;
	HeroRole townRole;
	bool hasSomethingToBuild;

	TownDevelopmentInfo(const CGTownInstance * town):
		town(town),
		armyStrength(0),
		townRole(HeroRole::SCOUT),
		hasSomethingToBuild(false)
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
	float goldPressure;
	Nullkiller * ai;

public:
	BuildAnalyzer(Nullkiller * ai) : ai(ai) {}
	void update();

	TResources getResourcesRequiredNow() const;
	TResources getTotalResourcesRequired() const;
	const std::vector<TownDevelopmentInfo> & getDevelopmentInfo() const { return developmentInfos; }
	TResources getDailyIncome() const { return dailyIncome; }
	float getGoldPressure() const { return goldPressure; }
	bool isGoldPressureOverMax() const;
	bool isBuilt(FactionID alignment, BuildingID bid) const;

	void reset();

	static float calculateGoldPressure(TResource lockedGold, float armyCostGold, float economyDevelopmentCost, float freeGold, float dailyIncomeGold);
	static TResources calculateDailyIncome(std::vector<const CGObjectInstance *> objects, std::vector<const CGTownInstance *> townInfos);
	static void updateTownDwellings(TownDevelopmentInfo& developmentInfo, std::unique_ptr<ArmyManager>& armyManager, std::shared_ptr<CCallback>& cb);
	static void updateOtherBuildings(TownDevelopmentInfo& developmentInfo, std::unique_ptr<ArmyManager>& armyManager, std::shared_ptr<CCallback>& cb);
	static BuildingInfo getBuildingOrPrerequisite(
		const CGTownInstance* town,
		BuildingID toBuild,
		std::unique_ptr<ArmyManager> & armyManager,
		std::shared_ptr<CCallback> & cb,
		bool excludeDwellingDependencies = true);
	static int32_t approximateInGold(const TResources & res);
	static TResources withoutGold(TResources other);
};

}
