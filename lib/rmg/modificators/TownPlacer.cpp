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
	POSTFUNCTION(MinePlacer);
	POSTFUNCTION(RoadPlacer);
} 

void TownPlacer::placeTowns(ObjectManager & manager)
{
	if(zone.getOwner() && ((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START)))
	{
		//set zone types to player faction, generate main town
		logGlobal->info("Preparing playing zone");
		int player_id = *zone.getOwner() - 1;
		auto& playerInfo = map.getPlayer(player_id);
		PlayerColor player(player_id);
		if(playerInfo.canAnyonePlay())
		{
			player = PlayerColor(player_id);
			zone.setTownType(map.getMapGenOptions().getPlayersSettings().find(player)->second.getStartingTown());
			
			if(zone.getTownType() == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				zone.setTownType(getRandomTownType(true));
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			zone.setTownType(getRandomTownType());
		}
		
		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, zone.getTownType());

		CGTownInstance * town = dynamic_cast<CGTownInstance *>(townFactory->create());
		town->tempOwner = player;
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		
		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		
		auto position = placeMainTown(manager, *town);
		
		totalTowns++;
		//register MAIN town of zone only
		map.registerZone(town->getFaction());
		
		if(playerInfo.canAnyonePlay()) //configure info for owning player
		{
			logGlobal->trace("Fill player info %d", player_id);
			
			// Update player info
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
	else //randomize town types for any other zones as well
	{
		zone.setTownType(getRandomTownType());
	}
	
	addNewTowns(zone.getNeutralTowns().getCastleCount(), true, PlayerColor::NEUTRAL, manager);
	addNewTowns(zone.getNeutralTowns().getTownCount(), false, PlayerColor::NEUTRAL, manager);
	
	if(!totalTowns) //if there's no town present, get random faction for dwellings and pandoras
	{
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

int3 TownPlacer::placeMainTown(ObjectManager & manager, CGTownInstance & town)
{
	//towns are big objects and should be centered around visitable position
	rmg::Object rmgObject(town);
	rmgObject.setTemplate(zone.getTerrainType());

	int3 position(-1, -1, -1);
	{
		Zone::Lock lock(zone.areaMutex);
		position = manager.findPlaceForObject(zone.areaPossible(), rmgObject, [this](const int3& t)
			{
				float distance = zone.getPos().dist2dSQ(t);
				return 100000.f - distance; //some big number
			}, ObjectManager::OptimizeType::WEIGHT);
	}
	rmgObject.setPosition(position + int3(2, 2, 0)); //place visitable tile in the exact center of a zone
	manager.placeObject(rmgObject, false, true);
	cleanupBoundaries(rmgObject);
	zone.setPos(rmgObject.getVisitablePosition()); //roads lead to main town
	return position;
}

void TownPlacer::cleanupBoundaries(const rmg::Object & rmgObject)
{
	Zone::Lock lock(zone.areaMutex);
	for(const auto & t : rmgObject.getArea().getBorderOutside())
	{
		if(map.isOnMap(t))
		{
			map.setOccupied(t, ETileType::FREE);
			zone.areaPossible().erase(t);
			zone.freePaths().add(t);
		}
	}
}

void TownPlacer::addNewTowns(int count, bool hasFort, const PlayerColor & player, ObjectManager & manager)
{
	for(int i = 0; i < count; i++)
	{
		si32 subType = zone.getTownType();
		
		if(totalTowns>0)
		{
			if(!zone.areTownsSameType())
			{
				if(!zone.getTownTypes().empty())
					subType = *RandomGeneratorUtil::nextItem(zone.getTownTypes(), zone.getRand());
				else
					subType = *RandomGeneratorUtil::nextItem(zone.getDefaultTownTypes(), zone.getRand()); //it is possible to have zone with no towns allowed
			}
		}
		
		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, subType);
		auto * town = dynamic_cast<CGTownInstance *>(townFactory->create());
		town->ID = Obj::TOWN;
		
		town->tempOwner = player;
		if (hasFort)
			town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		
		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		
		if(totalTowns <= 0)
		{
			//FIXME: discovered bug with small zones - getPos is close to map boarder and we have outOfMap exception
			//register MAIN town of zone
			map.registerZone(town->getFaction());
			//first town in zone goes in the middle
			placeMainTown(manager, *town);
		}
		else
			manager.addRequiredObject(town);
		totalTowns++;
	}
}

si32 TownPlacer::getRandomTownType(bool matchUndergroundType)
{
	auto townTypesAllowed = (!zone.getTownTypes().empty() ? zone.getTownTypes() : zone.getDefaultTownTypes());
	if(matchUndergroundType)
	{
		std::set<FactionID> townTypesVerify;
		for(auto factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*VLC->townh)[factionIdx]->preferUndergroundPlacement;
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
