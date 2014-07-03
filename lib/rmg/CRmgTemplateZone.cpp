
/*
 * CRmgTemplateZone.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRmgTemplateZone.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"

#include "../VCMI_Lib.h"
#include "../CTownHandler.h"
#include "../CCreatureHandler.h"
#include "../CSpellHandler.h" //for choosing random spells

#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CGPandoraBox.h"
#include "../mapObjects/CRewardableObject.h"

class CMap;
class CMapEditManager;

CRmgTemplateZone::CTownInfo::CTownInfo() : townCount(0), castleCount(0), townDensity(0), castleDensity(0)
{

}

int CRmgTemplateZone::CTownInfo::getTownCount() const
{
	return townCount;
}

void CRmgTemplateZone::CTownInfo::setTownCount(int value)
{
	if(value < 0)
		throw rmgException("Negative value for town count not allowed.");
	townCount = value;
}

int CRmgTemplateZone::CTownInfo::getCastleCount() const
{
	return castleCount;
}

void CRmgTemplateZone::CTownInfo::setCastleCount(int value)
{
	if(value < 0)
		throw rmgException("Negative value for castle count not allowed.");
	castleCount = value;
}

int CRmgTemplateZone::CTownInfo::getTownDensity() const
{
	return townDensity;
}

void CRmgTemplateZone::CTownInfo::setTownDensity(int value)
{
	if(value < 0)
		throw rmgException("Negative value for town density not allowed.");
	townDensity = value;
}

int CRmgTemplateZone::CTownInfo::getCastleDensity() const
{
	return castleDensity;
}

void CRmgTemplateZone::CTownInfo::setCastleDensity(int value)
{
	if(value < 0)
		throw rmgException("Negative value for castle density not allowed.");
	castleDensity = value;
}

CTileInfo::CTileInfo():nearestObjectDistance(INT_MAX), terrain(ETerrainType::WRONG) 
{
	occupied = ETileType::POSSIBLE; //all tiles are initially possible to place objects or passages
}

int CTileInfo::getNearestObjectDistance() const
{
	return nearestObjectDistance;
}

void CTileInfo::setNearestObjectDistance(int value)
{
	nearestObjectDistance = std::max(0, value); //never negative (or unitialized)
}
bool CTileInfo::shouldBeBlocked() const
{
	return occupied == ETileType::BLOCKED;
}
bool CTileInfo::isBlocked() const
{
	return occupied == ETileType::BLOCKED || occupied == ETileType::USED;
}
bool CTileInfo::isPossible() const
{
	return occupied == ETileType::POSSIBLE;
}
bool CTileInfo::isFree() const
{
	return occupied == ETileType::FREE;
}
void CTileInfo::setOccupied(ETileType::ETileType value)
{
	occupied = value;
}

ETerrainType CTileInfo::getTerrainType() const
{
	return terrain;
}

void CTileInfo::setTerrainType(ETerrainType value)
{
	terrain = value;
}

CRmgTemplateZone::CRmgTemplateZone() :
	id(0),
	type(ETemplateZoneType::PLAYER_START),
	size(1),
	townsAreSameType(false),
	matchTerrainToTown(true),
	townType(0),
	terrainType (ETerrainType::GRASS),
	zoneMonsterStrength(EMonsterStrength::ZONE_NORMAL),
	totalDensity(0)
{
	townTypes = getDefaultTownTypes();
	terrainTypes = getDefaultTerrainTypes();
}

TRmgTemplateZoneId CRmgTemplateZone::getId() const
{
	return id;
}

void CRmgTemplateZone::setId(TRmgTemplateZoneId value)
{
	if(value <= 0)
		throw rmgException(boost::to_string(boost::format("Zone %d id should be greater than 0.") %id));
	id = value;
}

ETemplateZoneType::ETemplateZoneType CRmgTemplateZone::getType() const
{
	return type;
}
void CRmgTemplateZone::setType(ETemplateZoneType::ETemplateZoneType value)
{
	type = value;
}

int CRmgTemplateZone::getSize() const
{
	return size;
}

void CRmgTemplateZone::setSize(int value)
{
	if(value <= 0)
		throw rmgException(boost::to_string(boost::format("Zone %d size needs to be greater than 0.") % id));
	size = value;
}

boost::optional<int> CRmgTemplateZone::getOwner() const
{
	return owner;
}

void CRmgTemplateZone::setOwner(boost::optional<int> value)
{
	if(!(*value >= 0 && *value <= PlayerColor::PLAYER_LIMIT_I))
		throw rmgException(boost::to_string(boost::format ("Owner of zone %d has to be in range 0 to max player count.") %id));
	owner = value;
}

const CRmgTemplateZone::CTownInfo & CRmgTemplateZone::getPlayerTowns() const
{
	return playerTowns;
}

void CRmgTemplateZone::setPlayerTowns(const CTownInfo & value)
{
	playerTowns = value;
}

const CRmgTemplateZone::CTownInfo & CRmgTemplateZone::getNeutralTowns() const
{
	return neutralTowns;
}

void CRmgTemplateZone::setNeutralTowns(const CTownInfo & value)
{
	neutralTowns = value;
}

bool CRmgTemplateZone::getTownsAreSameType() const
{
	return townsAreSameType;
}

void CRmgTemplateZone::setTownsAreSameType(bool value)
{
	townsAreSameType = value;
}

const std::set<TFaction> & CRmgTemplateZone::getTownTypes() const
{
	return townTypes;
}

void CRmgTemplateZone::setTownTypes(const std::set<TFaction> & value)
{
	townTypes = value;
}

std::set<TFaction> CRmgTemplateZone::getDefaultTownTypes() const
{
	std::set<TFaction> defaultTowns;
	auto towns = VLC->townh->getDefaultAllowed();
	for(int i = 0; i < towns.size(); ++i)
	{
		if(towns[i]) defaultTowns.insert(i);
	}
	return defaultTowns;
}

bool CRmgTemplateZone::getMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

void CRmgTemplateZone::setMatchTerrainToTown(bool value)
{
	matchTerrainToTown = value;
}

const std::set<ETerrainType> & CRmgTemplateZone::getTerrainTypes() const
{
	return terrainTypes;
}

void CRmgTemplateZone::setTerrainTypes(const std::set<ETerrainType> & value)
{
	assert(value.find(ETerrainType::WRONG) == value.end() && value.find(ETerrainType::BORDER) == value.end() &&
		   value.find(ETerrainType::WATER) == value.end() && value.find(ETerrainType::ROCK) == value.end());
	terrainTypes = value;
}

std::set<ETerrainType> CRmgTemplateZone::getDefaultTerrainTypes() const
{
	std::set<ETerrainType> terTypes;
	static const ETerrainType::EETerrainType allowedTerTypes[] = {ETerrainType::DIRT, ETerrainType::SAND, ETerrainType::GRASS, ETerrainType::SNOW,
												   ETerrainType::SWAMP, ETerrainType::ROUGH, ETerrainType::SUBTERRANEAN, ETerrainType::LAVA};
	for (auto & allowedTerType : allowedTerTypes)
		terTypes.insert(allowedTerType);

	return terTypes;
}

void CRmgTemplateZone::setMinesAmount (TResource res, ui16 amount)
{
	mines[res] = amount;
}

std::map<TResource, ui16> CRmgTemplateZone::getMinesInfo() const
{
	return mines;
}

void CRmgTemplateZone::addConnection(TRmgTemplateZoneId otherZone)
{
	connections.push_back (otherZone);
}

std::vector<TRmgTemplateZoneId> CRmgTemplateZone::getConnections() const
{
	return connections;
}

void CRmgTemplateZone::setMonsterStrength (EMonsterStrength::EMonsterStrength val)
{
	assert (vstd::iswithin(val, EMonsterStrength::ZONE_WEAK, EMonsterStrength::ZONE_STRONG));
	zoneMonsterStrength = val;
}

void CRmgTemplateZone::setTotalDensity (ui16 val)
{
	totalDensity = val;
}

ui16 CRmgTemplateZone::getTotalDensity () const
{
	return totalDensity;
}

void CRmgTemplateZone::addTreasureInfo(CTreasureInfo & info)
{
	treasureInfo.push_back(info);
}

std::vector<CTreasureInfo> CRmgTemplateZone::getTreasureInfo()
{
	return treasureInfo;
}

std::set<int3>* CRmgTemplateZone::getFreePaths()
{
	return &freePaths;
}

float3 CRmgTemplateZone::getCenter() const
{
	return center;
}
void CRmgTemplateZone::setCenter(const float3 &f)
{
	//limit boundaries to (0,1) square
	center = float3 (std::min(std::max(f.x, 0.f), 1.f), std::min(std::max(f.y, 0.f), 1.f), f.z);
}


bool CRmgTemplateZone::pointIsIn(int x, int y)
{
	return true;
}

int3 CRmgTemplateZone::getPos() const
{
	return pos;
}
void CRmgTemplateZone::setPos(const int3 &Pos)
{
	pos = Pos;
}

void CRmgTemplateZone::addTile (const int3 &pos)
{
	tileinfo.insert(pos);
}

std::set<int3> CRmgTemplateZone::getTileInfo () const
{
	return tileinfo;
}

void CRmgTemplateZone::discardDistantTiles (int distance)
{
	vstd::erase_if (tileinfo, [distance, this](const int3 &tile) -> bool
	{
		return tile.dist2d(this->pos) > distance;
	});
}

void CRmgTemplateZone::createBorder(CMapGenerator* gen)
{
	for (auto tile : tileinfo)
	{
		gen->foreach_neighbour (tile, [this, gen](int3 &pos)
		{
			if (!vstd::contains(this->tileinfo, pos))
			{
				gen->foreach_neighbour (pos, [this, gen](int3 &pos)
				{
					if (gen->isPossible(pos))
						gen->setOccupied (pos, ETileType::BLOCKED);
				});
			}
		});
	}
}

void CRmgTemplateZone::fractalize(CMapGenerator* gen)
{
	std::vector<int3> clearedTiles;
	std::set<int3> possibleTiles;
	std::set<int3> tilesToClear; //will be set clear
	std::set<int3> tilesToIgnore; //will be erased in this iteration

	const float minDistance = std::sqrt(totalDensity);
	for (auto tile : tileinfo)
	{
		if (gen->isFree(tile))
			clearedTiles.push_back(tile);
		else if (gen->isPossible(tile))
			possibleTiles.insert(tile);
	}
	if (clearedTiles.empty()) //this should come from zone connections
	{
		clearedTiles.push_back(pos); //zone center should be always clear
	}

	while (possibleTiles.size())
	{
		for (auto tileToMakePath : possibleTiles)
		{
			//find closest free tile
			float currentDistance = 1e10;
			int3 closestTile (-1,-1,-1);

			for (auto clearTile : clearedTiles)
			{
				float distance = tileToMakePath.dist2d(clearTile);
				
				if (distance < currentDistance)
				{
					currentDistance = distance;
					closestTile = clearTile;
				}
				if (currentDistance <= minDistance)
				{
					//this tile is close enough. Forget about it and check next one
					tilesToIgnore.insert (tileToMakePath);
					break;
				}
			}
			//if tiles is not close enough, make path to it
			if (currentDistance > minDistance)
			{
				crunchPath (gen, tileToMakePath, closestTile, id, &tilesToClear);
				break; //next iteration - use already cleared tiles
			}
		}

		for (auto tileToClear : tilesToClear)
		{
			//move cleared tiles from one set to another
			clearedTiles.push_back(tileToClear);
			vstd::erase_if_present(possibleTiles, tileToClear);
		}
		for (auto tileToClear : tilesToIgnore)
		{
			//these tiles are already connected, ignore them
			vstd::erase_if_present(possibleTiles, tileToClear);
		}
		if (tilesToClear.empty()) //nothing else can be done (?)
			break;
		tilesToClear.clear(); //empty this container
		tilesToIgnore.clear();
	}
	logGlobal->infoStream() << boost::format ("Zone %d subdivided fractally") %id;
}

bool CRmgTemplateZone::crunchPath (CMapGenerator* gen, const int3 &src, const int3 &dst, TRmgTemplateZoneId zone, std::set<int3>* clearedTiles)
{
/*
make shortest path with free tiles, reachning dst or closest already free tile. Avoid blocks.
do not leave zone border
*/
	bool result = false;
	bool end = false;

	int3 currentPos = src;
	float distance = currentPos.dist2dSQ (dst);

	while (!end)
	{
		if (currentPos == dst)
		{
			result = true;
			break;
		}

		auto lastDistance = distance;
		gen->foreach_neighbour (currentPos, [this, gen, &currentPos, dst, &distance, &result, &end, clearedTiles](int3 &pos)
		{
			if (!result) //not sure if lambda is worth it...
			{
				if (pos == dst)
				{
					result = true;
					end = true;
				}
				if (pos.dist2dSQ (dst) < distance)
				{
					if (!gen->isBlocked(pos))
					{
						if (vstd::contains (tileinfo, pos))
						{
							if (gen->isPossible(pos))
							{
								gen->setOccupied (pos, ETileType::FREE);
								if (clearedTiles)
									clearedTiles->insert(pos);
								currentPos = pos;
								distance = currentPos.dist2dSQ (dst);
							}
							else if (gen->isFree(pos))
							{
								end = true;
								result = true;
							}
							else
								throw rmgException(boost::to_string(boost::format("Tile %s of uknown type found on path") % pos()));
						}
					}
				}
			}
		});
		if (!(result || distance < lastDistance)) //we do not advance, use more avdnaced pathfinding algorithm?
		{
			logGlobal->warnStream() << boost::format ("No tile closer than %s found on path from %s to %s") %currentPos %src %dst;
			break;
		}
	}

	return result;
}

void CRmgTemplateZone::addRequiredObject(CGObjectInstance * obj, si32 strength)
{
	requiredObjects.push_back(std::make_pair(obj, strength));
}

bool CRmgTemplateZone::addMonster(CMapGenerator* gen, int3 &pos, si32 strength)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426

	int mapMonsterStrength = gen->mapGenOptions->getMonsterStrength();
	int monsterStrength = zoneMonsterStrength + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};

	int strength1 = std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]); 
	int strength2 = std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]); 

	strength = strength1 + strength2;
	if (strength < 2000)
		return false; //no guard at all

	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for (auto cre : VLC->creh->creatures)
	{
		if (cre->special)
			continue;
		if ((cre->AIValue * (cre->ammMin + cre->ammMax) / 2 < strength) && (strength < cre->AIValue * 100)) //at least one full monster. size between minimum size of given stack and 100
		{
			possibleCreatures.push_back(cre->idNumber);
		}
	}
	if (possibleCreatures.size())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, gen->rand);
		amount = strength / VLC->creh->creatures[creId]->AIValue;
		if (amount >= 4)
			amount *= gen->rand.nextDouble(0.75, 1.25);
	}
	else //just pick any available creature
	{
		creId = CreatureID(132); //Azure Dragon
		amount = strength / VLC->creh->creatures[creId]->AIValue;
	}


	auto guard = new CGCreature();
	guard->ID = Obj::MONSTER;
	guard->subID = creId;
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);

	placeObject(gen, guard, pos);
	return true;
}

bool CRmgTemplateZone::createTreasurePile (CMapGenerator* gen, int3 &pos)
{
	std::map<int3, CGObjectInstance *> treasures;
	std::set<int3> boundary;
	int3 guardPos (-1,-1,-1);
	int3 nextTreasurePos = pos;

	//default values
	int maxValue = 5000;
	int minValue = 1500;

	if (treasureInfo.size())
	{
		//roulette wheel
		int r = gen->rand.nextInt (1, totalDensity);

		for (auto t : treasureInfo)
		{
			if (r <= t.threshold)
			{
				maxValue = t.max;
				minValue = t.min;
				break;
			}
		}
	}

	int currentValue = 0;
	CGObjectInstance * object = nullptr;
	while (currentValue < minValue)
	{
		//TODO: this works only for 1-tile objects
		//make sure our shape is consistent
		treasures[nextTreasurePos] = nullptr;
		for (auto treasurePos : treasures)
		{
			gen->foreach_neighbour (treasurePos.first, [gen, &boundary](int3 pos)
			{
				boundary.insert(pos);
			});
		}
		for (auto treasurePos : treasures)
		{
			//leaving only boundary around objects
			vstd::erase_if_present (boundary, treasurePos.first);
		}
		for (auto tile : boundary)
		{
			//we can't extend boundary anymore
			if (!(gen->isBlocked(tile) || gen->isPossible(tile)))
				break;
		}

		int remaining = maxValue - currentValue;

		auto oi = getRandomObject(gen, remaining);
		object = oi.generateObject();
		if (!object)
			break;

		currentValue += oi.value;
		
		treasures[nextTreasurePos] = object;

		//now find place for next object
		int3 placeFound(-1,-1,-1);

		//FIXME: find out why teh following code causes crashes
		//std::vector<int3> boundaryVec(boundary.begin(), boundary.end());
		//RandomGeneratorUtil::randomShuffle(boundaryVec, gen->rand);
		//for (auto tile : boundaryVec)
		for (auto tile : boundary)
		{
			if (gen->isPossible(tile)) //we can place new treasure only on possible tile
			{
				bool here = true;
				gen->foreach_neighbour (tile, [gen, &here](int3 pos)
				{
					if (!(gen->isBlocked(pos) || gen->isPossible(pos)))
						here = false;
				});
				if (here)
				{
					placeFound = tile;
					break;
				}
			}
		}
		if (placeFound.valid())
			nextTreasurePos = placeFound;
	}
	if (treasures.size())
	{
		//find object closest to zone center, then con nect it to the middle of the zone
		int3 zoneCenter = getPos();
		int3 closestTile = int3(-1,-1,-1);
		float minDistance = 1e10;
		for (auto treasure : treasures)
		{
			if (zoneCenter.dist2d(treasure.first) < minDistance)
			{
				closestTile = treasure.first;
				minDistance = zoneCenter.dist2d(treasure.first);
			}
		}
		assert (closestTile.valid());
		if (!crunchPath (gen, closestTile, getPos(), id)) //make sure pile is connected to the middle of zone
		{
			for (auto treasure : treasures)
			{
				if (gen->isPossible(treasure.first))
					gen->setOccupied (treasure.first, ETileType::BLOCKED);
			}
			return true;
		}

		for (auto tile : boundary) //guard must be standing there
		{
			if (gen->isFree(tile)) //this tile could be already blocked, don't place a monster here
			{
				guardPos = tile;
				break;
			}
		}

		if (guardPos.valid())
		{
			for (auto treasure : treasures)
			{
				placeObject(gen, treasure.second, treasure.first - treasure.second->getVisitableOffset());
			}
			if (addMonster(gen, guardPos, currentValue))
			{//block only if the object is guarded
				for (auto tile : boundary)
				{
					if (gen->isPossible(tile))
						gen->setOccupied (tile, ETileType::BLOCKED);
				}
			}
		}
		else //we couldn't make a connection to this location, block it
		{
			for (auto treasure : treasures)
			{
				if (gen->isPossible(treasure.first))
					gen->setOccupied (treasure.first, ETileType::BLOCKED);
			}
		}

		return true;
	}
	else //we did not place eveyrthing successfully
		return false;
}
void CRmgTemplateZone::initTownType (CMapGenerator* gen)
{
	//FIXME: handle case that this player is not present -> towns should be set to neutral
	int totalTowns = 0;

	auto addNewTowns = [&totalTowns, gen, this](int count, bool hasFort, PlayerColor player)
	{
		for (int i = 0; i < count; i++)
		{
			auto  town = new CGTownInstance();
			town->ID = Obj::TOWN;

			if (this->townsAreSameType)
				town->subID = townType;
			else
				town->subID = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand); //TODO: check allowed town types for this zone

			town->tempOwner = player;
			if (hasFort)
				town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);

			if (!totalTowns) //first town in zone goes in the middle
				placeObject(gen, town, getPos() + town->getVisitableOffset());
			else
				addRequiredObject (town);
			totalTowns++;
		}
	};


	if ((type == ETemplateZoneType::CPU_START) || (type == ETemplateZoneType::PLAYER_START))
	{
		//set zone types to player faction, generate main town
		logGlobal->infoStream() << "Preparing playing zone";
		int player_id = *owner - 1;
		auto & playerInfo = gen->map->players[player_id];
		if (playerInfo.canAnyonePlay())
		{
			PlayerColor player(player_id);
			townType = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if(townType == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				townType = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand); // all possible towns, skip neutral
			
			auto  town = new CGTownInstance();
			town->ID = Obj::TOWN;

			town->subID = townType;
			town->tempOwner = player;
			town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);
			placeObject(gen, town, getPos() + town->getVisitableOffset()); //towns are big objects and should be centered around visitable position

			totalTowns++;

			logGlobal->traceStream() << "Fill player info " << player_id;

			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert (townType);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos - town->getVisitableOffset();
			playerInfo.generateHeroAtMainTown = true;

			//now create actual towns
			addNewTowns (playerTowns.getCastleCount() - 1, true, player);
			addNewTowns (playerTowns.getTownCount(), false, player);

			//requiredObjects.push_back(town);
		}
		else
		{			
			type = ETemplateZoneType::TREASURE;
			townType = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand);
			logGlobal->infoStream() << "Skipping this zone cause no player";
		}
	}
	else //no player
	{
		townType = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand);
	}

	addNewTowns (neutralTowns.getCastleCount(), true, PlayerColor::NEUTRAL);
	addNewTowns (neutralTowns.getTownCount(), false, PlayerColor::NEUTRAL);
}

void CRmgTemplateZone::initTerrainType (CMapGenerator* gen)
{

	if (matchTerrainToTown)
		terrainType = VLC->townh->factions[townType]->nativeTerrain;
	else
		terrainType = *RandomGeneratorUtil::nextItem(terrainTypes, gen->rand);

	//paint zone with matching terrain
	std::vector<int3> tiles;
	for (auto tile : tileinfo)
	{
		tiles.push_back (tile);
	}
	gen->editManager->getTerrainSelection().setSelection(tiles);
	gen->editManager->drawTerrain(terrainType, &gen->rand);
}

bool CRmgTemplateZone::placeMines (CMapGenerator* gen)
{
	std::vector<Res::ERes> required_mines;
	required_mines.push_back(Res::ERes::WOOD);
	required_mines.push_back(Res::ERes::ORE);

	static const Res::ERes woodOre[] = {Res::ERes::WOOD, Res::ERes::ORE};
	static const Res::ERes preciousResources[] = {Res::ERes::GEMS, Res::ERes::CRYSTAL, Res::ERes::MERCURY, Res::ERes::SULFUR};


	//TODO: factory / copy constructor?
	for (const auto & res : woodOre)
	{
		//TODO: these 2 should be close to town (within 12 tiles radius)
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = new CGMine();
			mine->ID = Obj::MINE;
			mine->subID = static_cast<si32>(res);
			mine->producedResource = res;
			mine->producedQuantity = mine->defaultResProduction();
			addRequiredObject(mine, 1500);
		}
	}
	for (const auto & res : preciousResources)
	{
		for (int i = 0; i < mines[res]; i++)
		{
			auto mine = new CGMine();
			mine->ID = Obj::MINE;
			mine->subID = static_cast<si32>(res);
			mine->producedResource = res;
			mine->producedQuantity = mine->defaultResProduction();
			addRequiredObject(mine, 3500);
		}
	}
	for (int i = 0; i < mines[Res::GOLD]; i++)
	{
		auto mine = new CGMine();
		mine->ID = Obj::MINE;
		mine->subID = static_cast<si32>(Res::GOLD);
		mine->producedResource = Res::GOLD;
		mine->producedQuantity = mine->defaultResProduction();
		addRequiredObject(mine, 7000);
	}

	return true;
}

bool CRmgTemplateZone::createRequiredObjects(CMapGenerator* gen)
{
	logGlobal->infoStream() << "Creating required objects";
	for(const auto &obj : requiredObjects)
	{
		int3 pos;
		logGlobal->traceStream() << "Looking for place";
		if ( ! findPlaceForObject(gen, obj.first, 3, pos))		
		{
			logGlobal->errorStream() << boost::format("Failed to fill zone %d due to lack of space") %id;
			//TODO CLEANUP!
			return false;
		}
		logGlobal->traceStream() << "Place found";

		placeObject(gen, obj.first, pos);
		guardObject (gen, obj.first, obj.second);
	}
	return true;
}

void CRmgTemplateZone::createTreasures(CMapGenerator* gen)
{
	const double minDistance = 3;

	do {
		
		int3 pos;
		if ( ! findPlaceForTreasurePile(gen,  minDistance, pos))		
		{
			break;
		}
		createTreasurePile (gen, pos);

	} while(true);
}

void CRmgTemplateZone::createObstacles(CMapGenerator* gen)
{
	//get all possible obstacles for this terrain
	for (auto primaryID : VLC->objtypeh->knownObjects()) 
	{ 
		for (auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID)) 
		{ 
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID); 
			if (handler->isStaticObject())
			{
				for (auto temp : handler->getTemplates())
				{
					if (temp.canBePlacedAt(terrainType) && temp.getBlockMapOffset().valid())
						possibleObstacles.push_back(temp);
				}
			}
		} 
	}

	auto sel = gen->editManager->getTerrainSelection();
	sel.clearSelection();

	auto tryToPlaceObstacleHere = [this, gen](int3& tile)-> bool
	{
		auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles, gen->rand);
		int3 obstaclePos = tile + temp.getBlockMapOffset();
		if (canObstacleBePlacedHere(gen, temp, obstaclePos)) //can be placed here
		{
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			placeObject(gen, obj, obstaclePos);
			return true;
		}
		return false;
	};

	for (auto tile : tileinfo)
	{
		if (gen->shouldBeBlocked(tile)) //fill tiles that should be blocked with obstacles
		{
			while (!tryToPlaceObstacleHere(tile));
		}
		else if (gen->isPossible(tile))
		{
			//try to place random obstacle once - if not possible, leave it clear
			tryToPlaceObstacleHere(tile);
		}
	}
}

bool CRmgTemplateZone::fill(CMapGenerator* gen)
{
	addAllPossibleObjects (gen);
	initTownType(gen);
	initTerrainType(gen);
	placeMines(gen);
	createRequiredObjects(gen);
	fractalize(gen); //after required objects are created and linked with their own paths
	createTreasures(gen);
	createObstacles(gen);

	logGlobal->infoStream() << boost::format ("Zone %d filled successfully") %id;
	return true;
}

bool CRmgTemplateZone::findPlaceForTreasurePile(CMapGenerator* gen, si32 min_dist, int3 &pos)
{
	//si32 min_dist = sqrt(tileinfo.size()/density);
	int best_distance = 0;
	bool result = false;

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;
	for(auto tile : tileinfo)
	{
		auto dist = gen->getTile(tile).getNearestObjectDistance();

		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			gen->foreach_neighbour (tile, [&gen, &allTilesAvailable](int3 neighbour)
			{
				if (!(gen->isPossible(neighbour) || gen->isBlocked(neighbour)))
				{
					allTilesAvailable = false; //all present tiles must be already blocked or ready for new objects
				}
			});
			if (allTilesAvailable)
			{
				best_distance = dist;
				pos = tile;
				result = true;
			}
		}
	}
	if (result)
	{
		gen->setOccupied(pos, ETileType::BLOCKED); //block that tile
	}
	return result;
}

bool CRmgTemplateZone::canObstacleBePlacedHere(CMapGenerator* gen, ObjectTemplate &temp, int3 &pos)
{
	auto tilesBlockedByObject = temp.getBlockedOffsets();

	bool allTilesAvailable = true;
	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = pos + blockingTile;
		if (!gen->map->isInTheMap(t) || !(gen->isPossible(t) || gen->shouldBeBlocked(t)))
		{
			allTilesAvailable = false; //if at least one tile is not possible, object can't be placed here
			break;
		}
	}
	return allTilesAvailable;
}

bool CRmgTemplateZone::findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tiles
	if (obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID)->getTemplates(gen->map->getTile(getPos()).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %obj->ID %obj->subID %pos));
	
		obj->appearance = templates.front();
	}

	//si32 min_dist = sqrt(tileinfo.size()/density);
	int best_distance = 0;
	bool result = false;
	//si32 w = gen->map->width;
	//si32 h = gen->map->height;

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;

	auto tilesBlockedByObject = obj->getBlockedOffsets();

	for (auto tile : tileinfo)
	{
		//object must be accessible from at least one surounding tile
		bool accessible = false;
		for (int x = -1; x < 2; x++)
			for (int y = -1; y <2; y++)
		{
			if (x && y) //check only if object is visitable from another tile
			{
				int3 offset = obj->getVisitableOffset() + int3(x, y, 0);
				if (!vstd::contains(tilesBlockedByObject, offset))
				{
					int3 nearbyPos = tile + offset;
					if (gen->map->isInTheMap(nearbyPos))
					{
						if (obj->appearance.isVisitableFrom(x, y) && !gen->isBlocked(nearbyPos))
							accessible = true;
					}
				}
			}
		};
		if (!accessible)
			continue;

		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			for (auto blockingTile : tilesBlockedByObject)
			{
				int3 t = tile + blockingTile;
				if (!gen->map->isInTheMap(t) || !gen->isPossible(t))
				{
					allTilesAvailable = false; //if at least one tile is not possible, object can't be placed here
					break;
				}
			}
			if (allTilesAvailable)
			{
				best_distance = dist;
				pos = tile;
				result = true;
			}
		}
	}
	if (result)
	{
		gen->setOccupied(pos, ETileType::BLOCKED); //block that tile
	}
	return result;
}

void CRmgTemplateZone::checkAndPlaceObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos)
{
	if (!gen->map->isInTheMap(pos))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % object->id % object->pos()));
	object->pos = pos;

	if (object->isVisitable() && !gen->map->isInTheMap(object->visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % object->visitablePos() % object->id % object->pos()));
	for (auto tile : object->getBlockedPos())
	{
		if (!gen->map->isInTheMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile() % object->id % object->pos()));
	}

	if (object->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(object->ID, object->subID)->getTemplates(gen->map->getTile(pos).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %object->ID %object->subID %pos));
	
		object->appearance = templates.front();
	}

	gen->map->addBlockVisTiles(object);
	gen->editManager->insertObject(object, pos);
	//logGlobal->traceStream() << boost::format ("Successfully inserted object (%d,%d) at pos %s") %object->ID %object->subID %pos();
}

void CRmgTemplateZone::placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos)
{
	//logGlobal->traceStream() << boost::format("Inserting object at %d %d") % pos.x % pos.y;

	checkAndPlaceObject (gen, object, pos);

	auto points = object->getBlockedPos();
	if (object->isVisitable())
		points.insert(pos + object->getVisitableOffset());
	points.insert(pos);
	for(auto p : points)
	{		
		if (gen->map->isInTheMap(p))
		{
			gen->setOccupied(p, ETileType::USED);
		}
	}
	for(auto tile : tileinfo)
	{		
		si32 d = pos.dist2dSQ(tile); //optimization, only relative distance is interesting
		gen->setNearestObjectDistance(tile, std::min(d, gen->getNearestObjectDistance(tile)));
	}
}

std::vector<int3> CRmgTemplateZone::getAccessibleOffsets (CMapGenerator* gen, CGObjectInstance* object)
{
	//get all tiles from which this object can be accessed
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;

	auto tilesBlockedByObject = object->getBlockedPos(); //absolue value, as object is already placed

	gen->foreach_neighbour(visitable, [&](int3& pos) 
	{
		if (gen->isPossible(pos))
		{
			if (!vstd::contains(tilesBlockedByObject, pos))
			{
				if (object->appearance.isVisitableFrom(pos.x - visitable.x, pos.y - visitable.y) && !gen->isBlocked(pos)) //TODO: refactor - info about visitability from absolute coordinates
				{
					tiles.push_back(pos);
				}
			}

		};
	});

	return tiles;
}

bool CRmgTemplateZone::guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str)
{
	logGlobal->traceStream() << boost::format("Guard object at %s") % object->pos();

	std::vector<int3> tiles = getAccessibleOffsets (gen, object);

	int3 guardTile(-1,-1,-1);

	for (auto tile : tiles)
	{
		//crunching path may fail if center of teh zone is dirrectly over wide object
		if (crunchPath (gen, tile, getPos(), id)) //make sure object is accessible before surrounding it with blocked tiles
		{
			guardTile = tile;
			break;
		}
	}
	if (!guardTile.valid())
	{
		logGlobal->errorStream() << boost::format("Failed to crunch path to object at %s") % object->pos();
		return false;
	}

	if (addMonster (gen, guardTile, str)) //do not place obstacles around unguarded object
	{
		for (auto pos : tiles)
		{
			if (!gen->isFree(pos))
				gen->setOccupied(pos, ETileType::BLOCKED);
		}

		gen->setOccupied (guardTile, ETileType::USED);
	}
	else //allow no guard or other object in front of this object
	{
		for (auto tile : tiles)
			gen->setOccupied (tile, ETileType::FREE);
	}

	return true;
}

ObjectInfo CRmgTemplateZone::getRandomObject (CMapGenerator* gen, ui32 value)
{
	std::vector<std::pair<ui32, ObjectInfo>> tresholds;
	ui32 total = 0;

	ui32 minValue = 0.25f * value;

	//roulette wheel
	for (auto oi : possibleObjects)
	{
		if (oi.value >= minValue && oi.value <= value)
		{
			//TODO: check place for non-removable object
			//problem: we need at least template for the object that does not yet exist
			total += oi.probability;
			tresholds.push_back (std::make_pair (total, oi));
		}
	}

	//TODO: generate pandora box with gold if the value is very high
	if (tresholds.empty())
	{
		ObjectInfo oi;
		if (minValue > 20000) //we don't have object valuable enough
		{
			oi.generateObject = [minValue]() -> CGObjectInstance *
			{
				auto obj = new CGPandoraBox();
				obj->ID = Obj::PANDORAS_BOX;
				obj->subID = 0;
				obj->resources[Res::GOLD] = minValue;
				return obj;
			};
			oi.value = minValue;
			oi.probability = 0;
		}
		else
		{
			oi.generateObject = [gen]() -> CGObjectInstance *
			{
				return nullptr;
			};
			oi.value = 0;
			oi.probability = 0;
		}
		return oi;
	}

	int r = gen->rand.nextInt (1, total);

	for (auto t : tresholds)
	{
		if (r <= t.first)
			return t.second;
	}
	//FIXME: control reaches end of non-void function. Missing return?
}

void CRmgTemplateZone::addAllPossibleObjects (CMapGenerator* gen)
{
	//TODO: move typical objects to config

	ObjectInfo oi;

	static const Res::ERes preciousRes[] = {Res::ERes::CRYSTAL, Res::ERes::GEMS, Res::ERes::MERCURY, Res::ERes::SULFUR};
	for (int i = 0; i < 4; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGResource();
			obj->ID = Obj::RESOURCE;
			obj->subID = static_cast<si32>(preciousRes[i]);
			obj->amount = 0;
			return obj;
		};
		oi.value = 1400;
		oi.probability = 300;
		possibleObjects.push_back (oi);
	}

	static const Res::ERes woodOre[] = {Res::ERes::WOOD, Res::ERes::ORE};
	for (int i = 0; i < 2; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGResource();
			obj->ID = Obj::RESOURCE;
			obj->subID = static_cast<si32>(woodOre[i]);
			obj->amount = 0;
			return obj;
		};
		oi.value = 1400;
		oi.probability = 300;
		possibleObjects.push_back (oi);
	}

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGResource();
		obj->ID = Obj::RESOURCE;
		obj->subID = static_cast<si32>(Res::ERes::GOLD);
		obj->amount = 0;
		return obj;
	};
	oi.value = 750;
	oi.probability = 300;
	possibleObjects.push_back (oi);

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGPickable();
		obj->ID = Obj::TREASURE_CHEST;
		obj->subID = 0;
		return obj;
	};
	oi.value = 1500;
	oi.probability = 1000;
	possibleObjects.push_back (oi);

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGArtifact();
		obj->ID = Obj::RANDOM_TREASURE_ART;
		obj->subID = 0;
		auto a = new CArtifactInstance();
		gen->map->addNewArtifactInstance(a);
		obj->storedArtifact = a;
		return obj;
	};
	oi.value = 2000;
	oi.probability = 150;
	possibleObjects.push_back (oi);

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGArtifact();
		obj->ID = Obj::RANDOM_MINOR_ART;
		obj->subID = 0;
		auto a = new CArtifactInstance();
		gen->map->addNewArtifactInstance(a);
		obj->storedArtifact = a;
		return obj;
	};
	oi.value = 5000;
	oi.probability = 150;
	possibleObjects.push_back (oi);

		oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGArtifact();
		obj->ID = Obj::RANDOM_MAJOR_ART;
		obj->subID = 0;
		auto a = new CArtifactInstance();
		gen->map->addNewArtifactInstance(a);
		obj->storedArtifact = a;
		return obj;
	};
	oi.value = 10000;
	oi.probability = 150;
	possibleObjects.push_back (oi);

	oi.generateObject = [gen]() -> CGObjectInstance *
	{
		auto obj = new CGArtifact();
		obj->ID = Obj::RANDOM_RELIC_ART;
		obj->subID = 0;
		auto a = new CArtifactInstance();
		gen->map->addNewArtifactInstance(a);
		obj->storedArtifact = a;
		return obj;
	};
	oi.value = 20000;
	oi.probability = 150;
	possibleObjects.push_back (oi);

	static const int scrollValues[] = {500, 2000, 3000, 4000, 5000};

	for (int i = 0; i < 5; i++)
	{
		oi.generateObject = [i, gen]() -> CGObjectInstance *
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::SPELL_SCROLL;
			obj->subID = 0;
			std::vector<SpellID> out;

			//TODO: unify with cb->getAllowedSpells?
			for (ui32 i = 0; i < gen->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
			{
				const CSpell *spell = SpellID(i).toSpell();
				if (gen->map->allowedSpell[spell->id] && spell->level == i+1)
				{
					out.push_back(spell->id);
				}
			}
			auto a = CArtifactInstance::createScroll(RandomGeneratorUtil::nextItem(out, gen->rand)->toSpell());
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			return obj;
		};
		oi.value = scrollValues[i];
		oi.probability = 30;
		possibleObjects.push_back (oi);
	}

	//non-removable object for test
	//oi.generateObject = [gen]() -> CGObjectInstance *
	//{
	//	auto obj = new CGMagicWell();
	//	obj->ID = Obj::MAGIC_WELL;
	//	obj->subID = 0;
	//	return obj;
	//};
	//oi.value = 250;
	//oi.probability = 100;
	//possibleObjects.push_back (oi);

	//oi.generateObject = [gen]() -> CGObjectInstance *
	//{
	//	auto obj = new CGObelisk();
	//	obj->ID = Obj::OBELISK;
	//	obj->subID = 0;
	//	return obj;
	//};
	//oi.value = 3500;
	//oi.probability = 200;
	//possibleObjects.push_back (oi);

	//oi.generateObject = [gen]() -> CGObjectInstance *
	//{
	//	auto obj = new CBank();
	//	obj->ID = Obj::CREATURE_BANK;
	//	obj->subID = 5; //naga bank
	//	return obj;
	//};
	//oi.value = 3000;
	//oi.probability = 100;
	//possibleObjects.push_back (oi);
	
}
