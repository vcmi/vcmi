/*
 * TownPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TownPlacer.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../entities/faction/CTownHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../../spells/CSpellHandler.h" //for choosing random spells
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "MinePlacer.h"
#include "WaterAdopter.h"
#include "../TileInfo.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void TownPlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
	{
		logGlobal->error("ObjectManager doesn't exist for zone %d, skip modificator %s", zone.getId(), getName());
		return;
	}
	
	placeTowns(*manager);
}

void TownPlacer::init()
{
	for(auto & townHint : zone.getTownHints())
	{
		logGlobal->info("Town hint of zone %d: %d", zone.getId(), townHint.likeZone);
		if(townHint.likeZone != rmg::ZoneOptions::NO_ZONE)
		{
			logGlobal->info("Dependency on town type of zone %d", townHint.likeZone);
			dependency(map.getZones().at(townHint.likeZone)->getModificator<TownPlacer>());
		}
		else if(!townHint.notLikeZone.empty())
		{
			for(auto zoneId : townHint.notLikeZone)
			{
				logGlobal->info("Dependency on town unlike type of zone %d", zoneId);
				dependency(map.getZones().at(zoneId)->getModificator<TownPlacer>());
			}
		}
		else if(townHint.relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE)
		{
			logGlobal->info("Dependency on town related to zone terrain of zone %d", townHint.relatedToZoneTerrain);
			dependency(map.getZones().at(townHint.relatedToZoneTerrain)->getModificator<TownPlacer>());
		}
	}
	POSTFUNCTION(MinePlacer);
	POSTFUNCTION(RoadPlacer);
} 

void TownPlacer::placeTowns(ObjectManager & manager)
{
	// TODO: Configurew each subseqquent town based on townHints

	if(zone.getOwner() && ((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START)))
	{
		//set zone types to player faction, generate main town
		logGlobal->info("Preparing playing zone");
		int player_id = *zone.getOwner() - 1;
		const auto & playerSettings = map.getMapGenOptions().getPlayersSettings();
		PlayerColor player;

		if (playerSettings.size() > player_id)
		{
			const auto & currentPlayerSettings = std::next(playerSettings.begin(), player_id);
			player = currentPlayerSettings->first;
			zone.setTownType(currentPlayerSettings->second.getStartingTown());
			
			if(zone.getTownType() == FactionID::RANDOM)
				zone.setTownType(getRandomTownType(true));
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			zone.setTownType(getTownTypeFromHint(0));
		}
		
		auto townFactory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, zone.getTownType());

		auto town = std::dynamic_pointer_cast<CGTownInstance>(townFactory->create(map.mapInstance->cb, nullptr));
		town->tempOwner = player;
		town->addBuilding(BuildingID::FORT);
		town->addBuilding(BuildingID::DEFAULT);
		

		for(auto spellID : LIBRARY->spellh->getDefaultAllowed()) //add all regular spells to town
			town->possibleSpells.push_back(spellID);
		
		auto position = placeMainTown(manager, town);
		
		totalTowns++;
		//register MAIN town of zone only
		map.registerZone(town->getFactionID());
		
		if(player.isValidPlayer()) //configure info for owning player
		{
			logGlobal->trace("Fill player info %d", player_id);
			
			// Update player info
			auto & playerInfo = map.getPlayer(player.getNum());
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(zone.getTownType());
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = position;
			playerInfo.generateHeroAtMainTown = true;
			
			//now create actual towns
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, player, manager);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, player, manager);
		}
		else
		{
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, PlayerColor::NEUTRAL, manager);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
		}
	}
	else //randomize town types for non-player zones
	{
		zone.setTownType(getTownTypeFromHint(0));
	}
	
	addNewTowns(zone.getNeutralTowns().getCastleCount(), true, PlayerColor::NEUTRAL, manager);
	addNewTowns(zone.getNeutralTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
	
	if(!totalTowns) //if there's no town present, get random faction for dwellings and pandoras
	{
		// TODO: Use townHints also when there are no towns in zone
		//25% chance for neutral
		if (zone.getRand().nextInt(1, 100) <= 25)
		{
			zone.setTownType(ETownType::NEUTRAL);
		}
		else
		{
			if(!zone.getTownTypes().empty())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getTownTypes(), zone.getRand()));
			else if(!zone.getMonsterTypes().empty())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getMonsterTypes(), zone.getRand())); //this happens in Clash of Dragons in treasure zones, where all towns are banned
			else //just in any case
				zone.setTownType(getRandomTownType());
		}
	}
}

int3 TownPlacer::placeMainTown(ObjectManager & manager, std::shared_ptr<CGTownInstance> town)
{
	//towns are big objects and should be centered around visitable position
	rmg::Object rmgObject(town);
	rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());

	int3 position(-1, -1, -1);
	{
		Zone::Lock lock(zone.areaMutex);
		position = manager.findPlaceForObject(zone.areaPossible().get(), rmgObject, [this](const int3& t)
			{
				float distance = zone.getPos().dist2dSQ(t);
				return 100000.f - distance; //some big number
			}, ObjectManager::OptimizeType::WEIGHT);
	}
	rmgObject.setPosition(position + int3(2, 2, 0)); //place visitable tile in the exact center of a zone
	manager.placeObject(rmgObject, false, true, true);
	cleanupBoundaries(rmgObject);
	zone.setPos(rmgObject.getVisitablePosition()); //roads lead to main town
	return position;
}

void TownPlacer::cleanupBoundaries(const rmg::Object & rmgObject)
{
	Zone::Lock lock(zone.areaMutex);
	for(const auto & t : rmgObject.getArea().getBorderOutside())
	{
		if (t.y > rmgObject.getVisitablePosition().y) //Line below the town
		{
			if (map.isOnMap(t))
			{
				map.setOccupied(t, ETileType::FREE);
				zone.areaPossible()->erase(t);
				zone.freePaths()->add(t);
			}
		}
	}
}

FactionID TownPlacer::getTownTypeFromHint(size_t hintIndex)
{
	const auto & hints = zone.getTownHints();
	if(hints.size() <= hintIndex)
	{
		return *RandomGeneratorUtil::nextItem(zone.getTownTypes(), zone.getRand());
	}

	const auto & townHints = hints[hintIndex];
	FactionID subType = zone.getTownType();

	if(townHints.likeZone != rmg::ZoneOptions::NO_ZONE)
	{
		// Copy directly from other zone
		subType = map.getZones().at(townHints.likeZone)->getTownType();
	}
	else if(!townHints.notLikeZone.empty())
	{
		// Exclude type rolled for other zone
		auto townTypes = zone.getTownTypes();
		for(auto zoneId : townHints.notLikeZone)
		{
			townTypes.erase(map.getZones().at(zoneId)->getTownType());
		}
		zone.setTownTypes(townTypes);
		
		if(!townTypes.empty())
			subType = *RandomGeneratorUtil::nextItem(townTypes, zone.getRand());
	}
	else if(townHints.relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE)
	{
		auto townTerrain = map.getZones().at(townHints.relatedToZoneTerrain)->getTerrainType();
		
		auto townTypesAllowed = zone.getTownTypes();
		vstd::erase_if(townTypesAllowed, [townTerrain](FactionID type)
		{
			return (*LIBRARY->townh)[type]->getNativeTerrain() != townTerrain;
		});
		zone.setTownTypes(townTypesAllowed);
		
		if(!townTypesAllowed.empty())
			subType = *RandomGeneratorUtil::nextItem(townTypesAllowed, zone.getRand());
	}

	return subType;
}

void TownPlacer::addNewTowns(int count, bool hasFort, const PlayerColor & player, ObjectManager & manager)
{
	for(int i = 0; i < count; i++)
	{
		FactionID subType = zone.getTownType();
		
		if(totalTowns > 0)
		{
			if(!zone.areTownsSameType())
			{
				subType = getTownTypeFromHint(totalTowns);
			}
		}
		
		auto townFactory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, subType);
		auto town = std::dynamic_pointer_cast<CGTownInstance>(townFactory->create(map.mapInstance->cb, nullptr));
		town->ID = Obj::TOWN;
		
		town->tempOwner = player;
		if (hasFort)
			town->addBuilding(BuildingID::FORT);
		town->addBuilding(BuildingID::DEFAULT);
		
		for(auto spellID : LIBRARY->spellh->getDefaultAllowed()) //add all regular spells to town
			town->possibleSpells.push_back(spellID);
		
		if(totalTowns <= 0)
		{
			//FIXME: discovered bug with small zones - getPos is close to map boarder and we have outOfMap exception
			//register MAIN town of zone
			map.registerZone(town->getFactionID());
			//first town in zone goes in the middle
			placeMainTown(manager, town);
		}
		else
		{
			manager.addRequiredObject(RequiredObjectInfo(town, 0, true));
		}
		totalTowns++;
	}
}

FactionID TownPlacer::getRandomTownType(bool matchUndergroundType)
{
	auto townTypesAllowed = (!zone.getTownTypes().empty() ? zone.getTownTypes() : zone.getDefaultTownTypes());
	if(matchUndergroundType)
	{
		std::set<FactionID> townTypesVerify;
		for(auto factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*LIBRARY->townh)[factionIdx]->preferUndergroundPlacement;
			if(zone.isUnderground() ? preferUnderground : !preferUnderground)
			{
				townTypesVerify.insert(factionIdx);
			}
		}
		if(!townTypesVerify.empty())
			townTypesAllowed = townTypesVerify;
	}
	
	return *RandomGeneratorUtil::nextItem(townTypesAllowed, zone.getRand());
}

int TownPlacer::getTotalTowns() const
{
	return totalTowns;
}

VCMI_LIB_NAMESPACE_END
