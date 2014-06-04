
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

CRmgTemplateZone::CRmgTemplateZone() : id(0), type(ETemplateZoneType::PLAYER_START), size(1),
	townsAreSameType(false), matchTerrainToTown(true)
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
	static const ETerrainType::EETerrainType allowedTerTypes[] = { ETerrainType::DIRT, ETerrainType::SAND, ETerrainType::GRASS, ETerrainType::SNOW,
												   ETerrainType::SWAMP, ETerrainType::ROUGH, ETerrainType::SUBTERRANEAN, ETerrainType::LAVA };
	for(auto & allowedTerType : allowedTerTypes) terTypes.insert(allowedTerType);
	return terTypes;
}

boost::optional<TRmgTemplateZoneId> CRmgTemplateZone::getTerrainTypeLikeZone() const
{
	return terrainTypeLikeZone;
}

void CRmgTemplateZone::setTerrainTypeLikeZone(boost::optional<TRmgTemplateZoneId> value)
{
	terrainTypeLikeZone = value;
}

boost::optional<TRmgTemplateZoneId> CRmgTemplateZone::getTownTypeLikeZone() const
{
	return townTypeLikeZone;
}

void CRmgTemplateZone::setTownTypeLikeZone(boost::optional<TRmgTemplateZoneId> value)
{
	townTypeLikeZone = value;
}

void CRmgTemplateZone::addConnection(TRmgTemplateZoneId otherZone)
{
	connections.push_back (otherZone);
}

std::vector<TRmgTemplateZoneId> CRmgTemplateZone::getConnections() const
{
	return connections;
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

bool CRmgTemplateZone::crunchPath (CMapGenerator* gen, const int3 &src, const int3 &dst, TRmgTemplateZoneId zone)
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
			break;

		auto lastDistance = distance;
		gen->foreach_neighbour (currentPos, [this, gen, &currentPos, dst, &distance, &result, &end](int3 &pos)
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

void CRmgTemplateZone::addMonster(CMapGenerator* gen, int3 &pos, si32 strength)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426

	int zoneMonsterStrength = 0; //TODO: range -1..1 based on template settings
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
		return; //no guard at all

	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	while (true)
	{
		creId = VLC->creh->pickRandomMonster(gen->rand);
		auto cre = VLC->creh->creatures[creId];
		if ((cre->AIValue * (cre->ammMin + cre->ammMax) / 2 < strength) && (strength < cre->AIValue * 100)) //at leats one full monster. size between minimum size of given stack and 100
		{
			amount = strength / cre->AIValue;
			if (amount >= 4)
				amount *= gen->rand.nextDouble(0.75, 1.25);
			break;
		}
	}

	auto guard = new CGCreature();
	guard->ID = Obj::MONSTER;
	guard->subID = creId;
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);

	placeObject(gen, guard, pos);
}

bool CRmgTemplateZone::createTreasurePile (CMapGenerator* gen, int3 &pos)
{
	//TODO: read treasure values from template
	const int maxValue = 5000;
	const int minValue = 1500;

	static const Res::ERes woodOre[] = {Res::ERes::WOOD, Res::ERes::ORE};
	static const Res::ERes preciousRes[] = {Res::ERes::CRYSTAL, Res::ERes::GEMS, Res::ERes::MERCURY, Res::ERes::SULFUR};
	static auto res_gen = gen->rand.getIntRange(Res::ERes::WOOD, Res::ERes::GOLD);

	int currentValue = 0;
	CGObjectInstance * object = nullptr;
	while (currentValue < minValue)
	{
		int remaining = maxValue - currentValue;
		int nextValue = gen->rand.nextInt (0.25f * remaining, remaining);

		if (nextValue >= 20000)
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::RANDOM_RELIC_ART;
			obj->subID = 0;
			auto a = new CArtifactInstance(); //TODO: probably some refactoring could help here
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			object = obj;
			currentValue += 20000;
		}
		else if (nextValue >= 10000)
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::RANDOM_MAJOR_ART;
			obj->subID = 0;
			auto a = new CArtifactInstance();
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			object = obj;
			currentValue += 10000;
		}
		else if (nextValue >= 5000)
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::RANDOM_MINOR_ART;
			obj->subID = 0;
			auto a = new CArtifactInstance();
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			object = obj;
			currentValue += 5000;
		}
		else if (nextValue >= 2000)
		{
			auto obj = new CGArtifact();
			obj->ID = Obj::RANDOM_TREASURE_ART;
			obj->subID = 0;
			auto a = new CArtifactInstance();
			gen->map->addNewArtifactInstance(a);
			obj->storedArtifact = a;
			object = obj;
			currentValue += 2000;
		}
		else if (nextValue >= 1500)
		{
			auto obj = new CGPickable();
			obj->ID = Obj::TREASURE_CHEST;
			obj->subID = 0;
			object = obj;
			currentValue += 1500;
		}
		else if (nextValue >= 1400)
		{
			auto obj = new CGResource();
			auto restype = static_cast<Res::ERes>(preciousRes[gen->rand.nextInt (0,3)]); //TODO: how about dedicated function to pick random element of array?
			obj->ID = Obj::RESOURCE;
			obj->subID = static_cast<si32>(restype);
			obj->amount = 0;
			object = obj;
			currentValue += 1400;
		}
		else if (nextValue >= 1000)
		{
			auto obj = new CGResource();
			auto restype = static_cast<Res::ERes>(woodOre[gen->rand.nextInt (0,1)]);
			obj->ID = Obj::RESOURCE;
			obj->subID = static_cast<si32>(restype);
			obj->amount = 0;
			object = obj;
			currentValue += 1000;
		}
		else if (nextValue >= 750)
		{
			auto obj = new CGResource();
			obj->ID = Obj::RESOURCE;
			obj->subID = static_cast<si32>(Res::ERes::GOLD);
			obj->amount = 0;
			object = obj;
			currentValue += 750;
		}
		else //no possible treasure left (should not happen)
			break;

		//TODO: generate actual zone and not just all objects on a pile
		placeObject(gen, object, pos);
	}
	if (object)
	{
		guardObject (gen, object, currentValue);
		return true;
	}
	else //we did not place eveyrthing successfully
		return false;
}

bool CRmgTemplateZone::fill(CMapGenerator* gen)
{
	int townId = 0;

	if ((type == ETemplateZoneType::CPU_START) || (type == ETemplateZoneType::PLAYER_START))
	{
		logGlobal->infoStream() << "Preparing playing zone";
		int player_id = *owner - 1;
		auto & playerInfo = gen->map->players[player_id];
		if (playerInfo.canAnyonePlay())
		{
			PlayerColor player(player_id);
			auto  town = new CGTownInstance();
			town->ID = Obj::TOWN;
			townId = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if(townId == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				townId = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand); // all possible towns, skip neutral

			town->subID = townId;
			town->tempOwner = player;
			town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);
			
			placeObject(gen, town, getPos() + town->getVisitableOffset()); //towns are big objects and should be centered around visitable position
			logGlobal->traceStream() << "Placed object";

			logGlobal->traceStream() << "Fill player info " << player_id;
			auto & playerInfo = gen->map->players[player_id];
			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(town->subID);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos - int3(2, 0, 0);
			playerInfo.generateHeroAtMainTown = true;

			//requiredObjects.push_back(town);

			std::vector<Res::ERes> required_mines;
			required_mines.push_back(Res::ERes::WOOD);
			required_mines.push_back(Res::ERes::ORE);

			for(const auto res : required_mines)
			{			
				auto mine = new CGMine();
				mine->ID = Obj::MINE;
				mine->subID = static_cast<si32>(res);
				mine->producedResource = res;
				mine->producedQuantity = mine->defaultResProduction();
				addRequiredObject(mine);
			}
		}
		else
		{			
			type = ETemplateZoneType::TREASURE;
			townId = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand);
			logGlobal->infoStream() << "Skipping this zone cause no player";
		}
	}
	else //no player
	{
		townId = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand);
	}
	//paint zone with matching terrain
	std::vector<int3> tiles;
	for (auto tile : tileinfo)
	{
		tiles.push_back (tile);
	}
	gen->editManager->getTerrainSelection().setSelection(tiles);
	gen->editManager->drawTerrain(VLC->townh->factions[townId]->nativeTerrain, &gen->rand);

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
		if (obj.second)
		{
			guardObject (gen, obj.first, obj.second);
		}
	}
	const double res_mindist = 5;

	//TODO: just placeholder to chekc for possible locations
	auto obj = new CGResource();
	obj->ID = Obj::RESOURCE;
	obj->subID = static_cast<si32>(Res::ERes::GOLD);
	obj->amount = 0;
	do {
		
		int3 pos;
		if ( ! findPlaceForObject(gen, obj, res_mindist, pos))		
		{
			delete obj;
			break;
		}
		createTreasurePile (gen, pos);

	} while(true);

	auto sel = gen->editManager->getTerrainSelection();
	sel.clearSelection();
	for (auto tile : tileinfo)
	{
		//test code - block all the map to show paths clearly
		//if (gen->isPossible(tile))
		//	gen->setOccupied(tile, ETileType::BLOCKED);

		if (gen->shouldBeBlocked(tile)) //fill tiles that should be blocked with obstacles
		{
			auto obj = new CGObjectInstance();
			obj->ID = static_cast<Obj>(130);
			obj->subID = 0;
			placeObject(gen, obj, tile);
		}
	}
	//logGlobal->infoStream() << boost::format("Filling %d with ROCK") % sel.getSelectedItems().size();
	//gen->editManager->drawTerrain(ETerrainType::ROCK, &gen->gen);
	logGlobal->infoStream() << boost::format ("Zone %d filled successfully") %id;
	return true;
}

bool CRmgTemplateZone::findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tiles
	if (obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->dobjinfo->pickCandidates(obj->ID, obj->subID, gen->map->getTile(getPos()).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %obj->ID %obj->subID %pos));
	
		obj->appearance = templates.front();
	}

	//si32 min_dist = sqrt(tileinfo.size()/density);
	int best_distance = 0;
	bool result = false;
	si32 w = gen->map->width;
	si32 h = gen->map->height; 

	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;
	for(auto tile : tileinfo)
	{
		auto ti = gen->getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if ((tile.x < 3) || (w - tile.x < 3) || (tile.y < 3) || (h - tile.y < 3))
			continue;
		if (gen->isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			bool allTilesAvailable = true;
			for (auto blockingTile : obj->getBlockedOffsets())
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
		auto templates = VLC->dobjinfo->pickCandidates(object->ID, object->subID, gen->map->getTile(pos).terType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %object->ID %object->subID %pos));
	
		object->appearance = templates.front();
	}

	gen->map->addBlockVisTiles(object);
	gen->editManager->insertObject(object, pos);
	logGlobal->traceStream() << boost::format ("Successfully inserted object (%d,%d) at pos %s") %object->ID %object->subID %pos();
}

void CRmgTemplateZone::placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos)
{
	logGlobal->traceStream() << boost::format("Inserting object at %d %d") % pos.x % pos.y;

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
		si32 d = pos.dist2d(tile);
		gen->setNearestObjectDistance(tile, std::min(d, gen->getNearestObjectDistance(tile)));
	}
}

bool CRmgTemplateZone::guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str)
{
	
	logGlobal->traceStream() << boost::format("Guard object at %d %d") % object->pos.x % object->pos.y;
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;
	gen->foreach_neighbour(visitable, [&](int3& pos) 
	{
		logGlobal->traceStream() << boost::format("Block at %d %d") % pos.x % pos.y;
		if (gen->isPossible(pos))
		{
			tiles.push_back(pos);
			gen->setOccupied(pos, ETileType::BLOCKED);
		};
	});
	if ( ! tiles.size())
	{		
		logGlobal->infoStream() << "Failed";
		return false;
	}
	auto guard_tile = *RandomGeneratorUtil::nextItem(tiles, gen->rand);
	gen->setOccupied (guard_tile, ETileType::USED);

	addMonster (gen, guard_tile, str);

	return true;
}
