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
#include <boost/range/algorithm/sort.hpp>

#include "../Engine/Nullkiller.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/IGameSettings.h"
#include "../AIUtility.h"

namespace NK2AI
{

TResources BuildAnalyzer::getMissingResourcesNow(const float armyGoldRatio) const
{
	TResources result = armyCost;
	result[GameResID::GOLD] *= armyGoldRatio;
	result += requiredResources;
	result -= aiNk->getFreeResources();
	result.positive();
	return result;
}

TResources BuildAnalyzer::getMissingResourcesInTotal(const float armyGoldRatio) const
{
	TResources result = armyCost;
	result[GameResID::GOLD] *= armyGoldRatio;
	result += totalDevelopmentCost;
	result -= aiNk->getFreeResources();
	result.positive();
	return result;
}

TResources BuildAnalyzer::getFreeResourcesAfterMissingTotal(const float armyGoldRatio) const
{
	TResources result = -armyCost;
	result[GameResID::GOLD] *= armyGoldRatio;
	result -= totalDevelopmentCost;
	result += aiNk->getFreeResources();
	result.positive();
	return result;
}

bool BuildAnalyzer::isGoldPressureOverMax() const
{
	return goldPressure > aiNk->settings->getMaxGoldPressure();
}

void BuildAnalyzer::update()
{
	logAi->trace("Start BuildAnalyzer::update");
	reset();
	const auto towns = aiNk->cc->getTownsInfo();
	float economyDevelopmentCost = 0;

	for(const CGTownInstance * town : towns)
	{
		if(town->built >= aiNk->cc->getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP))
			continue; // Not much point in trying anything - can't built in this town anymore today

#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace("Checking town %s", town->getNameTranslated());
#endif

		developmentInfos.push_back(TownDevelopmentInfo(town));
		TownDevelopmentInfo & tdi = developmentInfos.back();

		updateDwellings(tdi, aiNk->armyManager, aiNk->cc);
		updateOtherBuildings(tdi, aiNk->armyManager, aiNk->cc);
		requiredResources += tdi.requiredResources;
		totalDevelopmentCost += tdi.townDevelopmentCost;

		for(auto building : tdi.toBuild)
		{
			if (building.dailyIncome[EGameResID::GOLD] > 0)
				economyDevelopmentCost += building.buildCostWithPrerequisites[EGameResID::GOLD];
		}

		armyCost += tdi.armyCost;

#if NK2AI_TRACE_LEVEL >= 1
		for(const auto & biToBuild : tdi.toBuild)
			logAi->trace("Building preferences %s", biToBuild.toString());
#endif
	}

	boost::range::sort(developmentInfos, [](const TownDevelopmentInfo & tdi1, const TownDevelopmentInfo & tdi2) -> bool
	{
		auto val1 = goldApproximate(tdi1.armyCost) - goldApproximate(tdi1.townDevelopmentCost);
		auto val2 = goldApproximate(tdi2.armyCost) - goldApproximate(tdi2.townDevelopmentCost);
		return val1 > val2;
	});

	dailyIncome = calculateDailyIncome(aiNk->cc->getMyObjects(), aiNk->cc->getTownsInfo());
	goldPressure = calculateGoldPressure(aiNk->getLockedResources()[EGameResID::GOLD],
	                                     static_cast<float>(armyCost[EGameResID::GOLD]),
	                                     economyDevelopmentCost,
	                                     aiNk->getFreeGold(),
	                                     static_cast<float>(dailyIncome[EGameResID::GOLD]));
}

void BuildAnalyzer::reset()
{
	requiredResources = TResources();
	totalDevelopmentCost = TResources();
	armyCost = TResources();
	developmentInfos.clear();
}

bool BuildAnalyzer::isBuilt(FactionID alignment, BuildingID bid) const
{
	for(const auto & tdi : developmentInfos)
	{
		if(tdi.town->getFactionID() == alignment && tdi.town->hasBuilt(bid))
			return true;
	}

	return false;
}

void TownDevelopmentInfo::addBuildingBuilt(const BuildingInfo & bi)
{
	armyCost += bi.armyCost;
	armyStrength += bi.armyStrength;
	built.push_back(bi);
}

void TownDevelopmentInfo::addBuildingToBuild(const BuildingInfo & bi)
{
	townDevelopmentCost += bi.buildCostWithPrerequisites;
	townDevelopmentCost += BuildAnalyzer::goldRemove(bi.armyCost);

	if (bi.isBuildable)
	{
		toBuild.push_back(bi);
	}
	else if (bi.isMissingResources)
	{
		requiredResources += bi.buildCost;
		toBuild.push_back(bi);
	}
}

BuildingInfo::BuildingInfo() {}

BuildingInfo::BuildingInfo(
	const CBuilding * building,
	const CCreature * creature,
	CreatureID baseCreature,
	const CGTownInstance * town,
	const std::unique_ptr<ArmyManager> & armyManager)
{
	id = building->bid;
	buildCost = building->resources;
	buildCostWithPrerequisites = building->resources;
	dailyIncome = building->produce;
	isBuilt = town->hasBuilt(id);
	prerequisitesCount = 1;
	name = building->getNameTranslated();

	if(creature)
	{
		creatureGrowth = creature->getGrowth();
		creatureID = creature->getId();
		baseCreatureID = baseCreature;
		creatureUnitCost = creature->getFullRecruitCost();
		creatureLevel = creature->getLevel();

		if(isBuilt)
		{
			creatureGrowth = town->creatureGrowth(creatureLevel - 1);
		}
		else
		{
			if(id.isDwelling())
			{
				creatureGrowth = creature->getGrowth();

				if(town->hasBuilt(BuildingID::CASTLE))
					creatureGrowth *= 2;
				else if(town->hasBuilt(BuildingID::CITADEL))
					creatureGrowth += creatureGrowth / 2;
			}
			else
			{
				creatureGrowth = creature->getHorde();
			}
		}

		armyStrength = armyManager->evaluateStackPower(creature, creatureGrowth);
		armyCost = creatureUnitCost * creatureGrowth;
	}
}

std::string BuildingInfo::toString() const
{
	return name + ", cost: " + buildCost.toString()
		+ ", creature: " + std::to_string(creatureGrowth) + " x " + std::to_string(creatureLevel)
		+ " x " + creatureUnitCost.toString()
		+ ", daily: " + dailyIncome.toString();
}

float BuildAnalyzer::calculateGoldPressure(TResource lockedGold, float armyCostGold, float economyDevelopmentCost, float freeGold, float dailyIncomeGold)
{
	auto pressure = (lockedGold + armyCostGold + economyDevelopmentCost) / (1 + 2 * freeGold + dailyIncomeGold * 7.0f);

#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("Gold pressure: %f", pressure);
#endif

	return pressure;
}

TResources BuildAnalyzer::calculateDailyIncome(const std::vector<const CGObjectInstance *> & objects, const std::vector<const CGTownInstance *> & townInfos)
{
	auto result = TResources();

	for(const CGObjectInstance * obj : objects)
	{
		if(const auto * mine = dynamic_cast<const CGMine *>(obj))
			result += mine->dailyIncome();
	}

	for(const CGTownInstance * town : townInfos)
	{
		result += town->dailyIncome();
	}

	return result;
}

void BuildAnalyzer::updateDwellings(TownDevelopmentInfo & developmentInfo, std::unique_ptr<ArmyManager> & armyManager, std::shared_ptr<CCallback> & cc)
{
	for(int level = 0; level < developmentInfo.town->getTown()->creatures.size(); level++)
	{
#if NK2AI_TRACE_LEVEL >= 1
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

			const auto & info = getBuildingOrPrerequisite(developmentInfo.town, buildID, armyManager, cc);
			developmentInfo.addBuildingBuilt(info);
			break;
		}

		// find all non-built dwellings that can be built and add them for consideration
		for (const auto & buildID : dwellingsInTown)
		{
			if (developmentInfo.town->hasBuilt(buildID))
				continue;

			const auto & info = getBuildingOrPrerequisite(developmentInfo.town, buildID, armyManager, cc);
			if (info.isBuildable || info.isMissingResources)
				developmentInfo.addBuildingToBuild(info);
		}
	}
}

void BuildAnalyzer::updateOtherBuildings(TownDevelopmentInfo & developmentInfo,
                                         std::unique_ptr<ArmyManager> & armyManager,
                                         std::shared_ptr<CCallback> & cc)
{
	logAi->trace("Checking other buildings");

	std::vector<std::vector<BuildingID>> otherBuildings = {
		{BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL},
		{BuildingID::MAGES_GUILD_3, BuildingID::MAGES_GUILD_5}
	};

	if(developmentInfo.built.size() >= 2 && cc->getDate(Date::DAY_OF_WEEK) > 4)
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
				developmentInfo.addBuildingToBuild(getBuildingOrPrerequisite(developmentInfo.town, buildingID, armyManager, cc));
				break;
			}
		}
	}
}

BuildingInfo BuildAnalyzer::getBuildingOrPrerequisite(
	const CGTownInstance * town,
	BuildingID b,
	std::unique_ptr<ArmyManager> & armyManager,
	std::shared_ptr<CCallback> & cc,
	bool excludeDwellingDependencies)
{
	// TODO: Mircea: Remove redundant variable
	BuildingID building = b;
	const auto * townInfo = town->getTown();
	const auto & buildPtr = townInfo->buildings.at(building);
	const CCreature * creature = nullptr;
	CreatureID baseCreatureID;

	int creatureLevelIndex = -1;
	int creatureUpgradeNo = 0;

	if(b.isDwelling())
	{
		creatureLevelIndex = BuildingID::getLevelIndexFromDwelling(b);
		creatureUpgradeNo = BuildingID::getUpgradeNoFromDwelling(b);
	}
	else if(b == BuildingID::HORDE_1 || b == BuildingID::HORDE_1_UPGR)
	{
		creatureLevelIndex = townInfo->hordeLvl.at(0);
	}
	else if(b == BuildingID::HORDE_2 || b == BuildingID::HORDE_2_UPGR)
	{
		creatureLevelIndex = townInfo->hordeLvl.at(1);
	}

	if(creatureLevelIndex >= 0)
	{
		auto creatures = townInfo->creatures.at(creatureLevelIndex);
		auto creatureID = creatures.size() > creatureUpgradeNo
			? creatures.at(creatureUpgradeNo)
			: creatures.front();

		baseCreatureID = creatures.front();
		creature = creatureID.toCreature();
	}

	auto info = BuildingInfo(buildPtr.get(), creature, baseCreatureID, town, armyManager);

	//logAi->trace("checking %s buildInfo %s", info.name, info.toString());

	int highestFort = 0;
	for (const auto * ti : cc->getTownsInfo())
	{
		highestFort = std::max(highestFort, static_cast<int>(ti->fortLevel()));
	}

	if(!town->hasBuilt(building))
	{
		auto canBuild = cc->canBuildStructure(town, building);

		if(canBuild == EBuildingState::ALLOWED)
		{
			info.isBuildable = true;
		}
		else if(canBuild == EBuildingState::NO_RESOURCES)
		{
			//logAi->trace("cant build. Not enough resources. Need %s", info.buildCost.toString());
			info.isMissingResources = true;
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
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Can't build %d. Needs other dwelling %d", b.getNum(), missingBuildings.front().getNum());
#endif
			}
			else if(missingBuildings[0] != b)
			{
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Can't build %d. Needs %d", b.getNum(), missingBuildings[0].num);
#endif
				BuildingInfo prerequisite = getBuildingOrPrerequisite(town, missingBuildings[0], armyManager, cc, excludeDwellingDependencies);

				prerequisite.buildCostWithPrerequisites += info.buildCost;
				prerequisite.creatureUnitCost = info.creatureUnitCost;
				prerequisite.creatureGrowth = info.creatureGrowth;
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
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Can't build. The building requires itself as prerequisite");
#endif
				return info;
			}
		}
		else
		{
#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace("Can't build. Reason: %d", static_cast<int>(canBuild));
#endif
		}
	}
	else
	{
#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace("Dwelling %d exists", b.getNum());
#endif
		info.isBuilt = true;
	}

	return info;
}

TResource BuildAnalyzer::goldApproximate(const TResources & res)
{
	// TODO: Mircea: Would it make sense to use the marketplace rate of the player? See ResourceTrader::trade()
	return goldApproximate(res[EGameResID::WOOD], EGameResID::WOOD) + goldApproximate(res[EGameResID::MERCURY], EGameResID::MERCURY) +
		   goldApproximate(res[EGameResID::ORE], EGameResID::ORE) + goldApproximate(res[EGameResID::SULFUR], EGameResID::SULFUR) +
		   goldApproximate(res[EGameResID::CRYSTAL], EGameResID::CRYSTAL) + goldApproximate(res[EGameResID::GEMS], EGameResID::GEMS) +
		   goldApproximate(res[EGameResID::GOLD], EGameResID::GOLD);
}

TResource BuildAnalyzer::goldApproximate(const TResource & res, const EGameResID resId)
{
	// TODO: Mircea: replace with LIBRARY->objh->resVals[resId]
	switch(resId)
	{
		case EGameResID::WOOD:
		case EGameResID::ORE:
			return res * 75;
		case EGameResID::MERCURY:
		case EGameResID::SULFUR:
		case EGameResID::CRYSTAL:
		case EGameResID::GEMS:
			return res * 150;
		case EGameResID::GOLD:
			return res;
		default:
			throw std::runtime_error("Unsupported resource ID" + std::to_string(resId));
	}
}

TResources BuildAnalyzer::goldRemove(TResources other)
{
	TResources copy;
	for(int i = 0; i < GameResID::COUNT; ++i)
		copy[i] = other[i];

	copy[GameResID::GOLD] = 0;
	return copy;
}

TResources BuildAnalyzer::goldOnly(TResources other)
{
	TResources copy;
	copy[GameResID::GOLD] = other[GameResID::GOLD];
	return copy;
}

}
