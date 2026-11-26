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
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../../spells/CSpellHandler.h"
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "MinePlacer.h"
#include "ObjectPlacer.h"
#include "WaterAdopter.h"
#include "../TileInfo.h"

#include <vstd/RNG.h>

void TownPlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
	{
		logRmg->error("ObjectManager doesn't exist for zone %d, skip modificator %s", zone.getId(), getName());
		return;
	}
	
	placeTowns(*manager);
}

void TownPlacer::init()
{
	POSTFUNCTION(MinePlacer);
	POSTFUNCTION(ObjectPlacer);
	POSTFUNCTION(RoadPlacer);
} 

void TownPlacer::placeTowns(ObjectManager & manager)
{
	if(zone.getOwner() && ((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START)))
	{
		logRmg->info("Preparing playing zone");
		int player_id = *zone.getOwner() - 1;
		const auto & playerSettings = map.getMapGenOptions().getPlayersSettings();
		PlayerColor player;

		if (playerSettings.size() > player_id)
		{
			const auto & currentPlayerSettings = std::next(playerSettings.begin(), player_id);
			player = currentPlayerSettings->first;
		}
		else
		{
			player = PlayerColor::NEUTRAL;
		}
		
		auto & townFactory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, zone.getTownType());

		auto town = std::dynamic_pointer_cast<CGTownInstance>(townFactory.create(map.mapInstance->cb, nullptr));
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
			logRmg->trace("Fill player info %d", player_id);
			
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
	
	addNewTowns(zone.getNeutralTowns().getCastleCount(), true, PlayerColor::NEUTRAL, manager);
	addNewTowns(zone.getNeutralTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
}

int3 TownPlacer::placeMainTown(ObjectManager & manager, std::shared_ptr<CGTownInstance> town)
{
	//towns are big objects and should be centered around visitable position
	rmg::Object rmgObject(town);
	rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());
	const int3 townPositionOffset = rmgObject.getPosition() - rmgObject.getVisitablePosition();

	auto findTownPosition = [this, &manager, &rmgObject, townPositionOffset](bool requireExitInsideZone)
	{
		Zone::Lock lock(zone.areaMutex);
		return manager.findPlaceForObject(zone.areaPossible().get(), rmgObject,
			[this, &rmgObject, townPositionOffset, requireExitInsideZone](const int3 & t)
			{
				if(requireExitInsideZone && !hasTownExitInsideZone(rmgObject, townPositionOffset))
					return -1.f;

				float distance = zone.getPos().dist2dSQ(t);
				return 100000.f - distance; //some big number
			}, ObjectManager::OptimizeType::WEIGHT);
	};

	int3 position = findTownPosition(true);
	if(!position.isValid())
	{
		logRmg->warn("Failed to place town with local exit in zone %d, reserving boundary exit", zone.getId());
		position = findTownPosition(false);
	}

	if(!position.isValid())
		throw rmgException(boost::str(boost::format("Failed to place town with free exit in zone %d") % zone.getId()));

	rmgObject.setPosition(position + townPositionOffset); //place visitable tile in the exact center of a zone
	manager.placeObject(rmgObject, false, true, true);
	cleanupBoundaries(rmgObject);
	zone.setPos(rmgObject.getVisitablePosition()); //roads lead to main town
	return position;
}

bool TownPlacer::hasTownExitInsideZone(const rmg::Object & rmgObject, const int3 & offset) const
{
	for(const auto & t : rmgObject.getArea().getBorderOutside())
	{
		if(t.y <= rmgObject.getVisitablePosition().y)
			continue;

		const auto exitTile = t + offset;
		if(map.isOnMap(exitTile) && map.getZoneID(exitTile) == zone.getId())
			return true;
	}

	return false;
}

void TownPlacer::cleanupBoundaries(const rmg::Object & rmgObject)
{
	const bool hasLocalExit = hasTownExitInsideZone(rmgObject);
	std::map<TRmgTemplateZoneId, rmg::Tileset> neighboringExitTiles;

	{
		Zone::Lock lock(zone.areaMutex);
		for(const auto & t : rmgObject.getArea().getBorderOutside())
		{
			if(t.y <= rmgObject.getVisitablePosition().y || !map.isOnMap(t))
				continue;

			const auto exitZoneId = map.getZoneID(t);
			const bool localExit = exitZoneId == zone.getId();
			if(!localExit && hasLocalExit)
				continue;

			map.setOccupied(t, ETileType::FREE);
			zone.areaPossible()->erase(t);
			zone.freePaths()->add(t);

			if(!localExit)
				neighboringExitTiles[exitZoneId].insert(t);
		}
	}

	auto & zones = map.getZones();
	for(const auto & [zoneId, tiles] : neighboringExitTiles)
	{
		auto zoneIter = zones.find(zoneId);
		if(zoneIter == zones.end())
			continue;

		auto zoneToUpdate = zoneIter->second;
		Zone::Lock lock(zoneToUpdate->areaMutex);
		for(const auto & t : tiles)
		{
			zoneToUpdate->areaPossible()->erase(t);
			zoneToUpdate->areaUsed()->add(t);
		}
	}
}

void TownPlacer::addNewTowns(int count, bool hasFort, const PlayerColor & player, ObjectManager & manager)
{
	for(int i = 0; i < count; i++)
	{
		FactionID subType = zone.getTownType();

		if(totalTowns > 0 && !zone.areTownsSameType())
		{
			const auto & hinted = zone.getHintedTownTypes();
			if(static_cast<size_t>(totalTowns) < hinted.size())
				subType = hinted[totalTowns];
			else
				subType = townFactionOrRandom(FactionID::NONE, zone.getTownTypes(), zone.getRand());
		}
		
		auto & townFactory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, subType);
		auto town = std::dynamic_pointer_cast<CGTownInstance>(townFactory.create(map.mapInstance->cb, nullptr));
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

int TownPlacer::getTotalTowns() const
{
	return totalTowns;
}
