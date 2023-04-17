/*
* BuildAnalyzer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../Engine/Nullkiller.h"

namespace NKAI
{

void BuildAnalyzer::updateTownDwellings(TownDevelopmentInfo & developmentInfo)
{
	auto townInfo = developmentInfo.town->town;
	auto creatures = townInfo->creatures;
	auto buildings = townInfo->getAllBuildings();

	std::map<BuildingID, BuildingID> parentMap;

	for(auto &pair : townInfo->buildings)
	{
		if(pair.second->upgrade != -1)
		{
			parentMap[pair.second->upgrade] = pair.first;
		}
	}

	BuildingID prefixes[] = {BuildingID::DWELL_UP_FIRST, BuildingID::DWELL_FIRST};

	for(int level = 0; level < GameConstants::CREATURES_PER_TOWN; level++)
	{
		logAi->trace("Checking dwelling level %d", level);
		BuildingInfo nextToBuild = BuildingInfo();

		for(BuildingID prefix : prefixes)
		{
			BuildingID building = BuildingID(prefix + level);

			if(!vstd::contains(buildings, building))
				continue; // no such building in town

			auto info = getBuildingOrPrerequisite(developmentInfo.town, building);

			if(info.exists)
			{
				developmentInfo.addExistingDwelling(info);

				break;
			}

			nextToBuild = info;
		}

		if(nextToBuild.id != BuildingID::NONE)
		{
			developmentInfo.addBuildingToBuild(nextToBuild);
		}
	}
}

void BuildAnalyzer::updateOtherBuildings(TownDevelopmentInfo & developmentInfo)
{
	logAi->trace("Checking other buildings");

	std::vector<std::vector<BuildingID>> otherBuildings = {
		{BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL}
	};

	if(developmentInfo.existingDwellings.size() >= 2 && ai->cb->getDate(Date::DAY_OF_WEEK) > boost::date_time::Friday)
	{
		otherBuildings.push_back({BuildingID::CITADEL, BuildingID::CASTLE});
	}

	for(auto & buildingSet : otherBuildings)
	{
		for(auto & buildingID : buildingSet)
		{
			if(!developmentInfo.town->hasBuilt(buildingID))
			{
				developmentInfo.addBuildingToBuild(getBuildingOrPrerequisite(developmentInfo.town, buildingID));

				break;
			}
		}
	}
}

int32_t convertToGold(const TResources & res)
{
	return res[EGameResID::GOLD] 
		+ 75 * (res[EGameResID::WOOD] + res[EGameResID::ORE]) 
		+ 125 * (res[EGameResID::GEMS] + res[EGameResID::CRYSTAL] + res[EGameResID::MERCURY] + res[EGameResID::SULFUR]);
}

TResources BuildAnalyzer::getResourcesRequiredNow() const
{
	auto resourcesAvailable = ai->getFreeResources();
	auto result = requiredResources - resourcesAvailable;

	result.positive();

	return result;
}

TResources BuildAnalyzer::getTotalResourcesRequired() const
{
	auto resourcesAvailable = ai->getFreeResources();
	auto result = totalDevelopmentCost - resourcesAvailable;

	result.positive();

	return result;
}

void BuildAnalyzer::update()
{
	logAi->trace("Start analysing build");

	BuildingInfo bi;

	reset();

	auto towns = ai->cb->getTownsInfo();

	for(const CGTownInstance* town : towns)
	{
		logAi->trace("Checking town %s", town->getNameTranslated());

		developmentInfos.emplace_back(town);
		TownDevelopmentInfo & developmentInfo = developmentInfos.back();

		updateTownDwellings(developmentInfo);
		updateOtherBuildings(developmentInfo);

		requiredResources += developmentInfo.requiredResources;
		totalDevelopmentCost += developmentInfo.townDevelopmentCost;
		armyCost += developmentInfo.armyCost;

		for(auto bi : developmentInfo.toBuild)
		{
			logAi->trace("Building preferences %s", bi.toString());
		}
	}

	std::sort(developmentInfos.begin(), developmentInfos.end(), [](const TownDevelopmentInfo & t1, const TownDevelopmentInfo & t2) -> bool
	{
		auto val1 = convertToGold(t1.armyCost) - convertToGold(t1.townDevelopmentCost);
		auto val2 = convertToGold(t2.armyCost) - convertToGold(t2.townDevelopmentCost);

		return val1 > val2;
	});

	updateDailyIncome();

	if(ai->cb->getDate(Date::EDateType::DAY) == 1)
	{
		goldPreasure = 1;
	}
	else
	{
		goldPreasure = ai->getLockedResources()[EGameResID::GOLD] / 10000.0f
			+ (float)armyCost[EGameResID::GOLD] / (1 + ai->getFreeGold() + (float)dailyIncome[EGameResID::GOLD] * 7.0f);
	}

	logAi->trace("Gold preasure: %f", goldPreasure);
}

void BuildAnalyzer::reset()
{
	requiredResources = TResources();
	totalDevelopmentCost = TResources();
	armyCost = TResources();
	developmentInfos.clear();
}

BuildingInfo BuildAnalyzer::getBuildingOrPrerequisite(
	const CGTownInstance * town,
	BuildingID toBuild,
	bool excludeDwellingDependencies) const
{
	BuildingID building = toBuild;
	auto townInfo = town->town;

	const CBuilding * buildPtr = townInfo->buildings.at(building);
	const CCreature * creature = nullptr;
	CreatureID baseCreatureID;

	if(BuildingID::DWELL_FIRST <= toBuild && toBuild <= BuildingID::DWELL_UP_LAST)
	{
		int level = toBuild - BuildingID::DWELL_FIRST;
		auto creatures = townInfo->creatures.at(level % GameConstants::CREATURES_PER_TOWN);
		auto creatureID = creatures.size() > level / GameConstants::CREATURES_PER_TOWN
			? creatures.at(level / GameConstants::CREATURES_PER_TOWN)
			: creatures.front();

		baseCreatureID = creatures.front();
		creature = creatureID.toCreature();
	}

	auto info = BuildingInfo(buildPtr, creature, baseCreatureID, town, ai);

	logAi->trace("checking %s", info.name);
	logAi->trace("buildInfo %s", info.toString());

	if(!town->hasBuilt(building))
	{
		auto canBuild = ai->cb->canBuildStructure(town, building);

		if(canBuild == EBuildingState::ALLOWED)
		{
			info.canBuild = true;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			logAi->trace("cant build. Not enough resources. Need %s", info.buildCost.toString());
			info.notEnoughRes = true;
		}
		else if(canBuild == EBuildingState::PREREQUIRES)
		{
			auto buildExpression = town->genBuildingRequirements(building, false);
			auto missingBuildings = buildExpression.getFulfillmentCandidates([&](const BuildingID & id) -> bool
			{
				return town->hasBuilt(id);
			});

			auto otherDwelling = [](const BuildingID & id) -> bool
			{
				return BuildingID::DWELL_FIRST <= id && id <= BuildingID::DWELL_UP_LAST;
			};

			if(vstd::contains_if(missingBuildings, otherDwelling))
			{
				logAi->trace("cant build. Need other dwelling");
			}
			else
			{
				logAi->trace("cant build. Need %d", missingBuildings[0].num);

				BuildingInfo prerequisite = getBuildingOrPrerequisite(town, missingBuildings[0], excludeDwellingDependencies);

				prerequisite.buildCostWithPrerequisits += info.buildCost;
				prerequisite.creatureCost = info.creatureCost;
				prerequisite.creatureGrows = info.creatureGrows;
				prerequisite.creatureLevel = info.creatureLevel;
				prerequisite.creatureID = info.creatureID;
				prerequisite.baseCreatureID = info.baseCreatureID;
				prerequisite.prerequisitesCount++;
				prerequisite.armyCost = info.armyCost;
				prerequisite.dailyIncome = info.dailyIncome;

				return prerequisite;
			}
		}
	}
	else
	{
		logAi->trace("exists");
		info.exists = true;
	}

	return info;
}

void BuildAnalyzer::updateDailyIncome()
{
	auto objects = ai->cb->getMyObjects();
	auto towns = ai->cb->getTownsInfo();
	
	dailyIncome = TResources();

	for(const CGObjectInstance* obj : objects)
	{
		const CGMine* mine = dynamic_cast<const CGMine*>(obj);

		if(mine)
		{
			dailyIncome[mine->producedResource.getNum()] += mine->producedQuantity;
		}
	}

	for(const CGTownInstance* town : towns)
	{
		dailyIncome += town->dailyIncome();
	}
}

bool BuildAnalyzer::hasAnyBuilding(int32_t alignment, BuildingID bid) const
{
	for(auto tdi : developmentInfos)
	{
		if(tdi.town->subID == alignment && tdi.town->hasBuilt(bid))
			return true;
	}

	return false;
}

void TownDevelopmentInfo::addExistingDwelling(const BuildingInfo & existingDwelling)
{
	existingDwellings.push_back(existingDwelling);

	armyCost += existingDwelling.armyCost;
	armyStrength += existingDwelling.armyStrength;
}

void TownDevelopmentInfo::addBuildingToBuild(const BuildingInfo & nextToBuild)
{
	townDevelopmentCost += nextToBuild.buildCostWithPrerequisits;

	if(nextToBuild.canBuild)
	{
		hasSomethingToBuild = true;
		toBuild.push_back(nextToBuild);
	}
	else if(nextToBuild.notEnoughRes)
	{
		requiredResources += nextToBuild.buildCost;
		hasSomethingToBuild = true;
		toBuild.push_back(nextToBuild);
	}
}

BuildingInfo::BuildingInfo()
{
	id = BuildingID::NONE;
	creatureGrows = 0;
	creatureID = CreatureID::NONE;
	buildCost = 0;
	buildCostWithPrerequisits = 0;
	prerequisitesCount = 0;
	name.clear();
	armyStrength = 0;
}

BuildingInfo::BuildingInfo(
	const CBuilding * building,
	const CCreature * creature,
	CreatureID baseCreature,
	const CGTownInstance * town,
	Nullkiller * ai)
{
	id = building->bid;
	buildCost = building->resources;
	buildCostWithPrerequisits = building->resources;
	dailyIncome = building->produce;
	exists = town->hasBuilt(id);
	prerequisitesCount = 1;
	name = building->getNameTranslated();

	if(creature)
	{
		creatureGrows = creature->getGrowth();
		creatureID = creature->getId();
		creatureCost = creature->getFullRecruitCost();
		creatureLevel = creature->getLevel();
		baseCreatureID = baseCreature;

		if(exists)
		{
			creatureGrows = town->creatureGrowth(creatureLevel - 1);
		}
		else
		{
			creatureGrows = creature->getGrowth();

			if(town->hasBuilt(BuildingID::CASTLE))
				creatureGrows *= 2;
			else if(town->hasBuilt(BuildingID::CITADEL))
				creatureGrows += creatureGrows / 2;
		}

		armyStrength = ai->armyManager->evaluateStackPower(creature, creatureGrows);
		armyCost = creatureCost * creatureGrows;
	}
	else
	{
		creatureGrows = 0;
		creatureID = CreatureID::NONE;
		baseCreatureID = CreatureID::NONE;
		creatureCost = TResources();
		armyCost = TResources();
		creatureLevel = 0;
		armyStrength = 0;
	}
}

std::string BuildingInfo::toString() const
{
	return name + ", cost: " + buildCost.toString()
		+ ", creature: " + std::to_string(creatureGrows) + " x " + std::to_string(creatureLevel)
		+ " x " + creatureCost.toString()
		+ ", daily: " + dailyIncome.toString();
}

}
