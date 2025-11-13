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

#include "CMapEditManager.h"
#include "CMapOperation.h"
#include "CCastleEvent.h"

#include "../CCreatureHandler.h"
#include "../CSkillHandler.h"
#include "../GameLibrary.h"
#include "../GameSettings.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"

#include "../bonuses/Limiters.h"
#include "../callback/IGameInfoCallback.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../spells/CSpellHandler.h"
#include "../texts/CGeneralTextHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void Rumor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeStruct("text", text);
}

CMapEvent::CMapEvent()
	: humanAffected(false)
	, computerAffected(false)
	, firstOccurrence(0)
	, nextOccurrence(0)
{

}

bool CMapEvent::occursToday(int currentDay) const
{
	if (currentDay == firstOccurrence + 1)
		return true;

	if (nextOccurrence == 0)
		return false;

	if (currentDay < firstOccurrence)
		return false;

	return (currentDay - firstOccurrence - 1) % nextOccurrence == 0;
}

bool CMapEvent::affectsPlayer(PlayerColor color, bool isHuman) const
{
	if (players.count(color) == 0)
		return false;

	if (!isHuman && !computerAffected)
		return false;

	if (isHuman && !humanAffected)
		return false;

	return true;
}

void CMapEvent::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("name", name);
	handler.serializeStruct("message", message);
	if (!handler.saving && handler.getCurrent()["players"].isNumber())
	{
		// compatibility for old maps
		int playersMask = 0;
		handler.serializeInt("players", playersMask);
		for (int i = 0; i < 8; ++i)
			if ((playersMask & (1 << i)) != 0)
				players.insert(PlayerColor(i));
	}
	else
	{
		handler.serializeIdArray("players", players);
	}
	handler.serializeInt("humanAffected", humanAffected);
	handler.serializeInt("computerAffected", computerAffected);
	handler.serializeInt("firstOccurrence", firstOccurrence);
	handler.serializeInt("nextOccurrence", nextOccurrence);
	resources.serializeJson(handler, "resources");

	auto deletedObjects = handler.enterArray("deletedObjectsInstances");
	deletedObjects.serializeArray(deletedObjectsInstances);
}

void CCastleEvent::serializeJson(JsonSerializeFormat & handler)
{
	CMapEvent::serializeJson(handler);

	// TODO: handler.serializeIdArray("buildings", buildings);
	{
		std::vector<BuildingID> temp(buildings.begin(), buildings.end());
		auto a = handler.enterArray("buildings");
		a.syncSize(temp);
		for(int i = 0; i < temp.size(); ++i)
		{
			int buildingID = temp[i].getNum();
			a.serializeInt(i, buildingID);
			buildings.insert(buildingID);
		}
	}

	{
		auto a = handler.enterArray("creatures");
		a.syncSize(creatures);
		for(int i = 0; i < creatures.size(); ++i)
			a.serializeInt(i, creatures[i]);
	}
}

TerrainTile::TerrainTile():
	riverType(River::NO_RIVER),
	roadType(Road::NO_ROAD),
	terView(0),
	riverDir(0),
	roadDir(0),
	extTileFlags(0)
{
}

bool TerrainTile::isClear(const TerrainTile * from) const
{
	return entrableTerrain(from) && !blocked();
}

ObjectInstanceID TerrainTile::topVisitableObj(bool excludeTop) const
{
	if(visitableObjects.empty() || (excludeTop && visitableObjects.size() == 1))
		return {};

	if(excludeTop)
		return visitableObjects[visitableObjects.size()-2];

	return visitableObjects.back();
}

EDiggingStatus TerrainTile::getDiggingStatus(const bool excludeTop) const
{
	if(isWater() || !getTerrain()->isPassable())
		return EDiggingStatus::WRONG_TERRAIN;

	int allowedBlocked = excludeTop ? 1 : 0;
	if(blockingObjects.size() > allowedBlocked || topVisitableObj(excludeTop).hasValue())
		return EDiggingStatus::TILE_OCCUPIED;
	else
		return EDiggingStatus::CAN_DIG;
}

CMap::CMap(IGameInfoCallback * cb)
	: GameCallbackHolder(cb)
	, grailPos(-1, -1, -1)
	, grailRadius(0)
	, waterMap(false)
	, uidCounter(0)
{
	heroesPool.resize(LIBRARY->heroh->size());
	allowedAbilities = LIBRARY->skillh->getDefaultAllowed();
	allowedArtifact = LIBRARY->arth->getDefaultAllowed();
	allowedSpells = LIBRARY->spellh->getDefaultAllowed();

	gameSettings = std::make_unique<GameSettings>();
	gameSettings->loadBase(LIBRARY->settingsHandler->getFullConfig());
}

CMap::~CMap() = default;

void CMap::hideObject(CGObjectInstance * obj)
{
	const int zVal = obj->anchorPos().z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->anchorPos().x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->anchorPos().y - fy;
			if(xVal>=0 && xVal < width && yVal>=0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				curt.visitableObjects -= obj->id;
				curt.blockingObjects -= obj->id;
			}
		}
	}
}

void CMap::showObject(CGObjectInstance * obj)
{
	const int zVal = obj->anchorPos().z;
	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		int xVal = obj->anchorPos().x - fx;
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int yVal = obj->anchorPos().y - fy;
			if(xVal>=0 && xVal < width && yVal >= 0 && yVal < height)
			{
				TerrainTile & curt = terrain[zVal][xVal][yVal];
				if(obj->visitableAt(int3(xVal, yVal, zVal)))
				{
					assert(!vstd::contains(curt.visitableObjects, obj->id));
					curt.visitableObjects.push_back(obj->id);
				}

				if(obj->blockingAt(int3(xVal, yVal, zVal)))
				{
					assert(!vstd::contains(curt.blockingObjects, obj->id));
					curt.blockingObjects.push_back(obj->id);
				}
			}
		}
	}
}

void CMap::calculateGuardingGreaturePositions()
{
	for(int z = 0; z < levels(); z++)
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

CGHeroInstance * CMap::getHero(HeroTypeID heroID)
{
	for (const auto & objectID : heroesOnMap)
	{
		const auto hero = std::dynamic_pointer_cast<CGHeroInstance>(objects.at(objectID.getNum()));

		if (hero->getHeroTypeID() == heroID)
			return hero.get();
	}
	return nullptr;
}

const CGHeroInstance * CMap::getHero(HeroTypeID heroID) const
{
	for (const auto & objectID : heroesOnMap)
	{
		const auto hero = std::dynamic_pointer_cast<CGHeroInstance>(objects.at(objectID.getNum()));

		if (hero->getHeroTypeID() == heroID)
			return hero.get();
	}
	return nullptr;
}

bool CMap::isCoastalTile(const int3 & pos) const
{
	//todo: refactoring: extract neighbour tile iterator and use it in GameState
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	if(!isInTheMap(pos))
	{
		logGlobal->error("Coastal check outside of map: %s", pos.toString());
		return false;
	}

	if(getTile(pos).isWater())
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
	for(const auto & objID : pom->visitableObjects) //checking destination tile
	{
		if(!vstd::contains(pom->blockingObjects, objID)) //this visitable object is not blocking, ignore
			continue;

		if (!getObject(objID)->appearance->isVisitableFrom(src.x - dst.x, src.y - dst.y))
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
	if (posTile.visitable())
	{
		for (const auto & objID : posTile.visitableObjects)
		{
			const auto * object = getObject(objID);
			if (object->ID == Obj::MONSTER)
				return pos;
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
				if (tile.visitable() && (tile.isWater() == water))
				{
					for (const auto & objID : tile.visitableObjects)
					{
						const auto * object = getObject(objID);
						if (object->ID == Obj::MONSTER  &&  checkForVisitableDir(pos, &posTile, originalPos)) // Monster being able to attack investigated tile
							return pos;
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
	for(const auto & objID : getTile(pos).visitableObjects)
	{
		const auto * object = getObject(objID);
		if (object->ID == type)
			return object;
	}
	// There is weird bug because of which sometimes heroes will not be found properly despite having correct position
	// Try to workaround that and find closest object that we can use

	logGlobal->error("Failed to find object of type %d at %s", type.getNum(), pos.toString());
	logGlobal->error("Will try to find closest matching object");

	CGObjectInstance * bestMatch = nullptr;
	for (const auto & object : objects)
	{
		if (object && object->ID == type)
		{
			if (bestMatch == nullptr)
				bestMatch = object.get();
			else
			{
				if (object->anchorPos().dist2dSQ(pos) < bestMatch->anchorPos().dist2dSQ(pos))
					bestMatch = object.get();// closer than one we already found
			}
		}
	}
	assert(bestMatch != nullptr); // if this happens - victory conditions or map itself is very, very broken

	logGlobal->error("Will use %s from %s", bestMatch->getObjectName(), bestMatch->anchorPos().toString());
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
					event.onFulfill.replaceTextID(cond.objectType.as<ArtifactID>().toEntity(LIBRARY)->getNameTextID());
					break;

				case EventCondition::HAVE_CREATURES:
					event.onFulfill.replaceTextID(cond.objectType.as<CreatureID>().toEntity(LIBRARY)->getNameSingularTextID());
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_RESOURCES:
					event.onFulfill.replaceName(cond.objectType.as<GameResID>());
					event.onFulfill.replaceNumber(cond.value);
					break;

				case EventCondition::HAVE_BUILDING:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, Obj::TOWN)->id;
					break;

				case EventCondition::CONTROL:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, cond.objectType.as<MapObjectID>())->id;

					if (cond.objectID != ObjectInstanceID::NONE)
					{
						const auto * town = dynamic_cast<const CGTownInstance *>(objects[cond.objectID].get());
						if (town)
							event.onFulfill.replaceRawString(town->getNameTranslated());
						const auto * hero = dynamic_cast<const CGHeroInstance *>(objects[cond.objectID].get());
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;

				case EventCondition::DESTROY:
					if (isInTheMap(cond.position))
						cond.objectID = getObjectiveObjectFrom(cond.position, cond.objectType.as<MapObjectID>())->id;

					if (cond.objectID != ObjectInstanceID::NONE)
					{
						const auto * hero = dynamic_cast<const CGHeroInstance *>(objects[cond.objectID].get());
						if (hero)
							event.onFulfill.replaceRawString(hero->getNameTranslated());
					}
					break;
				case EventCondition::TRANSPORT:
					cond.objectID = getObjectiveObjectFrom(cond.position, Obj::TOWN)->id;
					break;
				//break; case EventCondition::DAYS_PASSED:
				//break; case EventCondition::IS_HUMAN:
				//break; case EventCondition::DAYS_WITHOUT_TOWN:
				//break; case EventCondition::STANDARD_WIN:
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
}

void CMap::eraseArtifactInstance(ArtifactInstanceID art)
{
	//TODO: handle for artifacts removed in map editor
	artInstances[art.getNum()] = nullptr;
}

void CMap::moveArtifactInstance(
	CArtifactSet & srcSet, const ArtifactPosition & srcSlot,
	CArtifactSet & dstSet, const ArtifactPosition & dstSlot)
{
	auto art = srcSet.getArt(srcSlot);
	removeArtifactInstance(srcSet, srcSlot);
	putArtifactInstance(dstSet, art->getId(), dstSlot);
}

void CMap::putArtifactInstance(CArtifactSet & set, ArtifactInstanceID artID, const ArtifactPosition & slot)
{
	auto artifact = artInstances.at(artID.getNum());

	artifact->addPlacementMap(set.putArtifact(slot, artifact.get()));
}

void CMap::removeArtifactInstance(CArtifactSet & set, const ArtifactPosition & slot)
{
	ArtifactInstanceID artID = set.getArtID(slot);
	auto artifact = artInstances.at(artID.getNum());
	assert(artifact);
	set.removeArtifact(slot);
	CArtifactSet::ArtPlacementMap partsMap;
	for(auto & part : artifact->getPartsInfo())
	{
		if(part.slot != ArtifactPosition::PRE_FIRST)
			partsMap.try_emplace(part.getArtifact(), ArtifactPosition::PRE_FIRST);
	}
	artifact->addPlacementMap(partsMap);
}

void CMap::generateUniqueInstanceName(CGObjectInstance * target)
{
	//this gives object unique name even if objects are removed later
	auto uid = uidCounter++;

	boost::format fmt("%s_%d");
	fmt % target->getTypeName() % uid;
	target->instanceName = fmt.str();
}

void CMap::addNewObject(std::shared_ptr<CGObjectInstance> obj)
{
	if (!obj->id.hasValue())
		obj->id = ObjectInstanceID(objects.size());

	if(obj->id != ObjectInstanceID(objects.size()) && objects.at(obj->id.getNum()) != nullptr)
		throw std::runtime_error("Invalid object instance id");

	if(obj->instanceName.empty())
		throw std::runtime_error("Object instance name missing");

	if (vstd::contains(instanceNames, obj->instanceName))
		throw std::runtime_error("Object instance name duplicated: "+obj->instanceName);

	if (obj->id == ObjectInstanceID(objects.size()))
		objects.emplace_back(obj);
	else
		objects[obj->id.getNum()] = obj;

	instanceNames[obj->instanceName] = obj;
	showObject(obj.get());

	//TODO: how about defeated heroes recruited again?

	obj->afterAddToMap(this);
}

void CMap::moveObject(ObjectInstanceID target, const int3 & dst)
{
	auto obj = objects.at(target).get();
	hideObject(obj);
	obj->setAnchorPos(dst);
	showObject(obj);
}

std::shared_ptr<CGObjectInstance> CMap::removeObject(ObjectInstanceID oldObject)
{
	auto obj = objects.at(oldObject);

	hideObject(obj.get());
	instanceNames.erase(obj->instanceName);
	obj->afterRemoveFromMap(this);

	//update indices

	auto iter = std::next(objects.begin(), obj->id.getNum());
	iter = objects.erase(iter);

	for(int i = obj->id.getNum(); iter != objects.end(); ++i, ++iter)
		(*iter)->id = ObjectInstanceID(i);

	for (auto & town : towns)
		if (town.getNum() >= obj->id)
			town = ObjectInstanceID(town.getNum()-1);

	for (auto & hero : heroesOnMap)
		if (hero.getNum() >= obj->id)
			hero = ObjectInstanceID(hero.getNum()-1);

	for(auto tile = terrain.origin(); tile < (terrain.origin() + terrain.num_elements()); ++tile)
	{
		for (auto & objectID : tile->blockingObjects)
			if (objectID.getNum() >= obj->id)
				objectID = ObjectInstanceID(objectID.getNum()-1);

		for (auto & objectID : tile->visitableObjects)
			if (objectID.getNum() >= obj->id)
				objectID = ObjectInstanceID(objectID.getNum()-1);
	}

	//TODO: Clean artifact instances (mostly worn by hero?) and quests related to this object
	//This causes crash with undo/redo in editor

	return obj;
}

std::shared_ptr<CGObjectInstance> CMap::replaceObject(ObjectInstanceID oldObjectID, const std::shared_ptr<CGObjectInstance> & newObject)
{
	auto oldObject = objects.at(oldObjectID.getNum());

	newObject->id = oldObjectID;

	hideObject(oldObject.get());
	instanceNames.erase(oldObject->instanceName);

	objects.at(oldObjectID.getNum()) = newObject;
	showObject(newObject.get());
	instanceNames[newObject->instanceName] = newObject;

	oldObject->afterRemoveFromMap(this);
	newObject->afterAddToMap(this);

	return oldObject;
}

std::shared_ptr<CGObjectInstance> CMap::eraseObject(ObjectInstanceID oldObjectID)
{
	auto oldObject = objects.at(oldObjectID.getNum());

	instanceNames.erase(oldObject->instanceName);
	objects.at(oldObjectID) = nullptr;
	hideObject(oldObject.get());
	oldObject->afterRemoveFromMap(this);

	return oldObject;
}

void CMap::heroAddedToMap(const CGHeroInstance * hero)
{
	assert(!vstd::contains(heroesOnMap, hero->id));
	heroesOnMap.push_back(hero->id);
}

void CMap::heroRemovedFromMap(const CGHeroInstance * hero)
{
	assert(vstd::contains(heroesOnMap, hero->id));
	vstd::erase(heroesOnMap, hero->id);
}

void CMap::townAddedToMap(const CGTownInstance * town)
{
	assert(!vstd::contains(towns, town->id));
	towns.push_back(town->id);
}

void CMap::townRemovedFromMap(const CGTownInstance * town)
{
	assert(vstd::contains(towns, town->id));
	vstd::erase(towns, town->id);
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
	else
	{
		waterMap = false;
	}

	return waterMap;
}

void CMap::banWaterContent()
{
	banWaterHeroes(isWaterMap());
	banWaterArtifacts();
	banWaterSpells();
	banWaterSkills();
}

void CMap::banWaterSpells()
{
	vstd::erase_if(allowedSpells, [&](SpellID spell)
	{
		return spell.toSpell()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMap::banWaterArtifacts()
{
	vstd::erase_if(allowedArtifact, [&](ArtifactID artifact)
	{
		return artifact.toArtifact()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMap::banWaterSkills()
{
	vstd::erase_if(allowedAbilities, [&](SecondarySkill skill)
	{
		return skill.toSkill()->onlyOnWaterMap && !isWaterMap();
	});
}

void CMapHeader::banWaterHeroes(bool isWaterMap)
{
	vstd::erase_if(allowedHeroes, [&](HeroTypeID hero)
	{
		return hero.toHeroType()->onlyOnWaterMap && !isWaterMap;
	});

	vstd::erase_if(allowedHeroes, [&](HeroTypeID hero)
	{
		return hero.toHeroType()->onlyOnMapWithoutWater && isWaterMap;
	});
}

void CMap::banHero(const HeroTypeID & id)
{
	if (!vstd::contains(allowedHeroes, id))
		logGlobal->warn("Attempt to ban hero %s, who is already not allowed", id.encode(id));
	allowedHeroes.erase(id);
}

void CMap::unbanHero(const HeroTypeID & id)
{
	if (vstd::contains(allowedHeroes, id))
		logGlobal->warn("Attempt to unban hero %s, who is already allowed", id.encode(id));
	allowedHeroes.insert(id);
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

void CMap::reindexObjects()
{
	// Only reindex at editor / RMG operations

	auto oldIndex = objects;

	std::sort(objects.begin(), objects.end(), [](const auto & lhs, const auto & rhs)
	{
		// Obstacles first, then visitable, at the end - removable

		if (!lhs->isVisitable() && rhs->isVisitable())
			return true;
		if (lhs->isVisitable() && !rhs->isVisitable())
			return false;

		// Special case for Windomill - draw on top of other objects
		if (lhs->ID != Obj::WINDMILL && rhs->ID == Obj::WINDMILL)
			return true;
		if (lhs->ID == Obj::WINDMILL && rhs->ID != Obj::WINDMILL)
			return false;

		if (!lhs->isRemovable() && rhs->isRemovable())
			return true;
		if (lhs->isRemovable() && !rhs->isRemovable())
			return false;

		return lhs->anchorPos().y < rhs->anchorPos().y;
	});

	// instanceNames don't change
	for (size_t i = 0; i < objects.size(); ++i)
		objects[i]->id = ObjectInstanceID(i);

	for (auto & town : towns)
		town = oldIndex.at(town.getNum())->id;

	for (auto & hero : heroesOnMap)
		hero = oldIndex.at(hero.getNum())->id;

	for(auto tile = terrain.origin(); tile < (terrain.origin() + terrain.num_elements()); ++tile)
	{
		for (auto & objectID : tile->blockingObjects)
			objectID = oldIndex.at(objectID.getNum())->id;

		for (auto & objectID : tile->visitableObjects)
			objectID = oldIndex.at(objectID.getNum())->id;
	}
}

const IGameSettings & CMap::getSettings() const
{
	return *gameSettings;
}

void CMap::overrideGameSetting(EGameSettings option, const JsonNode & input)
{
	return gameSettings->addOverride(option, input);
}

void CMap::overrideGameSettings(const JsonNode & input)
{
	return gameSettings->loadOverrides(input);
}

CArtifactInstance * CMap::createScroll(const SpellID & spellId)
{
	return createArtifact(ArtifactID::SPELL_SCROLL, spellId);
}

CArtifactInstance * CMap::createArtifactComponent(const ArtifactID & artId)
{
	auto newArtifact = artId.hasValue() ?
		std::make_shared<CArtifactInstance>(cb, artId.toArtifact()):
		std::make_shared<CArtifactInstance>(cb);

	newArtifact->setId(ArtifactInstanceID(artInstances.size()));
	artInstances.push_back(newArtifact);
	return newArtifact.get();
}

CArtifactInstance * CMap::createArtifact(const ArtifactID & artID, const SpellID & spellId)
{
	if(!artID.hasValue())
		throw std::runtime_error("Can't create empty artifact!");

	auto art = artID.toArtifact();

	auto artInst = createArtifactComponent(artID);
	if(art->isCombined() && !art->isFused())
	{
		for(const auto & part : art->getConstituents())
			artInst->addPart(createArtifact(part->getId(), spellId), ArtifactPosition::PRE_FIRST);
	}
	if(art->isGrowing())
	{
		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::ARTIFACT_GROWING;
		bonus->val = 0;
		artInst->addNewBonus(bonus);
	}
	if(art->isScroll())
	{
		artInst->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL,
													 BonusSource::ARTIFACT_INSTANCE, -1, BonusSourceID(ArtifactID(ArtifactID::SPELL_SCROLL)), BonusSubtypeID(spellId)));
	}
	if(art->isCharged())
	{
		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::ARTIFACT_CHARGE;
		bonus->sid = artInst->getId();
		bonus->val = 0;
		artInst->addNewBonus(bonus);
		artInst->addCharges(art->getDefaultStartCharges());
	}

	for (const auto & bonus : art->instanceBonuses)
		artInst->addNewBonus(std::make_shared<Bonus>(*bonus, artInst->getId()));

	return artInst;
}

CArtifactInstance * CMap::getArtifactInstance(const ArtifactInstanceID & artifactID)
{
	return artInstances.at(artifactID.getNum()).get();
}

const CArtifactInstance * CMap::getArtifactInstance(const ArtifactInstanceID & artifactID) const
{
	return artInstances.at(artifactID.getNum()).get();
}

const std::vector<ObjectInstanceID> & CMap::getAllTowns() const
{
	return towns;
}

const std::vector<ObjectInstanceID> & CMap::getHeroesOnMap() const
{
	return heroesOnMap;
}

void CMap::addToHeroPool(std::shared_ptr<CGHeroInstance> hero)
{
	assert(hero->getHeroTypeID().isValid());
	assert(!vstd::contains(heroesOnMap, hero->id));
	assert(heroesPool.at(hero->getHeroTypeID().getNum()) == nullptr);

	heroesPool.at(hero->getHeroTypeID().getNum()) = hero;

	if (!hero->id.hasValue())
	{
		// reserve ID for this new hero, if needed (but don't actually add it since hero is not present on map)
		hero->id = ObjectInstanceID(objects.size());
		objects.push_back(nullptr);
	}
}

CGHeroInstance * CMap::tryGetFromHeroPool(HeroTypeID hero)
{
	return heroesPool.at(hero.getNum()).get();
}

std::shared_ptr<CGHeroInstance> CMap::tryTakeFromHeroPool(HeroTypeID hero)
{
	auto result = heroesPool.at(hero.getNum());
	heroesPool.at(hero.getNum()) = nullptr;
	return result;
}

std::vector<HeroTypeID> CMap::getHeroesInPool() const
{
	std::vector<HeroTypeID> result;
	for (const auto & hero : heroesPool)
		if (hero)
			result.push_back(hero->getHeroTypeID());

	return result;
}

CGObjectInstance * CMap::getObject(ObjectInstanceID obj)
{
	return objects.at(obj).get();
}

const CGObjectInstance * CMap::getObject(ObjectInstanceID obj) const
{
	return objects.at(obj).get();
}

void CMap::saveCompatibilityStoreAllocatedArtifactID()
{
	if (!artInstances.empty())
		cb->gameState().saveCompatibilityLastAllocatedArtifactID = artInstances.back()->getId();
}

void CMap::saveCompatibilityAddMissingArtifact(std::shared_ptr<CArtifactInstance> artifact)
{
	assert(artifact->getId().getNum() == artInstances.size());
	artInstances.push_back(artifact);
}

ObjectInstanceID CMap::allocateUniqueInstanceID()
{
	objects.push_back(nullptr);
	return ObjectInstanceID(objects.size() - 1);
}

void CMap::parseUidCounter()
{
	int max_index = -1;
	for (const auto& entry : instanceNames) {
		const std::string& key = entry.first;
		const size_t pos = key.find_last_of('_');

		// Validate underscore position
		if (pos == std::string::npos || pos + 1 >= key.size()) {
			logGlobal->error("Instance name '%s' is not valid.", key);
			continue;
		}

		const std::string index_part = key.substr(pos + 1);
		try {
			const int current_index = std::stoi(index_part);
			max_index = std::max(max_index, current_index);
		}
		catch (const std::invalid_argument&) {
			logGlobal->error("Instance name %s contains non-numeric index part: %s", key, index_part);
		}
		catch (const std::out_of_range&) {
			logGlobal->error("Instance name %s index part is overflow.", key);
		}
	}

	// Directly set uidCounter using simplified logic
	uidCounter = max_index + 1;  // Automatically 0 when max_index = -1
}


VCMI_LIB_NAMESPACE_END
