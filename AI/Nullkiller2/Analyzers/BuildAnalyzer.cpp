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
#include "../Engine/Nullkiller.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/IGameSettings.h"

namespace NKAI
{

void BuildAnalyzer::updateTownDwellings(TownDevelopmentInfo & developmentInfo)
{
	for(int level = 0; level < developmentInfo.town->getTown()->creatures.size(); level++)
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Checking dwelling level %d", level);
#endif
		std::vector<BuildingID> dwellingsInTown;

		for(BuildingID buildID = BuildingID::getDwellingFromLevel(level, 0); buildID.hasValue(); BuildingID::advanceDwelling(buildID))
			if(developmentInfo.town->getTown()->buildings.count(buildID) != 0)
				dwellingsInTown.push_back(buildID);

		// find best, already built dwelling
		for (const auto & buildID : boost::adaptors::reverse(dwellingsInTown))
		{
			if (!developmentInfo.town->hasBuilt(buildID))
				continue;

			const auto & info = getBuildingOrPrerequisite(developmentInfo.town, buildID);
			developmentInfo.addExistingDwelling(info);
			break;
		}

		// find all non-built dwellings that can be built and add them for consideration
		for (const auto & buildID : dwellingsInTown)
		{
			if (developmentInfo.town->hasBuilt(buildID))
				continue;

			const auto & info = getBuildingOrPrerequisite(developmentInfo.town, buildID);
			if (info.canBuild || info.notEnoughRes)
				developmentInfo.addBuildingToBuild(info);
		}
	}
}

void BuildAnalyzer::updateOtherBuildings(TownDevelopmentInfo & developmentInfo)
{
	logAi->trace("Checking other buildings");

	std::vector<std::vector<BuildingID>> otherBuildings = {
		{BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL},
		{BuildingID::MAGES_GUILD_3, BuildingID::MAGES_GUILD_5}
	};

	if(developmentInfo.existingDwellings.size() >= 2 && ai->cb->getDate(Date::DAY_OF_WEEK) > 4)
	{
		otherBuildings.push_back({BuildingID::HORDE_1});
		otherBuildings.push_back({BuildingID::HORDE_2});
	}

	otherBuildings.push_back({ BuildingID::CITADEL, BuildingID::CASTLE });
	otherBuildings.push_back({ BuildingID::RESOURCE_SILO });
	otherBuildings.push_back({ BuildingID::SPECIAL_1 });
	otherBuildings.push_back({ BuildingID::SPECIAL_2 });
	otherBuildings.push_back({ BuildingID::SPECIAL_3 });
	otherBuildings.push_back({ BuildingID::SPECIAL_4 });
	otherBuildings.push_back({ BuildingID::MARKETPLACE });

	for(auto & buildingSet : otherBuildings)
	{
		for(auto & buildingID : buildingSet)
		{
			if(!developmentInfo.town->hasBuilt(buildingID) && developmentInfo.town->getTown()->buildings.count(buildingID))
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

TResources withoutGold(TResources other)
{
	other[GameResID::GOLD] = 0;

	return other;
}

TResources BuildAnalyzer::getResourcesRequiredNow() const
{
	auto resourcesAvailable = ai->getFreeResources();
	auto result = withoutGold(armyCost) + requiredResources - resourcesAvailable;

	result.positive();

	return result;
}

TResources BuildAnalyzer::getTotalResourcesRequired() const
{
	auto resourcesAvailable = ai->getFreeResources();
	auto result = totalDevelopmentCost + withoutGold(armyCost) - resourcesAvailable;

	result.positive();

	return result;
}

bool BuildAnalyzer::isGoldPressureHigh() const
{
	return goldPressure > ai->settings->getMaxGoldPressure();
}

void BuildAnalyzer::update()
{
	logAi->trace("Start analysing build");

	BuildingInfo bi;

	reset();

	auto towns = ai->cb->getTownsInfo();

	float economyDevelopmentCost = 0;

	for(const CGTownInstance* town : towns)
	{
		if(town->built >= cb->getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP))
			continue; // Not much point in trying anything - can't built in this town anymore today
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Checking town %s", town->getNameTranslated());
#endif
		developmentInfos.push_back(TownDevelopmentInfo(town));
		TownDevelopmentInfo & developmentInfo = developmentInfos.back();

		updateTownDwellings(developmentInfo);
		updateOtherBuildings(developmentInfo);

		requiredResources += developmentInfo.requiredResources;
		totalDevelopmentCost += developmentInfo.townDevelopmentCost;
		for(auto building : developmentInfo.toBuild)
		{
			if (building.dailyIncome[EGameResID::GOLD] > 0)
				economyDevelopmentCost += building.buildCostWithPrerequisites[EGameResID::GOLD];
		}
		armyCost += developmentInfo.armyCost;

#if NKAI_TRACE_LEVEL >= 1
		for(auto bi : developmentInfo.toBuild)
			logAi->trace("Building preferences %s", bi.toString());
#endif
	}

	std::sort(developmentInfos.begin(), developmentInfos.end(), [](const TownDevelopmentInfo & t1, const TownDevelopmentInfo & t2) -> bool
	{
		auto val1 = convertToGold(t1.armyCost) - convertToGold(t1.townDevelopmentCost);
		auto val2 = convertToGold(t2.armyCost) - convertToGold(t2.townDevelopmentCost);

		return val1 > val2;
	});

	updateDailyIncome();

	goldPressure = (ai->getLockedResources()[EGameResID::GOLD] + (float)armyCost[EGameResID::GOLD] + economyDevelopmentCost) / (1 + 2 * ai->getFreeGold() + (float)dailyIncome[EGameResID::GOLD] * 7.0f);

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Gold pressure: %f", goldPressure);
#endif
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
	auto townInfo = town->getTown();

	const auto & buildPtr = townInfo->buildings.at(building);
	const CCreature * creature = nullptr;
	CreatureID baseCreatureID;

	int creatureLevel = -1;
	int creatureUpgrade = 0;

	if(toBuild.isDwelling())
	{
		creatureLevel = BuildingID::getLevelFromDwelling(toBuild);
		creatureUpgrade = BuildingID::getUpgradedFromDwelling(toBuild);
	}
	else if(toBuild == BuildingID::HORDE_1 || toBuild == BuildingID::HORDE_1_UPGR)
	{
		creatureLevel = townInfo->hordeLvl.at(0);
	}
	else if(toBuild == BuildingID::HORDE_2 || toBuild == BuildingID::HORDE_2_UPGR)
	{
		creatureLevel = townInfo->hordeLvl.at(1);
	}

	if(creatureLevel >=  0)
	{
		auto creatures = townInfo->creatures.at(creatureLevel);
		auto creatureID = creatures.size() > creatureUpgrade
			? creatures.at(creatureUpgrade)
			: creatures.front();

		baseCreatureID = creatures.front();
		creature = creatureID.toCreature();
	}

	auto info = BuildingInfo(buildPtr.get(), creature, baseCreatureID, town, ai);

	//logAi->trace("checking %s buildInfo %s", info.name, info.toString());

	int highestFort = 0;
	for (auto twn : ai->cb->getTownsInfo())
	{
		highestFort = std::max(highestFort, (int)twn->fortLevel());
	}

	if(!town->hasBuilt(building))
	{
		auto canBuild = ai->cb->canBuildStructure(town, building);

		if(canBuild == EBuildingState::ALLOWED)
		{
			info.canBuild = true;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			//logAi->trace("cant build. Not enough resources. Need %s", info.buildCost.toString());
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
				return id.isDwelling();
			};

			if(vstd::contains_if(missingBuildings, otherDwelling))
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("cant build %d. Need other dwelling %d", toBuild.getNum(), missingBuildings.front().getNum());
#endif
			}
			else if(missingBuildings[0] != toBuild)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("cant build %d. Need %d", toBuild.getNum(), missingBuildings[0].num);
#endif
				BuildingInfo prerequisite = getBuildingOrPrerequisite(town, missingBuildings[0], excludeDwellingDependencies);

				prerequisite.buildCostWithPrerequisites += info.buildCost;
				prerequisite.creatureCost = info.creatureCost;
				prerequisite.creatureGrows = info.creatureGrows;
				prerequisite.creatureLevel = info.creatureLevel;
				prerequisite.creatureID = info.creatureID;
				prerequisite.baseCreatureID = info.baseCreatureID;
				prerequisite.prerequisitesCount++;
				prerequisite.armyCost = info.armyCost;
				prerequisite.armyStrength = info.armyStrength;
				bool haveSameOrBetterFort = false;
				if (prerequisite.id == BuildingID::FORT && highestFort >= CGTownInstance::EFortLevel::FORT)
					haveSameOrBetterFort = true;
				if (prerequisite.id == BuildingID::CITADEL && highestFort >= CGTownInstance::EFortLevel::CITADEL)
					haveSameOrBetterFort = true;
				if (prerequisite.id == BuildingID::CASTLE && highestFort >= CGTownInstance::EFortLevel::CASTLE)
					haveSameOrBetterFort = true;
				if(!haveSameOrBetterFort)
					prerequisite.dailyIncome = info.dailyIncome;

				return prerequisite;
			}
			else
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Cant build. The building requires itself as prerequisite");
#endif
				return info;
			}
		}
		else
		{
#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Cant build. Reason: %d", static_cast<int>(canBuild));
#endif
		}
	}
	else
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Dwelling %d exists", toBuild.getNum());
#endif
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
			dailyIncome += mine->dailyIncome();
	}

	for(const CGTownInstance* town : towns)
	{
		dailyIncome += town->dailyIncome();
	}
}

bool BuildAnalyzer::hasAnyBuilding(FactionID alignment, BuildingID bid) const
{
	for(auto tdi : developmentInfos)
	{
		if(tdi.town->getFactionID() == alignment && tdi.town->hasBuilt(bid))
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
	townDevelopmentCost += nextToBuild.buildCostWithPrerequisites;
	townDevelopmentCost += withoutGold(nextToBuild.armyCost);

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
	buildCostWithPrerequisites = 0;
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
	buildCostWithPrerequisites = building->resources;
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
			if(id.isDwelling())
			{
				creatureGrows = creature->getGrowth();

				if(town->hasBuilt(BuildingID::CASTLE))
					creatureGrows *= 2;
				else if(town->hasBuilt(BuildingID::CITADEL))
					creatureGrows += creatureGrows / 2;
			}
			else
			{
				creatureGrows = creature->getHorde();
			}
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
