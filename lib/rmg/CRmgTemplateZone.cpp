
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

CRmgTemplateZone::CTileInfo::CTileInfo():nearestObjectDistance(INT_MAX), obstacle(false), occupied(false), terrain(ETerrainType::WRONG) 
{

}

int CRmgTemplateZone::CTileInfo::getNearestObjectDistance() const
{
	return nearestObjectDistance;
}

void CRmgTemplateZone::CTileInfo::setNearestObjectDistance(int value)
{
	nearestObjectDistance = std::max(0, value); //never negative (or unitialized)
}

bool CRmgTemplateZone::CTileInfo::isObstacle() const
{
	return obstacle;
}

void CRmgTemplateZone::CTileInfo::setObstacle(bool value)
{
	obstacle = value;
}

bool CRmgTemplateZone::CTileInfo::isOccupied() const
{
	return occupied;
}

void CRmgTemplateZone::CTileInfo::setOccupied(bool value)
{
	occupied = value;
}

ETerrainType CRmgTemplateZone::CTileInfo::getTerrainType() const
{
	return terrain;
}

void CRmgTemplateZone::CTileInfo::setTerrainType(ETerrainType value)
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

bool CRmgTemplateZone::pointIsIn(int x, int y)
{
	int i, j;
	bool c = false;
	int nvert = shape.size();
	for (i = 0, j = nvert-1; i < nvert; j = i++) {
		if ( ((shape[i].y>y) != (shape[j].y>y)) &&
			(x < (shape[j].x-shape[i].x) * (y-shape[i].y) / (shape[j].y-shape[i].y) + shape[i].x) )
			c = !c;
	}
	return c;
}

void CRmgTemplateZone::setShape(std::vector<int3> shape)
{
	int z = -1;
	si32 minx = INT_MAX;
	si32 maxx = -1;
	si32 miny = INT_MAX;
	si32 maxy = -1;
	for(auto &point : shape)
	{
		if (z == -1)
			z = point.z;
		if (point.z != z)
			throw rmgException("Zone shape points should lie on same z.");
		minx = std::min(minx, point.x);
		maxx = std::max(maxx, point.x);
		miny = std::min(miny, point.y);
		maxy = std::max(maxy, point.y);
	}
	this->shape = shape;
	for(int x = minx; x <= maxx; ++x)
	{
		for(int y = miny; y <= maxy; ++y)
		{
			if (pointIsIn(x, y))
			{
				tileinfo[int3(x,y,z)] = CTileInfo();
			}
		}
	}
}

int3 CRmgTemplateZone::getCenter()
{
	si32 cx = 0;
	si32 cy = 0;
	si32 area = 0;
	si32 sz = shape.size();
	//include last->first too
	for(si32 i = 0, j = sz-1; i < sz; j = i++) {
		si32 sf = (shape[i].x * shape[j].y - shape[j].x * shape[i].y);
		cx += (shape[i].x + shape[j].x) * sf;
		cy += (shape[i].y + shape[j].y) * sf;
		area += sf;
	}
	area /= 2;
	return int3(std::abs(cx/area/6), std::abs(cy/area/6), shape[0].z);
}

bool CRmgTemplateZone::fill(CMapGenerator* gen)
{
	std::vector<CGObjectInstance*> required_objects;
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
			int townId = gen->mapGenOptions->getPlayersSettings().find(player)->second.getStartingTown();

			if(townId == CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
				townId = *RandomGeneratorUtil::nextItem(VLC->townh->getAllowedFactions(), gen->rand); // all possible towns, skip neutral

			town->subID = townId;
			town->tempOwner = player;
			town->builtBuildings.insert(BuildingID::FORT);
			town->builtBuildings.insert(BuildingID::DEFAULT);
			
			placeObject(gen, town, getCenter());
			logGlobal->infoStream() << "Placed object";

			logGlobal->infoStream() << "Fill player info " << player_id;
			auto & playerInfo = gen->map->players[player_id];
			// Update player info
			playerInfo.allowedFactions.clear();
			playerInfo.allowedFactions.insert(town->subID);
			playerInfo.hasMainTown = true;
			playerInfo.posOfMainTown = town->pos - int3(2, 0, 0);
			playerInfo.generateHeroAtMainTown = true;

			//required_objects.push_back(town);

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
				required_objects.push_back(mine);
			}
		}
		else
		{			
			type = ETemplateZoneType::TREASURE;
			logGlobal->infoStream() << "Skipping this zone cause no player";
		}
	}
	logGlobal->infoStream() << "Creating required objects";
	for(const auto &obj : required_objects)
	{
		int3 pos;
		logGlobal->infoStream() << "Looking for place";
		if ( ! findPlaceForObject(gen, obj, 3, pos))		
		{
			logGlobal->errorStream() << boost::format("Failed to fill zone %d due to lack of space") %id;
			//TODO CLEANUP!
			return false;
		}
		logGlobal->infoStream() << "Place found";

		placeObject(gen, obj, pos);
	}
	std::vector<CGObjectInstance*> guarded_objects;
	static auto res_gen = gen->rand.getIntRange(Res::ERes::WOOD, Res::ERes::GOLD);
	const double res_mindist = 5;
	do {
		auto obj = new CGResource();
		auto restype = static_cast<Res::ERes>(res_gen());
		obj->ID = Obj::RESOURCE;
		obj->subID = static_cast<si32>(restype);
		obj->amount = 0;
		
		int3 pos;
		if ( ! findPlaceForObject(gen, obj, res_mindist, pos))		
		{
			delete obj;
			break;
		}
		placeObject(gen, obj, pos);

		if ((restype != Res::ERes::WOOD) && (restype != Res::ERes::ORE))
		{
			guarded_objects.push_back(obj);
		}
	} while(true);

	for(const auto &obj : guarded_objects)
	{
		if ( ! guardObject(gen, obj, 500))
		{
			//TODO, DEL obj from map
		}
	}

	auto sel = gen->editManager->getTerrainSelection();
	sel.clearSelection();
	for(auto it = tileinfo.begin(); it != tileinfo.end(); ++it)
	{
		if (it->second.isObstacle())
		{
			auto obj = new CGObjectInstance();
			obj->ID = static_cast<Obj>(130);
			obj->subID = 0;
			placeObject(gen, obj, it->first);
		}
	}
	//logGlobal->infoStream() << boost::format("Filling %d with ROCK") % sel.getSelectedItems().size();
	//gen->editManager->drawTerrain(ETerrainType::ROCK, &gen->gen);
	logGlobal->infoStream() << boost::format ("Zone %d filled successfully") %id;
	return true;
}

bool CRmgTemplateZone::findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//si32 min_dist = sqrt(tileinfo.size()/density);
	int best_distance = 0;
	bool result = false;
	si32 w = gen->map->width;
	si32 h = gen->map->height; 
	auto ow = obj->getWidth();
	auto oh = obj->getHeight();
	//logGlobal->infoStream() << boost::format("Min dist for density %f is %d") % density % min_dist;
	for(auto it = tileinfo.begin(); it != tileinfo.end(); ++it)
	{
		auto &ti = it->second;
		auto p = it->first;
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if ((p.x < 3) || (w - p.x < 3) || (p.y < 3) || (h - p.y < 3))
			continue;
		if (!ti.isOccupied() && !ti.isObstacle()  && (dist >= min_dist) && (dist > best_distance))
		{
			best_distance = dist;
			pos = p;
			result = true;
		}
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

	auto templates = VLC->dobjinfo->pickCandidates(object->ID, object->subID, gen->map->getTile(pos).terType);
	if (templates.empty())
		throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") %object->ID %object->subID %pos));
	
	object->appearance = templates.front();
	gen->map->addBlockVisTiles(object);
	gen->editManager->insertObject(object, pos);
	logGlobal->traceStream() << boost::format ("Successfully inserted object (%d,%d) at pos %s") %object->ID %object->subID %pos();
}

void CRmgTemplateZone::placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos)
{
	logGlobal->infoStream() << boost::format("Inserting object at %d %d") % pos.x % pos.y;

	checkAndPlaceObject (gen, object, pos);

	auto points = object->getBlockedPos();
	if (object->isVisitable())
		points.emplace(pos + object->getVisitableOffset());
	points.emplace(pos);
	for(auto const &p : points)
	{		
		if (tileinfo.find(pos + p) != tileinfo.end())
		{
			tileinfo[pos + p].setOccupied(true);
		}
	}
	for(auto it = tileinfo.begin(); it != tileinfo.end(); ++it)
	{		
		si32 d = pos.dist2d(it->first);
		it->second.setNearestObjectDistance(std::min(d, it->second.getNearestObjectDistance()));
	}
}

bool CRmgTemplateZone::guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str)
{
	
	logGlobal->infoStream() << boost::format("Guard object at %d %d") % object->pos.x % object->pos.y;
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;
	for(int i = -1; i < 2; ++i)
	{
		for(int j = -1; j < 2; ++j)
		{
			auto it = tileinfo.find(visitable + int3(i, j, 0));
			if (it != tileinfo.end())
			{
				logGlobal->infoStream() << boost::format("Block at %d %d") % it->first.x % it->first.y;
				if ( ! it->second.isOccupied() &&  ! it->second.isObstacle())
				{
					tiles.push_back(it->first);
					it->second.setObstacle(true);
				}
			}
		}
	}
	if ( ! tiles.size())
	{		
		logGlobal->infoStream() << "Failed";
		return false;
	}
	auto guard_tile = *RandomGeneratorUtil::nextItem(tiles, gen->rand);
	tileinfo[guard_tile].setObstacle(false);
	auto guard = new CGCreature();
	guard->ID = Obj::RANDOM_MONSTER;
	guard->subID = 0;
	auto  hlp = new CStackInstance();
	hlp->count = 10;
	//type will be set during initialization
	guard->putStack(SlotID(0), hlp);

	checkAndPlaceObject(gen, guard, guard_tile);
	return true;
}
