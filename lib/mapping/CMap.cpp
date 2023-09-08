/*
 * CMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMap.h"

#include "../CArtHandler.h"
#include "../VCMI_Lib.h"
#include "../CCreatureHandler.h"
#include "../CTownHandler.h"
#include "../CHeroHandler.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../CGeneralTextHandler.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "CMapEditManager.h"
#include "CMapOperation.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

void Rumor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeString("text", text);
}

DisposedHero::DisposedHero() : heroId(0), portrait(255)
{

}

CMapEvent::CMapEvent() : players(0), humanAffected(0), computerAffected(0),
	firstOccurence(0), nextOccurence(0)
{

}

bool CMapEvent::earlierThan(const CMapEvent & other) const
{
	return firstOccurence < other.firstOccurence;
}

bool CMapEvent::earlierThanOrEqual(const CMapEvent & other) const
{
	return firstOccurence <= other.firstOccurence;
}

CCastleEvent::CCastleEvent() : town(nullptr)
{

}

TerrainTile::TerrainTile():
	terType(nullptr),
	terView(0),
	riverType(VLC->riverTypeHandler->getById(River::NO_RIVER)),
	riverDir(0),
	roadType(VLC->roadTypeHandler->getById(Road::NO_ROAD)),
	roadDir(0),
	extTileFlags(0),
	visitable(false),
	blocked(false)
{
}

bool TerrainTile::entrableTerrain(const TerrainTile * from) const
{
	return entrableTerrain(from ? from->terType->isLand() : true, from ? from->terType->isWater() : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	return terType->isPassable()
			&& ((allowSea && terType->isWater())  ||  (allowLand && terType->isLand()));
}

bool TerrainTile::isClear(const TerrainTile * from) const
{
	return entrableTerrain(from) && !blocked;
}

Obj TerrainTile::topVisitableId(bool excludeTop) const
{
	return topVisitableObj(excludeTop) ? topVisitableObj(excludeTop)->ID : Obj(Obj::NO_OBJ);
}

CGObjectInstance * TerrainTile::topVisitableObj(bool excludeTop) const
{
	if(visitableObjects.empty() || (excludeTop && visitableObjects.size() == 1))
		return nullptr;

	if(excludeTop)
		return visitableObjects[visitableObjects.size()-2];

	return visitableObjects.back();
}

EDiggingStatus TerrainTile::getDiggingStatus(const bool excludeTop) const
{
	if(terType->isWater() || !terType->isPassable())
		return EDiggingStatus::WRONG_TERRAIN;

	int allowedBlocked = excludeTop ? 1 : 0;
	if(blockingObjects.size() > allowedBlocked || topVisitableObj(excludeTop))
		return EDiggingStatus::TILE_OCCUPIED;
	else
		return EDiggingStatus::CAN_DIG;
}

bool TerrainTile::hasFavorableWinds() const
{
	return extTileFlags & 128;
}

bool TerrainTile::isWater() const
{
	return terType->isWater();
}

CMap::CMap()
	: checksum(0)
	, grailPos(-1, -1, -1)
	, grailRadius(0)
	, uidCounter(0)
{
	allHeroes.resize(allowedHeroes.size());
	allowedAbilities = VLC->skillh->getDefaultAllowed();
	allowedArtifact = VLC->arth->getDefaultAllowed();
	allowedSpells = VLC->spellh->getDefaultAllowed();
}

CMap::~CMap()
{
	getEditManager()->getUndoManager().clearAll();

	for(auto obj : objects)
		obj.dellNull();

	for(auto quest : quests)
		quest.dellNull();

	resetStaticData();
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	const int zVal = obj->pos.z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->pos.x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->pos.y - fy;
			if(xVal>=0 && xVal < width && yVal>=0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(total || obj->visitableAt(xVal, yVal))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || obj->blockingAt(xVal, yVal))
				{
					curt.blockingObjects -= obj;
					curt.blocked = curt.blockingObjects.size();
				}
			}
		}
	}
}

void CMap::addBlockVisTiles(CGObjectInstance * obj)
{
	const int zVal = obj->pos.z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->pos.x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->pos.y - fy;
			if(xVal>=0 && xVal < width && yVal >= 0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(obj->visitableAt(xVal, yVal))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(obj->blockingAt(xVal, yVal))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
			}
		}
	}
}

void CMap::calculateGuardingGreaturePositions()
{
	int levels = twoLevel ? 2 : 1;
	for(int z = 0; z < levels; z++)
	{
		for(int x = 0; x < width; x++)
		{
			for(int y = 0; y < height; y++)
			{
				guardingCreaturePositions[z][x][y] = guardingCreaturePosition(int3(x, y, z));
			}
		}
	}
}

CGHeroInstance * CMap::getHero(int heroID)
{
	for(auto & elem : heroesOnMap)
		if(elem->subID == heroID)
			return elem;
	return nullptr;
}

bool CMap::isCoastalTile(const int3 & pos) const
{
	//todo: refactoring: extract neighbor tile iterator and use it in GameState
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	if(!isInTheMap(pos))
	{
		logGlobal->error("Coastal check outside of map: %s", pos.toString());
		return false;
	}

	if(isWaterTile(pos))
		return false;

	for(const auto & dir : dirs)
	{
		const int3 hlp = pos + dir;

		if(!isInTheMap(hlp))
			continue;
		const TerrainTile &hlpt = getTile(hlp);
		if(hlpt.isWater())
			return true;
	}

	return false;
}

bool CMap::isInTheMap(const int3 & pos) const
{
	return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < width && pos.y < height && pos.z <= (twoLevel ? 1 : 0);
}

TerrainTile & CMap::getTile(const int3 & tile)
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

const TerrainTile & CMap::getTile(const int3 & tile) const
{
	assert(isInTheMap(tile));
	return terrain[tile.z][tile.x][tile.y];
}

bool CMap::isWaterTile(const int3 &pos) const
{
	return isInTheMap(pos) && getTile(pos).isWater();
}
bool CMap::canMoveBetween(const int3 &src, const int3 &dst) const
{
	const TerrainTile * dstTile = &getTile(dst);
	const TerrainTile * srcTile = &getTile(src);
	return checkForVisitableDir(src, dstTile, dst) && checkForVisitableDir(dst, srcTile, src);
}

bool CMap::checkForVisitableDir(const int3 & src, const TerrainTile * pom, const int3 & dst) const
{
	if (!pom->entrableTerrain()) //rock is never accessible
		return false;
	for(auto * obj : pom->visitableObjects) //checking destination tile
	{
		if(!vstd::contains(pom->blockingObjects, obj)) //this visitable object is not blocking, ignore
			continue;

		if (!obj->appearance->isVisitableFrom(src.x - dst.x, src.y - dst.y))
			return false;
	}
	return true;
}

int3 CMap::guardingCreaturePosition (int3 pos) const
{
	const int3 originalPos = pos;
	// Give monster at position priority.
	if (!isInTheMap(pos))
		return int3(-1, -1, -1);
	const TerrainTile &posTile = getTile(pos);
	if (posTile.visitable)
	{
		for (CGObjectInstance* obj : posTile.visitableObjects)
		{
			if(obj->isBlockedVisitable())
			{
				if (obj->ID == Obj::MONSTER) // Monster
					return pos;
				else
					return int3(-1, -1, -1); //blockvis objects are not guarded by neighbouring creatures
			}
		}
	}

	// See if there are any monsters adjacent.
	bool water = posTile.isWater();

	pos -= int3(1, 1, 0); // Start with top left.
	for (int dx = 0; dx < 3; dx++)
	{
		for (int dy = 0; dy < 3; dy++)
		{
			if (isInTheMap(pos))
			{
				const auto & tile = getTile(pos);
                if (tile.visitable && (tile.isWater() == water))
				{
					for (CGObjectInstance* obj : tile.visitableObjects)
					{
						if (obj->ID == Obj::MONSTER  &&  checkForVisitableDir(pos, &posTile, originalPos)) // Monster being able to attack investigated tile
						{
							return pos;
						}
					}
				}
			}

			pos.y++;
		}
		pos.y -= 3;
		pos.x++;
	}

	return int3(-1, -1, -1);
}

const CGObjectInstance * CMap::getObjectiveObjectFrom(const int3 & pos, Obj type)
{
	for (CGObjectInstance * object : getTile(pos).visitableObjects)
	{
		if (object->ID == type)
			return object;
	}
	// There is weird bug because of which sometimes heroes will not be found properly despite having correct position
	// Try to workaround that and find closest object that we can use

	logGlobal->error("Failed to find object of type %d at %s", static_cast<int>(type), pos.toString());
	logGlobal->error("Will try to find closest matching object");

	CGObjectInstance * bestMatch = nullptr;
	for (CGObjectInstance * object : objects)
	{
		if (object && object->ID == type)
		{
			if (bestMatch == nullptr)
				bestMatch = object;
			else
			{
				if (object->pos.dist2dSQ(pos) < bestMatch->pos.dist2dSQ(pos))
					bestMatch = object;// closer than one we already found
			}
		}
	}
	assert(bestMatch != nullptr); // if this happens - victory conditions or map itself is very, very broken

	logGlobal->error("Will use %s from %s", bestMatch->getObjectName(), bestMatch->pos.toString());
	return bestMatch;
}

void CMap::checkForObjectives()
{
	// NOTE: probably should be moved to MapFormatH3M.cpp
	for (TriggeredEvent & event : triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			switch (cond.condition)
			{
				case EventCondition::HAVE_ARTIFACT:
					event.onFulfill.replaceTextID(VLC->artifacts()->getByIndex(cond.objectType)->getNameTextID());
					break;

				case EventCondition::HAVE_CREATURES:
					event.onFulfill.replaceTextID(VLC->creatures()->getByIndex(cond.objectType)->getNameSingularTextID());
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_RESOURCES:
					event.onFulfill.replaceLocalString(EMetaText::RES_NAMES, cond.objectType);
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_BUILDING:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);
					break;

				case EventCondition::CONTROL:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, static_cast<Obj>(cond.objectType));

					if (cond.object)
					{
						const auto * town = dynamic_cast<const CGTownInstance *>(cond.object);
						if (town)
							event.onFulfill.replaceRawString(town->getNameTranslated());
						const auto * hero = dynamic_cast<const CGHeroInstance *>(cond.object);
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;

				case EventCondition::DESTROY:
					if (isInTheMap(cond.position))
						cond.object = getObjectiveObjectFrom(cond.position, static_cast<Obj>(cond.objectType));

					if (cond.object)
					{
						const auto * hero = dynamic_cast<const CGHeroInstance *>(cond.object);
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;
				case EventCondition::TRANSPORT:
					cond.object = getObjectiveObjectFrom(cond.position, Obj::TOWN);
					break;
				//break; case EventCondition::DAYS_PASSED:
				//break; case EventCondition::IS_HUMAN:
				//break; case EventCondition::DAYS_WITHOUT_TOWN:
				//break; case EventCondition::STANDARD_WIN:

				//TODO: support new condition format
				case EventCondition::HAVE_0:
					break;
				case EventCondition::DESTROY_0:
					break;
				case EventCondition::HAVE_BUILDING_0:
					break;
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
}

void CMap::addNewArtifactInstance(CArtifactInstance * art)
{
	art->setId(static_cast<ArtifactInstanceID>(artInstances.size()));
	artInstances.emplace_back(art);
}

void CMap::eraseArtifactInstance(CArtifactInstance * art)
{
	//TODO: handle for artifacts removed in map editor
	assert(artInstances[art->getId().getNum()] == art);
	artInstances[art->getId().getNum()].dellNull();
}

void CMap::addNewQuestInstance(CQuest* quest)
{
	quest->qid = static_cast<si32>(quests.size());
	quests.emplace_back(quest);
}

void CMap::removeQuestInstance(CQuest * quest)

{
	//TODO: should be called only by map editor.
	//During game, completed quests or quests from removed objects stay forever

	//Shift indexes
	auto iter = std::next(quests.begin(), quest->qid);
	iter = quests.erase(iter);
	for (int i = quest->qid; iter != quests.end(); ++i, ++iter)
	{
		(*iter)->qid = i;
	}
}

void CMap::setUniqueInstanceName(CGObjectInstance * obj)
{
	//this gives object unique name even if objects are removed later

	auto uid = uidCounter++;

	boost::format fmt("%s_%d");
	fmt % obj->typeName % uid;
	obj->instanceName = fmt.str();
}

void CMap::addNewObject(CGObjectInstance * obj)
{
	if(obj->id != ObjectInstanceID(static_cast<si32>(objects.size())))
		throw std::runtime_error("Invalid object instance id");

	if(obj->instanceName.empty())
		throw std::runtime_error("Object instance name missing");

	if (vstd::contains(instanceNames, obj->instanceName))
		throw std::runtime_error("Object instance name duplicated: "+obj->instanceName);

	objects.emplace_back(obj);
	instanceNames[obj->instanceName] = obj;
	addBlockVisTiles(obj);

	//TODO: how about defeated heroes recruited again?

	obj->afterAddToMap(this);
}

void CMap::moveObject(CGObjectInstance * obj, const int3 & pos)
{
	removeBlockVisTiles(obj);
	obj->pos = pos;
	addBlockVisTiles(obj);
}

void CMap::removeObject(CGObjectInstance * obj)
{
	removeBlockVisTiles(obj);
	instanceNames.erase(obj->instanceName);

	//update indeces
	auto iter = std::next(objects.begin(), obj->id.getNum());
	iter = objects.erase(iter);
	for(int i = obj->id.getNum(); iter != objects.end(); ++i, ++iter)
	{
		(*iter)->id = ObjectInstanceID(i);
	}
	
	obj->afterRemoveFromMap(this);

	//TOOD: Clean artifact instances (mostly worn by hero?) and quests related to this object
}

bool CMap::isWaterMap() const
{
	return waterMap;
}

bool CMap::calculateWaterContent()
{
	size_t totalTiles = height * width * levels();
	size_t waterTiles = 0;

	for(auto tile = terrain.origin(); tile < (terrain.origin() + terrain.num_elements()); ++tile) 
	{
		if (tile->isWater())
		{
			waterTiles++;
		}
	}

	if (waterTiles >= totalTiles / 100) //At least 1% of area is water
	{
		waterMap = true;
	}

	return waterMap;
}

void CMap::banWaterContent()
{
	banWaterHeroes();
	banWaterArtifacts();
	banWaterSpells();
	banWaterSkills();
}

void CMap::banWaterSpells()
{
	for (int j = 0; j < allowedSpells.size(); j++)
	{
		if (allowedSpells[j])
		{
			auto* spell = dynamic_cast<const CSpell*>(VLC->spells()->getByIndex(j));
			if (spell->onlyOnWaterMap && !isWaterMap())
			{
				allowedSpells[j] = false;
			}
		}
	}
}

void CMap::banWaterArtifacts()
{
	for (int j = 0; j < allowedArtifact.size(); j++)
	{
		if (allowedArtifact[j])
		{
			auto* art = dynamic_cast<const CArtifact*>(VLC->artifacts()->getByIndex(j));
			if (art->onlyOnWaterMap && !isWaterMap())
			{
				allowedArtifact[j] = false;
			}
		}
	}
}

void CMap::banWaterSkills()
{
	for (int j = 0; j < allowedAbilities.size(); j++)
	{
		if (allowedAbilities[j])
		{
			auto* skill = dynamic_cast<const CSkill*>(VLC->skills()->getByIndex(j));
			if (skill->onlyOnWaterMap && !isWaterMap())
			{
				allowedAbilities[j] = false;
			}
		}
	}
}

void CMap::banWaterHeroes()
{
	for (int j = 0; j < allowedHeroes.size(); j++)
	{
		if (allowedHeroes[j])
		{
			auto* h = dynamic_cast<const CHero*>(VLC->heroTypes()->getByIndex(j));
			if ((h->onlyOnWaterMap && !isWaterMap()) || (h->onlyOnMapWithoutWater && isWaterMap()))
			{
				banHero(HeroTypeID(j));
			}
		}
	}
}

void CMap::banHero(const HeroTypeID & id)
{
	allowedHeroes.at(id) = false;
}

void CMap::initTerrain()
{
	terrain.resize(boost::extents[levels()][width][height]);
	guardingCreaturePositions.resize(boost::extents[levels()][width][height]);
}

CMapEditManager * CMap::getEditManager()
{
	if(!editManager) editManager = std::make_unique<CMapEditManager>(this);
	return editManager.get();
}

void CMap::resetStaticData()
{
	CGKeys::reset();
	CGMagi::reset();
	CGObelisk::reset();
	CGTownInstance::reset();
}

VCMI_LIB_NAMESPACE_END
