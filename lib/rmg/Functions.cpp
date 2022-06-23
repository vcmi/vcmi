//
//  Functions.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "Functions.h"
#include "CMapGenerator.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "../CTownHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../VCMI_Lib.h"
#include "../spells/CSpellHandler.h" //for choosing random spells

std::set<int3> collectDistantTiles(const Zone& zone, float distance)
{
	std::set<int3> discardedTiles;
	for(auto & tile : zone.getTileInfo())
	{
		if(tile.dist2d(zone.getPos()) > distance)
		{
			discardedTiles.insert(tile);
		}
	};
	return discardedTiles;
}

void createBorder(CMapGenerator & gen, const Zone & zone)
{
	for(auto & tile : zone.getTileInfo())
	{
		bool edge = false;
		gen.foreach_neighbour(tile, [&zone, &edge, &gen](int3 &pos)
		{
			if (edge)
				return; //optimization - do it only once
			if (gen.getZoneID(pos) != zone.getId()) //optimization - better than set search
			{
				//bugfix with missing pos
				if (gen.isPossible(pos))
					gen.setOccupied(pos, ETileType::BLOCKED);
				//we are edge if at least one tile does not belong to zone
				//mark all nearby tiles blocked and we're done
				gen.foreach_neighbour(pos, [&gen](int3 &nearbyPos)
				{
					if (gen.isPossible(nearbyPos))
						gen.setOccupied(nearbyPos, ETileType::BLOCKED);
				});
				edge = true;
			}
		});
	}
}

si32 getRandomTownType(const Zone & zone, CRandomGenerator & generator, bool matchUndergroundType)
{
	auto townTypesAllowed = (zone.getTownTypes().size() ? zone.getTownTypes() : zone.getDefaultTownTypes());
	if(matchUndergroundType)
	{
		std::set<TFaction> townTypesVerify;
		for(TFaction factionIdx : townTypesAllowed)
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
	
	return *RandomGeneratorUtil::nextItem(townTypesAllowed, generator);
}

void paintZoneTerrain(const Zone & zone, CMapGenerator & gen, const Terrain & terrainType)
{
	std::vector<int3> tiles(zone.getTileInfo().begin(), zone.getTileInfo().end());
	gen.getEditManager()->getTerrainSelection().setSelection(tiles);
	gen.getEditManager()->drawTerrain(terrainType, &gen.rand);
}

boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}

bool placeMines(const Zone & zone, CMapGenerator & gen, ObjectManager & manager)
{
	using namespace Res;
	std::vector<CGMine*> createdMines;
	
	for(const auto & mineInfo : zone.getMinesInfo())
	{
		ERes res = (ERes)mineInfo.first;
		for(int i = 0; i < mineInfo.second; ++i)
		{
			auto mine = (CGMine*)VLC->objtypeh->getHandlerFor(Obj::MINE, res)->create(ObjectTemplate());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			createdMines.push_back(mine);
			
			if(!i && (res == ERes::WOOD || res == ERes::ORE))
				manager.addCloseObject(mine, gen.getConfig().mineValues.at(res)); //only first wood&ore mines are close
			else
				manager.addRequiredObject(mine, gen.getConfig().mineValues.at(res));
		}
	}
	
	//create extra resources
	if(int extraRes = gen.getConfig().mineExtraResources)
	{
		for(auto * mine : createdMines)
		{
			for(int rc = gen.rand.nextInt(1, extraRes); rc > 0; --rc)
			{
				auto resourse = (CGResource*) VLC->objtypeh->getHandlerFor(Obj::RESOURCE, mine->producedResource)->create(ObjectTemplate());
				resourse->amount = CGResource::RANDOM_AMOUNT;
				manager.addNearbyObject(resourse, mine);
			}
		}
	}
	
	return true;
}

void initTownType(Zone & zone, CMapGenerator & gen, ObjectManager & manager)
{
	//FIXME: handle case that this player is not present -> towns should be set to neutral
	int totalTowns = 0;
	
	//cut a ring around town to ensure crunchPath always hits it.
	auto cutPathAroundTown = [&gen](const CGTownInstance * town)
	{
		auto clearPos = [&gen](const int3 & pos)
		{
			if (gen.isPossible(pos))
				gen.setOccupied(pos, ETileType::FREE);
		};
		for(auto blockedTile : town->getBlockedPos())
		{
			gen.foreach_neighbour(blockedTile, clearPos);
		}
		//clear town entry
		gen.foreach_neighbour(town->visitablePos() + int3{0,1,0}, clearPos);
	};
	
	auto addNewTowns = [&zone, &gen, &manager, &totalTowns, &cutPathAroundTown](int count, bool hasFort, PlayerColor player)
	{
		for(int i = 0; i < count; i++)
		{
			si32 subType = zone.getTownType();
			
			if(totalTowns>0)
			{
				if(!zone.areTownsSameType())
				{
					if (zone.getTownTypes().size())
						subType = *RandomGeneratorUtil::nextItem(zone.getTownTypes(), gen.rand);
					else
						subType = *RandomGeneratorUtil::nextItem(zone.getDefaultTownTypes(), gen.rand); //it is possible to have zone with no towns allowed
				}
			}
			
			auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, subType);
			auto town = (CGTownInstance *) townFactory->create(ObjectTemplate());
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
				gen.registerZone(town->subID);
				//first town in zone goes in the middle
				manager.placeObject(town, zone.getPos() + town->getVisitableOffset(), true);
				cutPathAroundTown(town);
				zone.setPos(town->visitablePos()); //roads lead to mian town
			}
			else
				manager.addRequiredObject(town);
			totalTowns++;
		}
	};
	
	
	if((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START))
	{
		//set zone types to player faction, generate main town
		logGlobal->info("Preparing playing zone");
		int player_id = *zone.getOwner() - 1;
		auto & playerInfo = gen.map->players[player_id];
		PlayerColor player(player_id);
		if(playerInfo.canAnyonePlay())
		{
			player = PlayerColor(player_id);
			zone.setTownType(gen.getMapGenOptions().getPlayersSettings().find(player)->second.getStartingTown());
			
			if(zone.getTownType() == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				zone.setTownType(getRandomTownType(zone, gen.rand, true));
		}
		else //no player - randomize town
		{
			player = PlayerColor::NEUTRAL;
			zone.setTownType(getRandomTownType(zone, gen.rand));
		}
		
		auto townFactory = VLC->objtypeh->getHandlerFor(Obj::TOWN, zone.getTownType());
		
		CGTownInstance * town = (CGTownInstance *) townFactory->create(ObjectTemplate());
		town->tempOwner = player;
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		
		for(auto spell : VLC->spellh->objects) //add all regular spells to town
		{
			if(!spell->isSpecial() && !spell->isCreatureAbility())
				town->possibleSpells.push_back(spell->id);
		}
		//towns are big objects and should be centered around visitable position
		manager.placeObject(town, zone.getPos() + town->getVisitableOffset(), true);
		cutPathAroundTown(town);
		zone.setPos(town->visitablePos()); //roads lead to mian town
		
		totalTowns++;
		//register MAIN town of zone only
		gen.registerZone(town->subID);
		
		if(playerInfo.canAnyonePlay()) //configure info for owning player
		{
			logGlobal->trace("Fill player info %d", player_id);
			
			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(zone.getTownType());
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos;
			playerInfo.generateHeroAtMainTown = true;
			
			//now create actual towns
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, player);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, player);
		}
		else
		{
			addNewTowns(zone.getPlayerTowns().getCastleCount() - 1, true, PlayerColor::NEUTRAL);
			addNewTowns(zone.getPlayerTowns().getTownCount(), false, PlayerColor::NEUTRAL);
		}
	}
	else //randomize town types for any other zones as well
	{
		zone.setTownType(getRandomTownType(zone, gen.rand));
	}
	
	addNewTowns(zone.getNeutralTowns().getCastleCount(), true, PlayerColor::NEUTRAL);
	addNewTowns(zone.getNeutralTowns().getTownCount(), false, PlayerColor::NEUTRAL);
	
	if(!totalTowns) //if there's no town present, get random faction for dwellings and pandoras
	{
		//25% chance for neutral
		if (gen.rand.nextInt(1, 100) <= 25)
		{
			zone.setTownType(ETownType::NEUTRAL);
		}
		else
		{
			if(zone.getTownTypes().size())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getTownTypes(), gen.rand));
			else if(zone.getMonsterTypes().size())
				zone.setTownType(*RandomGeneratorUtil::nextItem(zone.getMonsterTypes(), gen.rand)); //this happens in Clash of Dragons in treasure zones, where all towns are banned
			else //just in any case
				zone.setTownType(getRandomTownType(zone, gen.rand));
		}
	}
}

void createObstacles1(const Zone & zone, CMapGenerator & gen)
{
	if(zone.isUnderground()) //underground
	{
		//now make sure all accessible tiles have no additional rock on them
		std::vector<int3> accessibleTiles;
		for(auto tile : zone.getTileInfo())
		{
			if(gen.isFree(tile) || gen.isUsed(tile))
			{
				accessibleTiles.push_back(tile);
			}
		}
		gen.getEditManager()->getTerrainSelection().setSelection(accessibleTiles);
		gen.getEditManager()->drawTerrain(zone.getTerrainType(), &gen.rand);
	}
}

void createObstacles2(const Zone & zone, CMapGenerator & gen, ObjectManager & manager)
{
	typedef std::vector<ObjectTemplate> obstacleVector;
	//obstacleVector possibleObstacles;
	
	std::map <ui8, obstacleVector> obstaclesBySize;
	typedef std::pair <ui8, obstacleVector> obstaclePair;
	std::vector<obstaclePair> possibleObstacles;
	
	//get all possible obstacles for this terrain
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler->isStaticObject())
			{
				for(auto temp : handler->getTemplates())
				{
					if(temp.canBePlacedAt(zone.getTerrainType()) && temp.getBlockMapOffset().valid())
						obstaclesBySize[(ui8)temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for (auto o : obstaclesBySize)
	{
		possibleObstacles.push_back (std::make_pair(o.first, o.second));
	}
	boost::sort (possibleObstacles, [](const obstaclePair &p1, const obstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
	
	auto sel = gen.getEditManager()->getTerrainSelection();
	sel.clearSelection();
	
	auto tryToPlaceObstacleHere = [&gen, &manager, &possibleObstacles](int3& tile, int index)-> bool
	{
		auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles[index].second, gen.rand);
		int3 obstaclePos = tile + temp.getBlockMapOffset();
		if(canObstacleBePlacedHere(gen, temp, obstaclePos)) //can be placed here
		{
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			manager.placeObject(obj, obstaclePos, false);
			return true;
		}
		return false;
	};
	
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	for(auto tile : boost::adaptors::reverse(zone.getTileInfo()))
	{
		//fill tiles that should be blocked with obstacles
		if(gen.shouldBeBlocked(tile))
		{
			//start from biggets obstacles
			for(int i = 0; i < possibleObstacles.size(); i++)
			{
				if (tryToPlaceObstacleHere(tile, i))
					break;
			}
		}
	}
	//cleanup - remove unused possible tiles to make space for roads
	for(auto tile : zone.getTileInfo())
	{
		if(gen.isPossible(tile))
		{
			gen.setOccupied(tile, ETileType::FREE);
		}
	}
}

bool canObstacleBePlacedHere(const CMapGenerator & gen, ObjectTemplate &temp, int3 &pos)
{
	if (!gen.map->isInTheMap(pos)) //blockmap may fit in the map, but botom-right corner does not
		return false;
	
	auto tilesBlockedByObject = temp.getBlockedOffsets();
	
	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = pos + blockingTile;
		if (!gen.map->isInTheMap(t) || !(gen.isPossible(t) || gen.shouldBeBlocked(t)) || !temp.canBePlacedAt(gen.map->getTile(t).terType))
		{
			return false; //if at least one tile is not possible, object can't be placed here
		}
	}
	return true;
}

void placeSubterraneanGate(Zone & zone, ObjectManager & manager, int3 pos, si32 guardStrength)
{
	auto factory = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
	auto gate = factory->create(ObjectTemplate());
	manager.placeObject(gate, pos, true);
	zone.addToConnectLater(manager.getAccessibleOffset(gate->appearance, pos)); //guard will be placed on accessibleOffset
	manager.guardObject(gate, guardStrength, true);
}

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, const Terrain & terrain)
{
	auto factories = VLC->objtypeh->knownSubObjects(ObjID);
	vstd::erase_if(factories, [ObjID, &terrain](si32 f)
	{
		return VLC->objtypeh->getHandlerFor(ObjID, f)->getTemplates(terrain).empty();
	});
	
	return *RandomGeneratorUtil::nextItem(factories, generator);
}

void initTerrainType(Zone & zone, CMapGenerator & gen)
{
	if(zone.getType()==ETemplateZoneType::WATER)
	{
		//collect all water terrain types
		std::vector<Terrain> waterTerrains;
		for(auto & terrain : Terrain::Manager::terrains())
			if(terrain.isWater())
				waterTerrains.push_back(terrain);
		
		zone.setTerrainType(*RandomGeneratorUtil::nextItem(waterTerrains, gen.rand));
	}
	else
	{
		if(zone.isMatchTerrainToTown() && zone.getTownType() != ETownType::NEUTRAL)
		{
			zone.setTerrainType((*VLC->townh)[zone.getTownType()]->nativeTerrain);
		}
		else
		{
			zone.setTerrainType(*RandomGeneratorUtil::nextItem(zone.getTerrainTypes(), gen.rand));
		}
		
		//TODO: allow new types of terrain?
		{
			if(zone.isUnderground())
			{
				if(!vstd::contains(gen.getConfig().terrainUndergroundAllowed, zone.getTerrainType()))
				{
					//collect all underground terrain types
					std::vector<Terrain> undegroundTerrains;
					for(auto & terrain : Terrain::Manager::terrains())
						if(terrain.isUnderground())
							undegroundTerrains.push_back(terrain);
					
					zone.setTerrainType(*RandomGeneratorUtil::nextItem(undegroundTerrains, gen.rand));
				}
			}
			else
			{
				if(vstd::contains(gen.getConfig().terrainGroundProhibit, zone.getTerrainType()) || zone.getTerrainType().isUnderground())
					zone.setTerrainType(Terrain("dirt"));
			}
		}
	}
}

bool processZone(Zone & zone, CMapGenerator & gen)
{
	ObjectManager manager(zone, gen);
	RoadPlacer roadPlacer(zone, gen);
	TreasurePlacer treasurePlacer(zone, gen);
	
	initTerrainType(zone, gen);
	paintZoneTerrain(zone, gen, zone.getTerrainType());
	
	treasurePlacer.addAllPossibleObjects();
	
	//zone center should be always clear to allow other tiles to connect
	zone.initFreeTiles();
	zone.connectLater(); //ideally this should work after fractalize, but fails
	zone.fractalize();
	placeMines(zone, gen, manager);
	manager.createRequiredObjects();
	treasurePlacer.createTreasures(manager);
	
	logGlobal->info("Zone %d filled successfully", zone.getId());
	return true;
}
