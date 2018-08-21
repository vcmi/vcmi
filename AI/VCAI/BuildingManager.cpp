/*
* BuildingManager.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "BuildingManager.h"

#include "../../CCallback.h"
#include "../../lib/mapObjects/MapObjects.h"

bool BuildingManager::tryBuildThisStructure(const CGTownInstance * t, BuildingID building, unsigned int maxDays)
{
	if (maxDays == 0)
	{
		logAi->warn("Request to build building %d in 0 days!", building.toEnum());
		return false;
	}

	if (!vstd::contains(t->town->buildings, building))
		return false; // no such building in town

	if (t->hasBuilt(building)) //Already built? Shouldn't happen in general
		return true;

	const CBuilding * buildPtr = t->town->buildings.at(building);

	auto toBuild = buildPtr->requirements.getFulfillmentCandidates([&](const BuildingID & buildID)
	{
		return t->hasBuilt(buildID);
	});
	toBuild.push_back(building);

	for (BuildingID buildID : toBuild)
	{
		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
		if (canBuild == EBuildingState::HAVE_CAPITAL || canBuild == EBuildingState::FORBIDDEN || canBuild == EBuildingState::NO_WATER)
			return false; //we won't be able to build this
	}

	if (maxDays && toBuild.size() > maxDays)
		return false;

	//TODO: calculate if we have enough resources to build it in maxDays?

	for (const auto & buildID : toBuild)
	{
		const CBuilding * b = t->town->buildings.at(buildID);

		EBuildingState::EBuildingState canBuild = cb->canBuildStructure(t, buildID);
		if (canBuild == EBuildingState::ALLOWED)
		{
			PotentialBuilding pb;
			pb.bid = buildID;
			pb.price = t->getBuildingCost(buildID);
			immediateBuildings.push_back(pb); //these are checked again in try
			return true;
		}
		else if (canBuild == EBuildingState::PREREQUIRES)
		{
			// can happen when dependencies have their own missing dependencies
			if (tryBuildThisStructure(t, buildID, maxDays - 1))
				return true;
		}
		else if (canBuild == EBuildingState::MISSING_BASE)
		{
			if (tryBuildThisStructure(t, b->upgrade, maxDays - 1))
				return true;
		}
		else if (canBuild == EBuildingState::NO_RESOURCES)
		{
			//we may need to gather resources for those
			PotentialBuilding pb;
			pb.bid = buildID;
			pb.price = t->getBuildingCost(buildID);
			expensiveBuildings.push_back(pb); //these are checked again in try
			return false;
		}
		else
			return false;
	}
	return false;
}

bool BuildingManager::tryBuildAnyStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays)
{
	for (const auto & building : buildList)
	{
		if (t->hasBuilt(building))
			continue;
		return tryBuildThisStructure(t, building, maxDays);

	}
	return false; //Can't build anything
}

boost::optional<BuildingID> BuildingManager::canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays) const
{
	for (const auto & building : buildList)
	{
		if (t->hasBuilt(building))
			continue;
		switch (cb->canBuildStructure(t, building))
		{
			case EBuildingState::ALLOWED:
			case EBuildingState::NO_RESOURCES: //TODO: allow this via optional parameter?
				return boost::optional<BuildingID>(building);
				break;
		}
	}
	return boost::optional<BuildingID>(); //Can't build anything
}

bool BuildingManager::tryBuildNextStructure(const CGTownInstance * t, std::vector<BuildingID> buildList, unsigned int maxDays)
{
	for (const auto & building : buildList)
	{
		if (t->hasBuilt(building))
			continue;
		return tryBuildThisStructure(t, building, maxDays);
	}
	return false; //Nothing to build
}

void BuildingManager::setCB(CPlayerSpecificInfoCallback * CB)
{
	cb = CB;
}

void BuildingManager::setAI(VCAI * AI)
{
	ai = AI;
}
//Set of buildings for different goals. Does not include any prerequisites.
static const BuildingID essential[] = { BuildingID::TAVERN, BuildingID::TOWN_HALL };
static const BuildingID goldSource[] = { BuildingID::TOWN_HALL, BuildingID::CITY_HALL, BuildingID::CAPITOL };
static const BuildingID capitolRequirements[] = { BuildingID::FORT, BuildingID::CITADEL };
static const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7 };
static const BuildingID unitsUpgrade[] = { BuildingID::DWELL_LVL_1_UP, BuildingID::DWELL_LVL_2_UP, BuildingID::DWELL_LVL_3_UP,
BuildingID::DWELL_LVL_4_UP, BuildingID::DWELL_LVL_5_UP, BuildingID::DWELL_LVL_6_UP, BuildingID::DWELL_LVL_7_UP };
static const BuildingID unitGrowth[] = { BuildingID::FORT, BuildingID::CITADEL, BuildingID::CASTLE, BuildingID::HORDE_1,
BuildingID::HORDE_1_UPGR, BuildingID::HORDE_2, BuildingID::HORDE_2_UPGR };
static const BuildingID _spells[] = { BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3,
BuildingID::MAGES_GUILD_4, BuildingID::MAGES_GUILD_5 };
static const BuildingID extra[] = { BuildingID::RESOURCE_SILO, BuildingID::SPECIAL_1, BuildingID::SPECIAL_2, BuildingID::SPECIAL_3,
BuildingID::SPECIAL_4, BuildingID::SHIPYARD }; // all remaining buildings

bool BuildingManager::getBuildingOptions(const CGTownInstance * t)
{
	//TODO make *real* town development system
	//TODO: faction-specific development: use special buildings, build dwellings in better order, etc
	//TODO: build resource silo, defences when needed
	//Possible - allow "locking" on specific building (build prerequisites and then building itself)

	immediateBuildings.clear();
	expensiveBuildings.clear();

	//below algorithm focuses on economy growth at start of the game.

	TResources currentRes = cb->getResourceAmount();
	TResources currentIncome = t->dailyIncome();

	if (tryBuildAnyStructure(t, std::vector<BuildingID>(essential, essential + ARRAY_COUNT(essential))))
		return true;

	//the more gold the better and less problems later
	if (tryBuildNextStructure(t, std::vector<BuildingID>(goldSource, goldSource + ARRAY_COUNT(goldSource))))
		return true;

	//workaround for mantis #2696 - build fort and citadel - building castle will be handled without bug
	if (vstd::contains(t->builtBuildings, BuildingID::CITY_HALL) &&
		cb->canBuildStructure(t, BuildingID::CAPITOL) != EBuildingState::HAVE_CAPITAL)
	{
		if (cb->canBuildStructure(t, BuildingID::CAPITOL) != EBuildingState::FORBIDDEN)
		{
			if (tryBuildNextStructure(t, std::vector<BuildingID>(capitolRequirements,
				capitolRequirements + ARRAY_COUNT(capitolRequirements))))
				return true;
		}
	}

	//TODO: save money for capitol or city hall if capitol unavailable
	//do not build other things (unless gold source buildings are disabled in map editor)


	if (cb->getDate(Date::DAY_OF_WEEK) > 6) // last 2 days of week - try to focus on growth
	{
		if (tryBuildNextStructure(t, std::vector<BuildingID>(unitGrowth, unitGrowth + ARRAY_COUNT(unitGrowth)), 2))
			return true;
	}

	// first in-game week or second half of any week: try build dwellings
	if (cb->getDate(Date::DAY) < 7 || cb->getDate(Date::DAY_OF_WEEK) > 3)
	{
		if (tryBuildAnyStructure(t, std::vector<BuildingID>(unitsSource,
			unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK)))
			return true;
	}

	//try to upgrade dwelling
	for (int i = 0; i < ARRAY_COUNT(unitsUpgrade); i++)
	{
		if (t->hasBuilt(unitsSource[i]) && !t->hasBuilt(unitsUpgrade[i]))
		{
			if (tryBuildThisStructure(t, unitsUpgrade[i]))
				return true;
		}
	}

	//remaining tasks
	if (tryBuildNextStructure(t, std::vector<BuildingID>(_spells, _spells + ARRAY_COUNT(_spells))))
		return true;
	if (tryBuildAnyStructure(t, std::vector<BuildingID>(extra, extra + ARRAY_COUNT(extra))))
		return true;

	//at the end, try to get and build any extra buildings with nonstandard slots (for example HotA 3rd level dwelling)
	std::vector<BuildingID> extraBuildings;
	for (auto buildingInfo : t->town->buildings)
	{
		if (buildingInfo.first > 43)
			extraBuildings.push_back(buildingInfo.first);
	}
	if (tryBuildAnyStructure(t, extraBuildings))
		return true;

	return false;
}

boost::optional<PotentialBuilding> BuildingManager::immediateBuilding() const
{
	if (immediateBuildings.size())
		return boost::optional<PotentialBuilding>(immediateBuildings.front()); //back? whatever
	else
		return boost::optional<PotentialBuilding>();
}

boost::optional<PotentialBuilding> BuildingManager::expensiveBuilding() const
{
	if (expensiveBuildings.size())
		return boost::optional<PotentialBuilding>(expensiveBuildings.front());
	else
		return boost::optional<PotentialBuilding>();
}
