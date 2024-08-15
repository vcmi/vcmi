/*
 * CTownHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTown.h"

#include "CFaction.h"
#include "CTownHandler.h"
#include "../building/CBuilding.h"
#include "../../texts/TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

CTown::CTown()
	: faction(nullptr), mageLevel(0), primaryRes(0), moatAbility(SpellID::NONE), defaultTavernChance(0)
{
}

CTown::~CTown()
{
	for(auto & build : buildings)
		build.second.dellNull();

	for(auto & str : clientInfo.structures)
		str.dellNull();
}

std::string CTown::getRandomNameTextID(size_t index) const
{
	return TextIdentifier("faction", faction->modScope, faction->identifier, "randomName", index).get();
}

size_t CTown::getRandomNamesCount() const
{
	return namesCount;
}

std::string CTown::getBuildingScope() const
{
	if(faction == nullptr)
		//no faction == random faction
		return "building";
	else
		return "building." + faction->getJsonKey();
}

std::set<si32> CTown::getAllBuildings() const
{
	std::set<si32> res;

	for(const auto & b : buildings)
	{
		res.insert(b.first.num);
	}

	return res;
}

const CBuilding * CTown::getSpecialBuilding(BuildingSubID::EBuildingSubID subID) const
{
	for(const auto & kvp : buildings)
	{
		if(kvp.second->subId == subID)
			return buildings.at(kvp.first);
	}
	return nullptr;
}

BuildingID CTown::getBuildingType(BuildingSubID::EBuildingSubID subID) const
{
	const auto * building = getSpecialBuilding(subID);
	return building == nullptr ? BuildingID::NONE : building->bid.num;
}

std::string CTown::getGreeting(BuildingSubID::EBuildingSubID subID) const
{
	return vstd::find_or(specialMessages, subID, std::string());
}

void CTown::setGreeting(BuildingSubID::EBuildingSubID subID, const std::string & message) const
{
	specialMessages.insert(std::pair<BuildingSubID::EBuildingSubID, const std::string>(subID, message));
}

VCMI_LIB_NAMESPACE_END
