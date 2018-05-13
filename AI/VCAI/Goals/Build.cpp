/*
 * Goals.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Goals/Build.h"
#include "VCAI.h"
#include "Fuzzy.h"
#include "SectorMap.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"
#include "Tasks/VisitTile.h"
#include "Tasks/BuildStructure.h"
#include "Tasks/RecruitHero.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;


class Goals::BuildingInfo {
public:
	BuildingID id;
	TResources buildCost;
	int creatureScore;
	int creatureGrows;
	TResources creatureCost;
	TResources dailyIncome;
	CBuilding::TRequired requirements;
	bool exists = false;
	bool canBuild = false;

	BuildingInfo() {
		id = BuildingID::NONE;
		creatureGrows = 0;
		creatureScore = 0;
	}

	BuildingInfo(const CBuilding* building, const CCreature* creature) {
		id = building->bid;
		buildCost = building->resources;
		dailyIncome = building->produce;
		requirements = building->requirements;
		exists = false;

		if (creature) {
			bool isShooting = creature->isShooting();
			int demage = (creature->getMinDamage(isShooting) + creature->getMinDamage(isShooting)) / 2;

			creatureGrows = creature->growth;
			creatureScore = demage;
			creatureCost = creature->cost;
		}
		else {
			creatureScore = 0;
			creatureGrows = 0;
			creatureCost = TResources();
		}
	}

	std::string toString() {
		return std::to_string(id) + ", cost: " + buildCost.toString()
			+ ", score: " + std::to_string(creatureGrows) + " x " + std::to_string(creatureScore)
			+ " x " + creatureCost.toString()
			+ ", daily: " + dailyIncome.toString();
	}

	double costPerScore() {
		if (creatureScore == 0) return 0;

		double gold = creatureCost[Res::ERes::GOLD];

		return (double)creatureScore / (double)gold;
	}
};

class TownDevelopmentInfo {
public:
	const CGTownInstance* town;
	BuildingID nextDwelling;
	BuildingID nextGoldSource;
	int armyScore;
	int economicsScore;

	TownDevelopmentInfo(const CGTownInstance* town) {
		this->town = town;
		this->armyScore = 0;
		this->economicsScore = 0;
	}
};

BuildingInfo Build::getBuildingOrPrerequisite(
	const CGTownInstance* town,
	BuildingID toBuild,
	TResources &requiredResourcesAccumulator,
	TResources &totalDevelopmentCostAccumulator,
	bool excludeDwellingDependencies)
{
	BuildingID building = toBuild;
	auto townInfo = town->town;

	const CBuilding * buildPtr = townInfo->buildings.at(building);
	const CCreature* creature = NULL;

	if (BuildingID::DWELL_FIRST <= toBuild && toBuild <= BuildingID::DWELL_UP_LAST) {
		int level = toBuild - BuildingID::DWELL_FIRST;
		auto creatureID = townInfo->creatures.at(level % GameConstants::CREATURES_PER_TOWN).at(level / GameConstants::CREATURES_PER_TOWN);
		creature = creatureID.toCreature();
	}

	while (buildPtr) {
		auto info = BuildingInfo(buildPtr, creature);

		logAi->trace("checking %s", buildPtr->Name());
		logAi->trace("buildInfo %s", info.toString());

		buildPtr = NULL;

		if (!town->hasBuilt(building)) {
			auto canBuild = cb->canBuildStructure(town, building);

			totalDevelopmentCostAccumulator += info.buildCost;

			if (canBuild == EBuildingState::ALLOWED) {
				info.canBuild = true;

				return info;
			}
			else if (canBuild == EBuildingState::NO_RESOURCES) {
				logAi->trace("cant build. Not enough resources. Need %s", info.buildCost.toString());
				requiredResourcesAccumulator += info.buildCost;
			}
			else if (canBuild == EBuildingState::PREREQUIRES) {
				auto buildExpression = town->genBuildingRequirements(building, false);
				auto alreadyBuiltOrOtherDwelling = [&](const BuildingID & id) -> bool {
					return BuildingID::DWELL_FIRST <= id && id <= BuildingID::DWELL_LAST || town->hasBuilt(id);
				};

				auto missingBuildings = buildExpression.getFulfillmentCandidates(alreadyBuiltOrOtherDwelling);

				if (missingBuildings.empty()) {
					logAi->trace("cant build. Need other dwelling");
				}
				else {
					building = missingBuildings[0];
					buildPtr = townInfo->buildings.at(building);
					creature = NULL;

					logAi->trace("cant build. Need %s", buildPtr->Name());
				}
			}
		}
		else {
			logAi->trace("exists");
			info.exists = true;

			return info;
		}
	}

	return BuildingInfo();
}

Tasks::TaskList Build::getTasks()
{
	Tasks::TaskList tasks = Tasks::TaskList();
	TResources availableResources = cb->getResourceAmount();

	logAi->trace("Searching tasks for BUILD");
	logAi->trace("Resources amount: %s", availableResources.toString());

	auto objects = cb->getMyObjects();
	TResources dailyIncome = TResources();

	for (const CGObjectInstance* obj : objects) {
		const CGMine* mine = dynamic_cast<const CGMine*>(obj);

		if (mine) {
			dailyIncome[mine->producedResource] += mine->producedQuantity;
		}
	}

	auto towns = cb->getTownsInfo();
	TResources requiredResources = TResources();
	TResources totalDevelopmentCost = TResources();
	auto developmentInfos = std::vector<TownDevelopmentInfo>();

	TResources armyCost = TResources();

	for (const CGTownInstance* town : towns) {
		auto townInfo = town->town;
		auto creatures = townInfo->creatures;
		auto buildings = townInfo->getAllBuildings();

		auto developmentInfo = TownDevelopmentInfo(town);

		dailyIncome += town->dailyIncome();;

		std::map<BuildingID, BuildingID> parentMap;

		for (auto &pair : townInfo->buildings) {
			if (pair.second->upgrade != -1) {
				parentMap[pair.second->upgrade] = pair.first;
			}
		}

		std::vector<BuildingInfo> existingDwellings;
		std::vector<BuildingInfo> toBuild;
		TResources townDevelopmentCost;

		for (int level = 0; level < GameConstants::CREATURES_PER_TOWN; level++)
		{
			BuildingID building = BuildingID(BuildingID::DWELL_FIRST + level);

			if (!vstd::contains(buildings, building))
				continue; // no such building in town

			BuildingInfo info = getBuildingOrPrerequisite(town, building, requiredResources, townDevelopmentCost);

			if (info.exists) {
				existingDwellings.push_back(info);
				armyCost += info.creatureCost * info.creatureGrows;
			}
			else if (info.canBuild) {
				toBuild.push_back(info);
			}
		}

		std::sort(toBuild.begin(), toBuild.end(), [](BuildingInfo& b1, BuildingInfo& b2) -> bool {
			return b1.costPerScore() > b2.costPerScore();
		});

		if (!town->hasBuilt(BuildingID::TOWN_HALL) && cb->canBuildStructure(town, BuildingID::TOWN_HALL) == EBuildingState::ALLOWED) {
			auto buildingInfo = getBuildingOrPrerequisite(town, BuildingID::TOWN_HALL, requiredResources, TResources());

			if (buildingInfo.canBuild) {
				tasks.push_back(Tasks::sptr(Tasks::BuildStructure(BuildingID::TOWN_HALL, town)));
				continue;
			}
		}

		if (toBuild.size() > 0) {
			developmentInfo.nextDwelling = toBuild[0].id;
		}

		if (existingDwellings.size() >= 2 && cb->getDate(Date::DAY_OF_WEEK) > boost::date_time::Friday) {
			if (!town->hasBuilt(BuildingID::CITADEL)) {
				if (cb->canBuildStructure(town, BuildingID::CITADEL) == EBuildingState::ALLOWED) {
					developmentInfo.nextDwelling = BuildingID::CITADEL;
					logAi->trace("Building preferences Citadel");
				}
			}

			else if (existingDwellings.size() >= 3 && !town->hasBuilt(BuildingID::CASTLE)) {
				if (cb->canBuildStructure(town, BuildingID::CASTLE) == EBuildingState::ALLOWED) {
					developmentInfo.nextDwelling = BuildingID::CASTLE;
					logAi->trace("Building preferences CASTLE");
				}
			}
		}

		for (auto bi : toBuild) {
			logAi->trace("Building preferences %s", bi.toString());
		}

		if (!town->hasBuilt(BuildingID::CITY_HALL)) {
			auto buildingInfo = getBuildingOrPrerequisite(town, BuildingID::CITY_HALL, requiredResources, TResources());

			developmentInfo.nextGoldSource = buildingInfo.id;
		}
		else {
			if (cb->canBuildStructure(town, BuildingID::CAPITOL) == EBuildingState::ALLOWED) {
				developmentInfo.nextGoldSource = BuildingID::CAPITOL;
			}
			// todo resource silo and treasury
		}

		if (developmentInfo.nextDwelling == BuildingID::NONE) {
			if (developmentInfo.nextGoldSource == BuildingID::NONE) {
				continue;
			}

			developmentInfo.armyScore = 1000; // nothing to build for army here
		}
		else {
			const int resourceMultiplier = 200;
			developmentInfo.armyScore = townDevelopmentCost[Res::GOLD]
				+ townDevelopmentCost[Res::WOOD] * resourceMultiplier
				+ townDevelopmentCost[Res::ORE] * resourceMultiplier
				+ townDevelopmentCost[Res::GEMS] * 2 * resourceMultiplier
				+ townDevelopmentCost[Res::SULFUR] * 2 * resourceMultiplier
				+ townDevelopmentCost[Res::CRYSTAL] * 2 * resourceMultiplier
				+ townDevelopmentCost[Res::MERCURY] * 2 * resourceMultiplier;

			totalDevelopmentCost += townDevelopmentCost;
		}

		developmentInfos.push_back(developmentInfo);
	}

	logAi->trace("daily income: %s", dailyIncome.toString());
	logAi->trace("resources required to develop towns now: %s, total: %s",
		requiredResources.toString(),
		totalDevelopmentCost.toString());

	//TODO: we need to try enforce capturing resources of particular type if we need them.

	TResources weeklyIncome = dailyIncome * 7;
	std::vector<BuildingID> result;

	armyCost *= 3; // multiply a few times to have gold for town development

	std::sort(developmentInfos.begin(), developmentInfos.end(), [&](TownDevelopmentInfo &b1, TownDevelopmentInfo &b2) -> bool {
		return b1.armyScore < b2.armyScore;
	});

	int i = 0;

	if (weeklyIncome.canAfford(armyCost)) {
		for (; i < (developmentInfos.size() + 1) / 2; ++i) {
			auto toBuild = developmentInfos[i].nextDwelling;

			if (toBuild != BuildingID::NONE) {
				tasks.push_back(Tasks::sptr(Tasks::BuildStructure(toBuild, developmentInfos[i].town)));
			}
		}
	}

	for (; i < developmentInfos.size(); ++i) {
		auto toBuild = developmentInfos[i].nextGoldSource;

		if (toBuild != BuildingID::NONE) {
			tasks.push_back(Tasks::sptr(Tasks::BuildStructure(toBuild, developmentInfos[i].town)));
		}
	}

	return tasks;
}