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
	BuildingID id = BuildingID::NONE;
	TResources buildCost;
	TResources buildCostWithPrerequisites;
	int creatureGrowth = 0;
	uint8_t creatureLevel = 0; /// @link CCreature::level
	TResources creatureUnitCost;
	CreatureID creatureID = CreatureID::NONE;
	CreatureID baseCreatureID = CreatureID::NONE;
	TResources dailyIncome;
	uint8_t prerequisitesCount = 0;
	uint64_t armyStrength = 0;
	TResources armyCost; // creatureCost * creatureGrows
	std::string name;
	bool isBuilt = false;
	bool isBuildable = false;
	bool isMissingResources = false;

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
	std::vector<BuildingInfo> built;
	TResources townDevelopmentCost;
	TResources requiredResources;
	TResources armyCost;
	uint64_t armyStrength = 0;

	TownDevelopmentInfo(const CGTownInstance * town) : town(town) {}
	TownDevelopmentInfo() : TownDevelopmentInfo(nullptr) {}

	void addBuildingToBuild(const BuildingInfo & bi);
	void addBuildingBuilt(const BuildingInfo & bi);
};

class DLL_EXPORT BuildAnalyzer
{
	TResources requiredResources;
	TResources totalDevelopmentCost;
	std::vector<TownDevelopmentInfo> developmentInfos;
	TResources armyCost;
	TResources dailyIncome;
	float goldPressure = 0;
	Nullkiller * aiNk;

public:
	virtual ~BuildAnalyzer() = default;
	explicit BuildAnalyzer(Nullkiller * aiNk) : aiNk(aiNk) {}
	void update();

	TResources getMissingResourcesNow(float armyGoldRatio = 0) const;
	TResources getMissingResourcesInTotal(float armyGoldRatio = 0) const;
	TResources getFreeResourcesAfterMissingTotal(float armyGoldRatio = 0) const;
	const std::vector<TownDevelopmentInfo> & getDevelopmentInfo() const { return developmentInfos; }
	TResources getDailyIncome() const { return dailyIncome; }
	float getGoldPressure() const { return goldPressure; }
	virtual bool isGoldPressureOverMax() const;
	bool isBuilt(FactionID alignment, BuildingID bid) const;

	void reset();

	static float calculateGoldPressure(TResource lockedGold, float armyCostGold, float economyDevelopmentCost, float freeGold, float dailyIncomeGold);
	static TResources calculateDailyIncome(const std::vector<const CGObjectInstance *> & objects, const std::vector<const CGTownInstance *> & townInfos);
	static void updateDwellings(TownDevelopmentInfo& developmentInfo, std::unique_ptr<ArmyManager>& armyManager, std::shared_ptr<CCallback>& cc);
	static void updateOtherBuildings(TownDevelopmentInfo& developmentInfo, std::unique_ptr<ArmyManager>& armyManager, std::shared_ptr<CCallback>& cc);
	static BuildingInfo getBuildingOrPrerequisite(
		const CGTownInstance* town,
		BuildingID b,
		std::unique_ptr<ArmyManager> & armyManager,
		std::shared_ptr<CCallback> & cc,
		bool excludeDwellingDependencies = true);
	static TResource goldApproximate(const TResources & res);
	static TResource goldApproximate(const TResource & res, EGameResID resId);
	static TResources goldRemove(TResources other);
	static TResources goldOnly(TResources other);
};

}
